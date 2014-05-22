/*
 * @(#)RpcClientHandler.java
 *
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

import java.nio.ByteBuffer;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.ByteString;

public class RpcClientHandler extends HttpHandler {
    /*
     * Rpc client doesn't own a session pool.
     * Session pool belongs to the rpc channel.
     */
    private RpcChannelImpl channelImpl;

    public RpcClientHandler() {
    }

    public void set_session_pool(RpcChannelImpl impl) {
        this.channelImpl = impl;
    }

    final static String RPC_HTTP_PATH = "/__rpc_service__";
    final static int RPC_HEADER_SIZE = 8;

    // Implements HttpHandler interface.
    public void onConnected(HttpConnection httpConnection) {
        StringBuilder sb = new StringBuilder();
        sb.append("POST ");
        sb.append(RPC_HTTP_PATH);
        sb.append(" HTTP/1.1\r\n\r\n");
        String request = sb.toString();
        if (!httpConnection.sendPacket(request.getBytes())) {
            // Failed to send packet, close the connection.
            httpConnection.close(true);
        }
    }

    public void onClose(HttpConnection httpConnection, int errorCode) {
        this.channelImpl.onClose(
                httpConnection.getConnectionId(),
                errorCode);
    }

    public void handleHeaders(HttpConnection httpConnection) {
        HttpResponse httpResponse = httpConnection.getHttpResponse();
        // Check if connected successfully.
        if (httpResponse.getStatus() != 200) {
            httpConnection.close(true);
            return;
        }
        this.channelImpl.onNewSession(httpConnection);
    }

    public void handleBodyPacket(HttpConnection httpConnection, byte[] data,
            int offset) {
        ByteBuffer bb = ByteBuffer.allocate(RPC_HEADER_SIZE);
        bb.put(data, offset, RPC_HEADER_SIZE);
        bb.position(0);
        int metaLength = bb.getInt();
        int messageLength = bb.getInt();

        ByteString bs = ByteString.copyFrom(
                data, offset + RPC_HEADER_SIZE, metaLength);
        RpcMetaInfo.RpcMeta rpc_meta = null;
        try {
            rpc_meta = RpcMetaInfo.RpcMeta.parseFrom(bs);
        } catch (InvalidProtocolBufferException e) {
            return;
        }

        if (rpc_meta.getSequenceId() < 0) {
            // Sequence id is invalid, unexpected, close the connection.
            // LOG(WARNING) << "Invalid rpc sequence id:" <<
            // rpc_meta.sequence_id();
            httpConnection.close(true);
            return;
        }
        bs = ByteString.copyFrom(
                data, offset + RPC_HEADER_SIZE + metaLength, messageLength);
        this.channelImpl.handleResponse(rpc_meta, bs.toByteArray());
    }

    public int detectBodyPacketSize(HttpConnection httpConnection,
            byte[] data, int offset) {
        if (data.length < RPC_HEADER_SIZE) {
            return 0;
        }

        ByteBuffer bb = ByteBuffer.allocate(RPC_HEADER_SIZE);
        bb.put(data, offset, RPC_HEADER_SIZE);
        bb.position(0);
        int meta_length = bb.getInt();
        int message_length = bb.getInt();

        // total_length = 4 * 2 + meta_length + message_length
        return RPC_HEADER_SIZE + meta_length + message_length;
    }

    public void setChannelImpl(RpcChannelImpl impl) {
        this.channelImpl = impl;
    }
}
