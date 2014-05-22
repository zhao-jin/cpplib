/*
 * @(#)RpcRequestQueue.java
 *
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

import com.soso.poppy.RpcOption.CompressType;
import java.util.ArrayList;
import java.util.concurrent.ExecutorService;
import java.util.Map;
import java.util.PriorityQueue;
import java.util.SortedSet;
import java.lang.System;
import java.util.TreeMap;
import java.util.TreeSet;

import org.jboss.netty.buffer.ChannelBuffer;
import org.jboss.netty.buffer.ChannelBuffers;
import org.xerial.snappy.Snappy;

class RpcRequestQueue {
    final static int UNDISPATCHED_REQUEST_CONNECTION_ID = -1;
    // The mutex to protect state of pending requests.
    Object mutex;
    // Pending requests queue. <sequence_id, RpcRequest> pairs.
    TreeMap<Long, RpcRequest> requestQueue;

    RpcConnectionGroup connectionGroup;

    // Sort request by connection id, on same connection request with
    // less sequence id first. <connection id, request id>

    // The pending requests sorted by their connection ids.
    TreeSet<Map.Entry<Long, Long>> connectionRequests;
    // For those requests have not been send because of no connection
    ArrayList<RpcRequest> undispatchedRequests;

    class Timeout implements Comparable<Timeout> {
        long time;
        long sequenceId;

        Timeout(long time, long sequenceId) {
            this.time = time;
            this.sequenceId = sequenceId;
        }

        public int compareTo(Timeout rhs) {
            if (time > rhs.time) {
                return 1;
            } else if (time < rhs.time) {
                return -1;
            }
            if (sequenceId > rhs.sequenceId) {
                return 1;
            } else if (sequenceId < rhs.sequenceId) {
                return -1;
            }
            return 0;
        }
    }
    // Timeout queue.
    PriorityQueue<Timeout> timeoutQueue;

    RpcClient rpcClient;

    private final int RPC_HEADER_SIZE = 8;

    long latestTimestamp = 0;

    RpcRequestQueue(RpcClient rpc_client, RpcConnectionGroup connection_group) {
        this.mutex = new Object();
        this.requestQueue = new TreeMap<Long, RpcRequest>();
        this.connectionGroup = connection_group;
        this.connectionRequests = new TreeSet<Map.Entry<Long, Long>>();
        this.undispatchedRequests = new ArrayList<RpcRequest>();
        this.timeoutQueue = new PriorityQueue<Timeout>();
        this.rpcClient = rpc_client;
    }

    // Add a new request (all information about a request) to the queue.
    // It assumes the sequence id of rpc request has been allocated and it's
    // caller's responsibility to guarantee no duplicated sequence id.
    void addRequest(RpcRequest rpcRequest) throws Exception {
        // Sanity check.
        // CHECK_GE(rpc_request->sequence_id, 0);
        // CHECK_GE(rpc_request->expected_timeout, 0);

        // Insert the request into the request queue.
        if (this.requestQueue.put(rpcRequest.sequenceId, rpcRequest) != null) {
            throw new Exception("Duplicated sequence id in request queue: "
                    .concat(Long.toString(rpcRequest.sequenceId)));
        }

        long connectionId = rpcRequest.controller.getConnectionId();
        if (connectionId != UNDISPATCHED_REQUEST_CONNECTION_ID) {
            // Insert the request into the connection request set.
            if (!this.connectionRequests.add(new Pair<Long, Long>(connectionId,
                    rpcRequest.sequenceId))) {
                throw new Exception(
                        "Duplicated sequence id in connection request set: "
                                .concat(Long.toString(rpcRequest.sequenceId)));
            }
        } else {
            this.undispatchedRequests.add(rpcRequest);
        }
        // Add timeout watcher.
        long expectedTimeout = this.latestTimestamp
                + rpcRequest.expectedTimeout;
        Timeout timeout = new Timeout(expectedTimeout, rpcRequest.sequenceId);
        this.timeoutQueue.add(timeout);
    }

    // Remove the request from queue and return the that request.
    // Return NULL if the sequence id doesn't exist.
    RpcRequest removeRequest(long sequenceId) throws Exception {
        synchronized (this.mutex) {
            RpcRequest rpcRequest = this.requestQueue.get(sequenceId);
            if (rpcRequest == null) {
                return null;
            }

            // Remove item for request queue.
            this.requestQueue.remove(sequenceId);

            long connectionId = rpcRequest.controller.getConnectionId();

            // Remove item for connection request set.
            if (this.connectionRequests.remove(new Pair<Long, Long>(
                    connectionId, sequenceId)) == false) {
                throw new Exception(
                        "Connection request set missing an expected item: "
                                .concat(Long.toString(rpcRequest.sequenceId)));
            }

            return rpcRequest;
        }
    }

    /*
     * Release all pending requests for a connection, and cancel them if
     * "cancel_requests" is true. If "cancel_requests" is false, the request
     * will not be canceled and just flaged as "undispatched" by set connection
     * id to -1.
     * It's called when a connection is closed.
     */
    void releaseConnection(long connectionId, boolean cancelRequests)
            throws Exception {
        // Canceled request list. For temporary storage, cancel the request
        // later.
        ArrayList<RpcRequest> releasedRequests = new ArrayList<RpcRequest>();
        synchronized (this.mutex) {
            SortedSet<Map.Entry<Long, Long>> subSet = this.connectionRequests
                    .subSet(new Pair<Long, Long>(connectionId, 0L),
                            new Pair<Long, Long>(connectionId + 1, 0L));

            for (Map.Entry<Long, Long> entry : subSet) {
                long sequenceId = entry.getValue();
                // Remove item in connection request set.
                this.connectionRequests.remove(entry);
                RpcRequest rpcRequest = this.requestQueue.get(sequenceId);
                if (rpcRequest == null) {
                    throw new Exception(
                            "Request queue missing an expected item: "
                                    .concat(Long.toString(sequenceId)));
                }
                if (!cancelRequests) {
                    // Don't cancel the request. just set undispatched.
                    rpcRequest.controller
                            .setConnectionId(UNDISPATCHED_REQUEST_CONNECTION_ID);
                    // Put undispatched item back to undispatched request queue.
                    this.undispatchedRequests.add(rpcRequest);
                } else {
                    releasedRequests.add(rpcRequest);
                    this.requestQueue.remove(sequenceId);
                }
            }
        }
        for (RpcRequest rpcRequest : releasedRequests) {
            long id = rpcRequest.controller.getConnectionId();
            if (id != UNDISPATCHED_REQUEST_CONNECTION_ID) {
                this.connectionGroup.confirmRequest(id,
                        RpcConnection.RequestConfirmType.Canceled);
            }
            rpcRequest.controller.setFailed("Connection closed.");
            cancelRpcRequest(rpcRequest);
        }
    }

    // Redispatch first undispatched request to specified connection.
    RpcRequest popFirstUndispatchedRequest() {
        synchronized (this.mutex) {
            if (this.undispatchedRequests.isEmpty()) {
                return null;
            }
            RpcRequest request = this.undispatchedRequests.remove(0);
            long sequenceId = request.sequenceId;
            this.requestQueue.remove(sequenceId);
            return request;
        }
    }

    void onTimer(long timestamp) throws Exception {
        // Note: we don't cancel a request under lock as canceling would invoke
        // user's callback and user possibly invoke other functions of rpc
        // request
        // queue which needs to acquire the lock, like the destructor. So we
        // put all timeout requests in a temporary queue on stack and cancel
        // them later.
        this.latestTimestamp = timestamp;
        ArrayList<RpcRequest> timeoutRequests = new ArrayList<RpcRequest>();
        // For scoped mutex locker.
        synchronized (this.mutex) {
            // break if no timeouts or the first timeout doesn't happen.
            while (!this.timeoutQueue.isEmpty()
                    && this.timeoutQueue.peek().time <= timestamp) {
                long sequenceId = this.timeoutQueue.peek().sequenceId;
                RpcRequest rpcRequest = this.requestQueue.get(sequenceId);
                if (rpcRequest == null) {
                    this.timeoutQueue.poll();
                    continue;
                }

                long connectionId = rpcRequest.controller.getConnectionId();
                this.requestQueue.remove(sequenceId);

                if (connectionId == UNDISPATCHED_REQUEST_CONNECTION_ID) {
                    if (!this.undispatchedRequests.remove(rpcRequest)) {
                        throw new Exception(
                                "Request queue missing an expected item: "
                                        .concat(Long.toString(sequenceId)));
                    }
                } else {
                    // Remove the item in connection request set.
                    Map.Entry<Long, Long> entry = new Pair<Long, Long>(
                            connectionId, sequenceId);
                    if (this.connectionRequests.remove(entry) == false) {
                        throw new Exception(
                                "Request queue missing an expected item: "
                                        .concat(Long.toString(sequenceId)));
                    }
                }
                // It's a timeout request, put it in the timeout requests and
                // cancel it later.
                timeoutRequests.add(rpcRequest);
                this.timeoutQueue.poll();
            }
        }
        for (RpcRequest rpcRequest : timeoutRequests) {
            long connectionId = rpcRequest.controller.getConnectionId();
            if (connectionId != UNDISPATCHED_REQUEST_CONNECTION_ID) {
                this.connectionGroup.confirmRequest(connectionId,
                        RpcConnection.RequestConfirmType.Timeout);
            }
            /*
             * VLOG(3) << "Cancel request because timeout: " <<
             * rpc_request->sequence_id;
             */
            rpcRequest.controller.setFailed("Request timeout.");
            cancelRpcRequest(rpcRequest);
        }
    }

    void appendUndispatchedRequest(RpcRequest rpcRequest) throws Exception {
        synchronized (this.mutex) {
            addRequest(rpcRequest);
        }
    }

    void sendRequest(HttpConnection httpConnection, RpcRequest rpcRequest)
            throws Exception {
        RpcMetaInfo.RpcMeta.Builder builder = RpcMetaInfo.RpcMeta.newBuilder()
                .setSequenceId(rpcRequest.sequenceId).setMethod(
                        rpcRequest.method.getFullName());
        RpcMetaInfo.RpcMeta rpcMeta = null;
        rpcRequest.controller.setRemoteAddress(httpConnection
                .getRemoteAddress());
        byte[] messageData = rpcRequest.request.toByteArray();
        if (rpcRequest.method.getOptions().hasExtension(
                RpcOption.requestCompressType)) {
            CompressType compressType = rpcRequest.method.getOptions()
                    .getExtension(RpcOption.requestCompressType);
            switch (compressType) {
            case CompressTypeSnappy: {
                builder.setCompressType(CompressType.CompressTypeSnappy);
                byte[] compressedMessage = Snappy.compress(messageData);
                messageData = compressedMessage;
            }
                break;
            default:
                // Ignore compress error
                throw new Exception("Unknown compress type. sequence id: "
                        .concat(Long.toString(rpcRequest.sequenceId)));
            }
        }

        rpcMeta = builder.build();
        synchronized (this.mutex) {
            addRequest(rpcRequest);
            sendMessage(httpConnection, rpcMeta, messageData);
        }
    }

    void sendMessage(HttpConnection httpConnection,
            RpcMetaInfo.RpcMeta rpcMeta, byte[] message_data) throws Exception {

        byte[] metaBytes = rpcMeta.toByteArray();

        int length = RPC_HEADER_SIZE + metaBytes.length
                + message_data.length;

        ChannelBuffer channelBuffer = ChannelBuffers.buffer(length);
        channelBuffer.writeInt(metaBytes.length);
        channelBuffer.writeInt(message_data.length);
        // Append content of rpc meta to the buffer.
        channelBuffer.writeBytes(metaBytes);
        // Append content of message to the buffer.
        // We have to send all data for one message in one packet to ensure they
        // are sent and received as a whole.
        channelBuffer.writeBytes(message_data);
        // Send the header and rpc meta data.
        // if (!httpConnection.sendPacket(buffer.array())) {
        if (!httpConnection.sendPacket(channelBuffer)) {
            // Failed to send packet, close the connection.
            httpConnection.close(true);
            throw new Exception(
                    "Send packet error, close connection: ".concat(Integer
                            .toString(httpConnection.getConnectionId())));
        }
    }

    void cancelRpcRequest(RpcRequest rpcRequest) throws Exception {
        if (!rpcRequest.controller.failed()) {
            throw new Exception(
                    "The controller must be set as failed when being canceled.");
        }
        ExecutorService threadPool = this.rpcClient.getThreadPool();
        final RpcRequest request = rpcRequest;
        threadPool.submit(new Runnable() {
            public void run() {
                request.done.run(request.response);
            }
        });
    }

    long getCurrentTimestamp() {
        if(this.latestTimestamp == 0) {
            this.latestTimestamp = System.currentTimeMillis();
        }
        return this.latestTimestamp;
    }
}
