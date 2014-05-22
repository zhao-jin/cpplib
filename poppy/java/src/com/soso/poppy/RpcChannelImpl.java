/*
 * @(#)RpcChannelImpl.java
 *
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

import java.util.Collections;
import java.util.ArrayList;
import java.util.List;
import java.util.ListIterator;
import java.util.Map;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantReadWriteLock;
import java.util.concurrent.atomic.AtomicLong;

import java.net.InetSocketAddress;

import org.apache.log4j.Logger;
import org.jboss.netty.channel.ChannelPipeline;
import org.jboss.netty.channel.Channels;
import org.jboss.netty.channel.socket.SocketChannel;

import com.google.protobuf.Message;
import com.google.protobuf.RpcCallback;

/*
 * RpcChannelImpl maintains a list of server, which are identical and stateless.
 * Once a session closes, all responses on this session are discarded, but all
 * pending requests would be kept and sent again later. It also has a scheduling
 * method for load balance among sessions.
 * It holds a lock to protect internal table and all interface functions are
 * reentrant.
 */
public class RpcChannelImpl implements AddressPublisher.Subscriber {
    static private Logger logger = Logger.getLogger("RpcChannelImpl");

    // data members
    private String channelName;
    // Rpc channel. Manage all live connections.
    private RpcClient rpcClient;
    // Server names
    private List<String> serverAddresses;

    private RpcConnectionGroup connectionGroup;

    // Hold all pending requests, dispatched or undispatched.
    private RpcRequestQueue pendingRequests;

    // Read Write lock to protect internal data structures.
    ReentrantReadWriteLock connectionLock;

    private int connectStatus;
    // Exit flag. if set, there is no need for reconnecting closed sessions.
    private boolean closing;

    // Allocate for each request. Doing this in session pool instead of session
    // is to help re-schedule request among sessions.
    private AtomicLong lastSequenceId;

    // time when send the latest package.
    private long latestSendTime = 0;
    private long latestTimestamp = 0;

    // time for re-connecting.
    private List<Map.Entry<Long, String>> reconnectTimestamp;

    RpcChannelImpl(RpcClient client, String name, List<String> servers) {
        this.channelName = name;
        this.rpcClient = client;
        this.serverAddresses = servers;
        this.connectionGroup = new RpcConnectionGroup();
        this.pendingRequests = new RpcRequestQueue(client, this.connectionGroup);
        this.connectStatus = RpcConnection.ConnectStatus.Disconnected;
        this.closing = false;
        this.lastSequenceId = new AtomicLong();
        this.latestTimestamp = 0;
        this.latestSendTime = 0;
        this.connectionLock = new ReentrantReadWriteLock();
        this.reconnectTimestamp = new ArrayList<Map.Entry<Long, String>>();
        Collections.sort(this.serverAddresses);
        this.rpcClient.getTimerManager().schedule(new java.util.TimerTask() {
            public void run() {
                onTimer();
            }
        }, 0, 100);
    }

    /*
     * It's only called by user's thread.
     */
    void sendRequest(com.google.protobuf.Descriptors.MethodDescriptor method,
            RpcController controller, Message request, Message response,
            RpcCallback<Message> done) {
        if (controller.getConnectionId() != -1) {
            logger.error("The connection id of controller "
                    + "must NOT be set at client side.");
            return;
        }
        RpcRequest rpcRequest = createRequest(
                method,
                controller,
                request,
                response,
                done);

        this.latestSendTime = this.latestTimestamp;

        Lock locker = this.connectionLock.readLock();
        locker.lock();
        try {
            HttpConnection httpConnection
                    = this.connectionGroup.getMostIdleConnection();
            if (httpConnection != null) {
                // SendRequest
                rpcRequest.controller
                        .setConnectionId(httpConnection.getConnectionId());
                sendRequest(httpConnection, rpcRequest);
                return;
            }
            // No available session now, add as an undispatched request.
            this.pendingRequests.appendUndispatchedRequest(rpcRequest);
        } catch (Exception e) {
            logger.error(e.getMessage());
        } finally {
            locker.unlock();
        }
    }

