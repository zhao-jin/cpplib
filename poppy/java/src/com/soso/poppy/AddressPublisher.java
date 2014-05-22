package com.soso.poppy;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
// thirdparty
import org.apache.log4j.Logger;
import org.apache.zookeeper.KeeperException;
import org.apache.zookeeper.WatchedEvent;
import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.ZooKeeper;
import org.apache.zookeeper.AsyncCallback.StatCallback;
import org.apache.zookeeper.data.Stat;

class ZookeeperConnection implements StatCallback, Watcher {
    static private Logger logger = Logger.getLogger("ZookeeperListener");
    ZooKeeper zk;
    Object mutex;
    Object event;
    boolean connected;
    AddressPublisher publisher;

    class Subscribers {
        List<AddressPublisher.Subscriber> members;
        Subscribers() {
            members = new ArrayList<AddressPublisher.Subscriber>();
        }
    }

    Map<String, Subscribers> subscriberMap; // znode and subscribers
    String server;

    static String getZookeeperServer(String path) {
        int pos = path.indexOf('/');
        return (pos == -1) ? path : path.substring(0, pos);
    }

    static String getZookeeperZnode(String path) {
        int pos = path.indexOf('/');
        return (pos == -1) ? null : path.substring(pos);
    }

    ZookeeperConnection(String server, AddressPublisher publisher) throws IOException {
        this.mutex = new Object();
        this.event = new Object();
        this.connected = false;
        this.server = server;
        this.publisher = publisher;
        this.subscriberMap = new TreeMap<String, Subscribers>();
        this.zk = new ZooKeeper(server, 10000, this);
    }

    void subscribe(String path, AddressPublisher.Subscriber subscriber) {
        String znode = getZookeeperZnode(path);
        if (znode == null) {
            return;
        }
        synchronized(this.mutex) {
            Subscribers subscribers = this.subscriberMap.get(znode);
            if (subscribers == null) {
                subscribers = new Subscribers();
                this.subscriberMap.put(znode, subscribers);
                // start to watch its status.
                zk.exists(znode, true, this, null);
            }
            subscribers.members.add(subscriber);
        }
    }

    void unsubscribe(String path, AddressPublisher.Subscriber subscriber) {
        String znode = getZookeeperZnode(path);
        if (znode == null) {
            return;
        }
        synchronized(this.mutex) {
            Subscribers subscribers = this.subscriberMap.get(znode);
            if (subscribers == null) {
                logger.error("Nobody is listening to path: " + znode);
                return;
            }
            if (!subscribers.members.remove(subscriber)) {
                logger.error("Path is not being listened. path: " + znode);
            }
            // remove this node if nobody is watching it.
            if (subscribers.members.isEmpty()) {
                this.subscriberMap.remove(znode);
            }
        }
    }

    String resolve(String path) {
        String znode = getZookeeperZnode(path);
        if (znode == null) {
            return null;
        }
        while (true) {
            try {
                // logger.info(znode);
                byte[] buff = zk.getData(znode, false, null);
                String ctx = new String(buff);
                return ctx;
            } catch (KeeperException e) {
                logger.error("Error when getting data from ZooKeeper server. "
                        .concat(e.getMessage()));
                return null;
            } catch (InterruptedException e) {
                logger.warn("ZooKeeper server transaction is interrupted. Retry.");
            }
        }
    }

    void onChanged(String path, String addr) {
        Subscribers subscribers = this.subscriberMap.get(path);
        if (subscribers == null) {
            return;
        }

        List<String> addresses = new ArrayList<String>();
        for(String item : addr.split(",")) {
            addresses.add(item);
        }

        for(AddressPublisher.Subscriber s : subscribers.members) {
            s.onAddressChanged(path, addresses);
        }
    }

    void onClosed() {
        logger.error("Connection to Zookeeper server is closed. server: " + server);

        for (Map.Entry<String, Subscribers> mapEntry : this.subscriberMap.entrySet()) {
            for(AddressPublisher.Subscriber subscriber : mapEntry.getValue().members) {
                subscriber.onConnectionClosed(mapEntry.getKey());
            }
        }
    }

    boolean observing(String znode) {
        return this.subscriberMap.containsKey(znode);
    }

    void close() {
        synchronized(this.mutex) {
            try {
                zk.close();
            } catch (InterruptedException e) {
                logger.warn("Transcation is interrupted when disconnecting from ZooKeeper "
                        + "server.");
            }
        }
    }

    public void onConnected() {
        synchronized(this.event) {
            this.connected = true;
            this.event.notify();
        }
    }
    public boolean waitForConnected(long timeout) throws InterruptedException {
        synchronized(this.event) {
            this.event.wait(timeout);
            return this.connected;
        }
    }

    /*
     * implement org.apache.zookeeper.Watcher.
     *
     * This interface specifies the public interface an event handler class must
     * implement. A ZooKeeper client will get various events from the ZooKeepr
     * server it connects to. An application using such a client handles these
     * events by registering a callback object with the client. The callback object
     * is expected to be an instance of a class that implements Watcher interface.
     */
    public void process(WatchedEvent event) {
        String path = event.getPath();
        synchronized(this.mutex) {
            if (event.getType() == Event.EventType.None) {
                // We are are being told that the state of the
                // connection has changed
                switch (event.getState()) {
                case SyncConnected:
                    // In this particular example we don't need to do anything
                    // here - watches are automatically re-registered with
                    // server and any watches triggered while the client was
                    // disconnected will be delivered (in order of course)
                    onConnected();
                    break;
                case Expired:
                    // It's all over
                    onClosed();
                    publisher.removeServer(server);
                    break;
                }
            } else {
                // Something has changed on a node, let's find out
                // connection.process(event);
                if (path != null && observing(path) ) {
                    // Something has changed on a node, let's find out
                    this.zk.exists(path, true, this, null);
                }
            }
        }
    }


