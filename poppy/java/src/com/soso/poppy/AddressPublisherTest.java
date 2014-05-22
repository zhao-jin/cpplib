package com.soso.poppy;

import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

import org.apache.log4j.Logger;
import org.apache.zookeeper.CreateMode;
import org.apache.zookeeper.WatchedEvent;
import org.apache.zookeeper.Watcher;
import org.apache.zookeeper.ZooKeeper;
import org.apache.zookeeper.ZooDefs;

/// This is used to test the functionality of AddressPublisher.
public class AddressPublisherTest implements AddressPublisher.Subscriber, Watcher{
    static Logger logger = Logger.getLogger("com.soso.poppy.AddressPublisherTest");
    static String zkserver = "xaec.zk.oa.com:2181";
    static String zkaddress = zkserver + "/zk/xaec/xcube/temp_address_publisher_test";
    static String poppyzkaddress = AddressPublisher.ZOOKEEPER_PREFIX + zkaddress;

    private String path;
    AddressPublisher publisher;
    ZooKeeper zk;
    Object mutex;
    Object event;
    List<String> ipList;

    AddressPublisherTest() {
        this.path = ZookeeperConnection.getZookeeperZnode(zkaddress);
        this.publisher = new AddressPublisher();
        mutex = new Object();
        event = new Object();
        ipList = new ArrayList<String>();
    }

    void StartTest() {
        try{
            logger.info("Start test...");
            this.zk = new ZooKeeper(zkserver, 300, this);
            // logger.info("return from ctor...");
            synchronized(event) {
                event.wait();
            }
            logger.info("create new node...");
            this.zk.create(
                    ZookeeperConnection.getZookeeperZnode(zkaddress),
                    "127.0.0.1:50000".getBytes(),
                    ZooDefs.Ids.OPEN_ACL_UNSAFE,
                    CreateMode.EPHEMERAL);
        } catch (Exception e) {
            logger.fatal("Fail to connect to zookeeper server.");
            logger.fatal(e.getMessage());
            System.exit(-1);
        }
    }

    void StopTest() {
        try {
            logger.info("Stop test...");
            this.zk.close();
        } catch (Exception e) {
            logger.fatal("Fail to disconnect from zookeeper server.");
            System.exit(-1);
        }
    }

    void RunTest() {
        try {
            StartTest();

            Thread.sleep(1000);

            logger.info("resolve address");
            this.publisher.resolveAddress(poppyzkaddress, ipList);
            logger.info("subscribe");
            this.publisher.subscribe(poppyzkaddress, this);

            logger.info("set data");
            this.zk.setData(
                    ZookeeperConnection.getZookeeperZnode(zkaddress),
                    "127.0.0.1:50000,127.0.0.1:50001".getBytes(),
                    -1);

            Thread.sleep(1000);

            logger.info("unsubscribe");
            this.publisher.unsubscribe(poppyzkaddress, this);

            Thread.sleep(1000);

            StopTest();
        } catch(Exception e) {
            logger.fatal("Fail in RunTest");
            logger.fatal(e.getClass().getName());
            e.printStackTrace();
            System.exit(-1);
        }
    }

    public void process(WatchedEvent event) {
        logger.info("watch event here...");
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
                    logger.info("Connected...");
                    try {
                        synchronized(this.event) {
                            this.event.notify();
                        }
                    } catch (Exception e) {
                        logger.fatal("failed to set event");
                        logger.fatal(e.getMessage());
                    }
                    break;
                case Expired:
                    // It's all over
                    logger.fatal("Lost connection to zookeeper server.");
                    System.exit(-1);
                    break;
                }
            }
        }
    }

    /// implement AddressPublisher.Subscriber
    public void onAddressChanged(String path, List<String> addresses) {
        if (!this.path.equals(path)) {
            String message = "Unexpected path, arrival is: " + path +
                             "; expected is: " + this.path + ".";
            logger.fatal(message);
            System.exit(-1);
        }
        StringBuilder sb = new StringBuilder();
        for (String str : addresses) {
            sb.append(str);
            sb.append(",");
        }
        logger.info("Addresses changed, new addresses are: " + sb.toString());
    }
    public void onConnectionClosed(String path) {
        String message = "Connection to " + path + " is reset.";
        logger.info(message);
    }

    public static void main(String[] args) {
        AddressPublisherTest test = new AddressPublisherTest();
        test.RunTest();
    }
}