    /*
     * Called when a new http connection has been verified as a valid rpc
     * session.
     */
    void onNewSession(HttpConnection http_connection) {
        logger.info("RpcChannelImpl.onNewSession");
        Lock locker = this.connectionLock.writeLock();
        locker.lock();
        try {
            // The connection is valid as a rpc communication channel.
            this.connectStatus = RpcConnection.ConnectStatus.Connected;
            this.connectionGroup.changeStatus(
                    http_connection.getConnectionId(),
                    RpcConnection.ConnectStatus.Connected);
            redispatchRequests();
        } finally {
            locker.unlock();
        }
    }

    void onClose(int connectionId, int errorCode) {
        if (connectionId < 0) {
            logger.error("The connection id must have "
                    + "been set at client side before closed.");
        }
        Lock locker = this.connectionLock.writeLock();
        locker.lock();
        try {
            // Don't cancel the requests but set them as "undispatched".
            this.pendingRequests.releaseConnection(connectionId, false);
            // Maintain connection status
            String serverAddress
                    = this.connectionGroup.getAddressFromConnectionId(connectionId);
            if (serverAddress == null) {
                throw new Exception(
                        "Fail to get address for this connection id.");
            }
            this.connectionGroup.changeStatus(
                    connectionId,
                    RpcConnection.ConnectStatus.Disconnected);
            this.connectionGroup.removeConnection(connectionId);

            if (this.closing && this.connectionGroup.isAvailable()) {
                this.connectStatus = RpcConnection.ConnectStatus.Disconnected;
            }

            if (this.connectStatus == RpcConnection.ConnectStatus.Connecting
                    || this.connectStatus == RpcConnection.ConnectStatus.Connected) {
                // retry 3 seconds later.
                long timestamp = this.latestTimestamp + 1000 * 3;
                this.reconnectTimestamp.add(new Pair<Long, String>(timestamp,
                        serverAddress));
            }
        } catch (Exception e) {
            logger.error(e.getMessage());
        } finally {
            locker.unlock();
        }
        redispatchRequests();
    }

    void handleResponse(RpcMetaInfo.RpcMeta rpcMeta, byte[] messageData) {
        // m_logger.info("RpcChannelImpl.handleResponse");
        RpcRequest rpcRequest = null;
        try {
            rpcRequest
                    = this.pendingRequests.removeRequest(rpcMeta.getSequenceId());
        } catch (Exception e) {
            logger.error(e.getMessage());
            return;
        }
        if (rpcRequest == null) {
            logger.info("Fail to get request from the queue, maybe it has expired"
                    + ". sequence id: "
                    .concat(Long.toString(rpcMeta.getSequenceId())));
            return;
        }
        long connectionId = rpcRequest.controller.getConnectionId();
        this.connectionGroup.confirmRequest(connectionId,
                RpcConnection.RequestConfirmType.Response);

        if (!rpcRequest.controller.failed()) {
            try {
                Message.Builder builder = rpcRequest.response.newBuilderForType();
                builder.mergeFrom(messageData);
                rpcRequest.response = builder.build();
            } catch (Exception e) {
                rpcRequest.controller.setFailed("Failed to parse the response message.");
            }
        }

        if (rpcRequest.controller.isSync()) {
            rpcRequest.done.run(rpcRequest.response);
        } else {
            final RpcRequest request = rpcRequest;
            this.rpcClient.getThreadPool().submit(
                    new Runnable() {
                        public void run() {
                            request.done.run(request.response);
                        }
                    });
        }
    }

    void callMethod(com.google.protobuf.Descriptors.MethodDescriptor method,
            com.google.protobuf.RpcController controller,
            com.google.protobuf.Message request,
            com.google.protobuf.Message response,
            com.google.protobuf.RpcCallback<Message> done) {
        connectServers(this.serverAddresses);

        sendRequest(method, (RpcController) controller, request, response, done);
    }

    int getConnectStatus() {
        return this.connectStatus;
    }

    String getName() {
        return this.channelName;
    }

    void getServerAddresses(List<String> addresses) {
        addresses.addAll(this.serverAddresses);
    }

    public String dump() {
        // TODO
        return "";
    }