    /*
     * Implement interface org.apache.zookeeper.AsyncCallback.StatCallback.
     *
     * This is a callback, which is called when call ZooKeeper.exists() asynchronously.
     * The 3rd parameter of Zookeeper.exists is AsyncCallback.StatCallback, which is
     * supposed to provide a method named processResult, this is the callback function.
     */
    public void processResult(int rc, String path, Object ctx, Stat stat) {
        boolean exists = false;
        synchronized(this.mutex) {
            switch (KeeperException.Code.get(rc)) {
            case OK:
                exists = true;
                break;
            case NONODE:
                exists = false;
                break;
            case SESSIONEXPIRED :
            case NOAUTH:
                onClosed();
                break;
            default:
                // Retry errors
                this.zk.exists(path, true, this, null);
                return;
            }

            byte buff[] = null;
            if (exists) {
                try {
                    buff = zk.getData(path, false, null);
                } catch (KeeperException e) {
                    // We don't need to worry about recovering now. The watch
                    // callbacks will kick off any exception handling
                    e.printStackTrace();
                } catch (InterruptedException e) {
                    logger.warn("Transaction to Zookeeper server is interrupted.");
                    return;
                }
            }
            onChanged(path, buff == null ? new String() : new String(buff));
        }
    }

    boolean isEmpty() {
        return subscriberMap.isEmpty();
    }
}

public class AddressPublisher {
    static final String ZOOKEEPER_PREFIX = "zk://";

    static final int TYPE_IPLIST = 0;
    static final int TYPE_ZOOKEEPER = 1;

    static private Logger logger = Logger.getLogger("AddressPublisher");

    private Map<String, ZookeeperConnection> connections;
    private Object mutex;

    // interface for who listen to the address changes.
    public interface Subscriber {
        void onAddressChanged(String path, List<String> addresses);
        void onConnectionClosed(String path);
    }

    public AddressPublisher() {
        this.connections = new TreeMap<String, ZookeeperConnection>();
        this.mutex = new Object();
    }

    public static int getAddressType(String address) {
        if (address.startsWith(ZOOKEEPER_PREFIX)) {
            return TYPE_ZOOKEEPER;
        }
        return TYPE_IPLIST;
    }

    public boolean resolveAddress(String address, List<String> ipList) {
        switch (getAddressType(address)) {
        case TYPE_ZOOKEEPER:
            return resolveZookeeperAddress(
                    address.substring(ZOOKEEPER_PREFIX.length()),
                    ipList);
        case TYPE_IPLIST:
            return resolveIpAddress(address, ipList);
        }
        return false;
    }

    private boolean resolveIpAddress(String address, List<String> ipList) {
        ipList.clear();
        String[] addresses = address.split(",");
        for(int k = 0; k < addresses.length; ++k) {
            try {
                String[] addr = addresses[k].split(":");
                // Exception may pop up here
                InetAddress.getByName(addr[0]);
                ipList.add(addresses[k]);
            } catch(UnknownHostException e) {
                // return false when format is wrong.
                logger.error("Wrong network address: " + addresses[k]);
                ipList.clear();
                return false;
            }
        }
        return true;
    }

    private boolean resolveZookeeperAddress(String address, List<String> ipList) {
        ipList.clear();
        String server = ZookeeperConnection.getZookeeperServer(address);
        ZookeeperConnection connection = getZookeeperConnection(server, true);
        String ctx = connection.resolve(address);
        if (ctx != null) {
            logger.info(ctx);
            return resolveIpAddress(ctx, ipList);
        }
        return false;
    }

    private ZookeeperConnection getZookeeperConnection(String server, boolean createIfNotExist) {
        ZookeeperConnection connection = this.connections.get(server);
        // insert a new one if not exist
        if (connection == null && createIfNotExist == true) {
            try {
                connection = new ZookeeperConnection(server, this);
                connection.waitForConnected(3000);
                this.connections.put(server, connection);
            } catch (Exception e) {
                logger.error("Fail to create ZookeeperConnection: " + server);
                logger.error(e.getMessage());
                logger.error(e.getClass().getName());
                return null;
            }
        }
        return connection;
    }

    public void subscribe(String path, Subscriber subscriber) {
        if (getAddressType(path) != TYPE_ZOOKEEPER) {
            return;
        }
        synchronized(this.mutex) {
            String zkpath = path.substring(ZOOKEEPER_PREFIX.length());
            String server = ZookeeperConnection.getZookeeperServer(zkpath);
            ZookeeperConnection connection = getZookeeperConnection(server, true);
            if (connection != null) {
                connection.subscribe(zkpath, subscriber);
            }
        }
    }

    public void unsubscribe(String path, Subscriber subscriber) {
        if (getAddressType(path) != TYPE_ZOOKEEPER) {
            return;
        }
        synchronized(this.mutex) {
            String zkpath = path.substring(ZOOKEEPER_PREFIX.length());
            String server = ZookeeperConnection.getZookeeperServer(zkpath);
            ZookeeperConnection connection = getZookeeperConnection(server, false);
            if (connection != null) {
                connection.unsubscribe(zkpath, subscriber);
                if (connection.isEmpty()) {
                    connection.close();
                    connections.remove(server);
                }
            }
        }
    }

    void removeServer(String server) {
        synchronized(this.mutex) {
            this.connections.remove(server);
        }
    }

    private boolean interestedIn(String znode) {
        String server = ZookeeperConnection.getZookeeperServer(znode);
        return getZookeeperConnection(server, false) != null;
    }
}