    RpcRequest removeRequest(long sequenceId) throws Exception {
        RpcRequest rpcRequest = this.pendingRequests.removeRequest(sequenceId);
        return rpcRequest;
    }

    void close() {
        closeAllSessions();
    }

    private void closeAllSessions() {
        Lock locker = this.connectionLock.writeLock();
        locker.lock();
        try {
            // Set to closing.
            this.connectStatus = RpcConnection.ConnectStatus.Disconnecting;
            this.closing = true;

            // No new client side HTTP connection after here.
            this.connectionGroup.closeAllSessions();
        } finally {
            locker.unlock();
        }
    }

    private void connectServer(final String address) {
        // New handler and connection will be taken over by http frame.
        RpcClientHandler handler = new RpcClientHandler();
        handler.setChannelImpl(this);
        HttpConnection httpConnection = new HttpClientConnection(handler);
        ChannelPipeline pipeline = Channels.pipeline();
        pipeline.addLast("", httpConnection);
        SocketChannel socketChannel
                = this.rpcClient.getNettyChannelFactory().newChannel(pipeline);
        String[] addr = address.split(":");
        InetSocketAddress sa = new InetSocketAddress(addr[0], Integer.parseInt(addr[1]));
        socketChannel.connect(sa);
        httpConnection.setChannel(socketChannel);
        httpConnection.setRemoteAddress(sa);
        Lock locker = this.connectionLock.writeLock();
        locker.lock();
        try {
            this.connectionGroup.insertConnection(
                    address,
                    socketChannel.getId(),
                    httpConnection);
            this.connectionGroup.changeStatus(
                    socketChannel.getId(),
                    RpcConnection.ConnectStatus.Connecting);
        } finally {
            locker.unlock();
        }
    }

    private void connectServers(final List<String> serverAddresses) {
        if (this.connectStatus == RpcConnection.ConnectStatus.Connected
                || this.connectStatus == RpcConnection.ConnectStatus.Connecting) {
            return;
        }
        Lock locker = this.connectionLock.writeLock();
        locker.lock();
        try {
            if (this.connectStatus == RpcConnection.ConnectStatus.Connected
                    || this.connectStatus == RpcConnection.ConnectStatus.Connecting) {
                return;
            }
            this.closing = false;
            this.connectStatus = RpcConnection.ConnectStatus.Connecting;
        } finally {
            locker.unlock();
        }
        for (int i = 0; i < serverAddresses.size(); ++i) {
            connectServer(serverAddresses.get(i));
        }
    }

    private RpcRequest createRequest(
            com.google.protobuf.Descriptors.MethodDescriptor method,
            RpcController controller, Message request, Message response,
            RpcCallback<Message> done) {
        // Allocate a unique sequence id by increasing a counter monotonously.
        long sequenceId = this.lastSequenceId.addAndGet(1);
        controller.setSequenceId(sequenceId);

        // Calculate the expected timeout.
        long timeoutStamp = controller.getTimeout();
        if (timeoutStamp < 0) {
            if (method.getOptions().hasExtension(RpcOption.methodTimeout)) {
                timeoutStamp = method.getOptions().getExtension(RpcOption.methodTimeout);
            } else {
                timeoutStamp = method.getService().getOptions()
                    .getExtension(RpcOption.serviceTimeout);
            }
        }
        if (timeoutStamp <= 0) {
            // Just a protection, it shouldn't happen.
            timeoutStamp = 1;
        }
        long expectedTimeout = timeoutStamp;
        RpcRequest rpcRequest = new RpcRequest(sequenceId, expectedTimeout,
                method, controller, request, response, done);
        return rpcRequest;
    }

    private void sendRequest(HttpConnection httpConnection,
            RpcRequest rpcRequest) {
        try {
            this.connectionGroup.addRequest(httpConnection.getConnectionId());
            this.pendingRequests.sendRequest(httpConnection, rpcRequest);
        } catch (Exception e) {
            logger.error(e.getCause().getMessage());
        }
    }

    /*
     * Try to redispatch the undispatched requests to available servers.
     */
    private void redispatchRequests() {
        Lock locker = this.connectionLock.readLock();
        locker.lock();
        try {
            while (true) {
                HttpConnection httpConnection =
                    this.connectionGroup.getMostIdleConnection();
                if (httpConnection == null) {
                    return;
                }
                RpcRequest rpcRequest =
                        this.pendingRequests.popFirstUndispatchedRequest();
                if (rpcRequest == null) {
                    break;
                }
                rpcRequest.controller.setConnectionId(httpConnection.getConnectionId());
                sendRequest(httpConnection, rpcRequest);
            }
        } finally {
            locker.unlock();
        }
    }

    void onTimer() {
        this.latestTimestamp = System.currentTimeMillis();
        doReconnect(this.latestTimestamp);
        doDisconnect(this.latestTimestamp);
        try {
            this.pendingRequests.onTimer(this.latestTimestamp);
        } catch (Exception e) {
        }
    }

    /*
     * Callback when fail to connect to servers
     */
    private void doReconnect(long timestamp) {
        List<String> serveres = new ArrayList<String>();
        Lock locker = this.connectionLock.readLock();
        locker.lock();
        try {
            if (this.connectStatus == RpcConnection.ConnectStatus.Connecting
                    || this.connectStatus == RpcConnection.ConnectStatus.Connected) {
                return;
            }
            while (!this.reconnectTimestamp.isEmpty()
                    && this.reconnectTimestamp.get(0).getKey() < timestamp) {
                Map.Entry<Long, String> entry = this.reconnectTimestamp.remove(0);
                serveres.add(entry.getValue());
            }
            // Don't re-connect when the channel is connecting or connected
        } finally {
            locker.unlock();
        }

        // There are still pending requests, retry to connect.
        for (String server : serveres) {
            connectServer(server);
        }
    }

    /*
     * when there isn't any request in the latest 1 minute, disconnect.
     */
    private void doDisconnect(long timestamp) {
        if (this.connectStatus == RpcConnection.ConnectStatus.Connected
                && timestamp > this.latestSendTime + 60 * 1000) {
            closeAllSessions();
        }
    }

    private void disconnectServer(String address) {
        Lock locker = this.connectionLock.writeLock();
        locker.lock();
        try { // for scoped lock.
            long connectionId =
                this.connectionGroup.getConnectionIdFromAddress(address);
            if (connectionId == -1) {
                return;
            }

            this.pendingRequests.releaseConnection(connectionId, false);
            this.connectionGroup.changeStatus(connectionId,
                    RpcConnection.ConnectStatus.Disconnecting);
        } catch (Exception e) {
            logger.warn(e.getMessage());
        } finally {
            locker.unlock();
        }
        redispatchRequests();
    }

    /// implement AddressPublisher.Subscriber
    public void onAddressChanged(String path, List<String> addresses) {
        List<String> servers = addresses;
        Collections.sort(servers);

        List<String> addList = new ArrayList<String>();
        List<String> removeList = new ArrayList<String>();

        Lock locker = this.connectionLock.writeLock();
        locker.lock();
        try {
            ListIterator<String> it1 = servers.listIterator();
            ListIterator<String> it2 = this.serverAddresses.listIterator();
            while (it1.hasNext() && it2.hasNext()) {
                String s1 = it1.next();
                String s2 = it2.next();
                if (s1.equalsIgnoreCase(s2)) {
                    continue;
                } else if (s1.compareToIgnoreCase(s2) < 0) {
                    addList.add(s1);
                    it2.previous();
                } else {
                    removeList.add(s1);
                    it1.previous();
                }
            }
            while (it1.hasNext()) {
                addList.add(it1.next());
            }
            while (it2.hasNext()) {
                removeList.add(it2.next());
            }
            this.serverAddresses = servers;
        } finally {
            locker.unlock();
        }

        ListIterator<String> it1 = addList.listIterator();
        while (it1.hasNext()) {
            connectServer(it1.next());
        }
        ListIterator<String> it2 = removeList.listIterator();
        while (it2.hasNext()) {
            disconnectServer(it2.next());
        }
    }

    public void onConnectionClosed(String path) {
        close();
    }
}
