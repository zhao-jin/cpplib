/*
 * @(#)HttpClientConnection.java	
 * 
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

import java.nio.ByteBuffer;
import org.apache.log4j.Logger;
import org.jboss.netty.buffer.ChannelBuffer;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.ChannelStateEvent;
import org.jboss.netty.channel.ExceptionEvent;
import org.jboss.netty.channel.MessageEvent;

public class HttpClientConnection extends HttpConnection {
    static private Logger logger = Logger.getLogger("HttpClientConnection");
    private ByteBuffer byteBuffer = null;
    private int packageSize = 0;

    public HttpClientConnection(HttpHandler httpHandler) {
        super(httpHandler);
    }

    @Override
    public void messageReceived(ChannelHandlerContext ctx, MessageEvent e) {
        // parse the package, and call the callback if necessary.
        ChannelBuffer cb = (ChannelBuffer) e.getMessage();
        while (cb.readable()) {
            if (this.headerReceived == false) {
                handleMessage(cb);
            } else {
                handleBodyPacket(cb);
            }
        }
    }

    @Override
    public void channelClosed(ChannelHandlerContext ctx, ChannelStateEvent e)
            throws Exception {
        this.httpHandler.onClose(this, 0);
        super.channelClosed(ctx, e);
    }

    @Override
    public void exceptionCaught(ChannelHandlerContext ctx, ExceptionEvent e) {
        // Close the connection when an exception is raised.
        logger.warn("Unexpected exception from downstream.");
        ctx.getChannel().close();
    }

    private void handleMessage(ChannelBuffer cb) {
        boolean messageCompleted = false;
        assert (this.headerReceived == false);

        this.headerReceived = true;

        int length = cb.readableBytes();
        byte[] header = new byte[length];
        cb.readBytes(header);

        this.httpResponse.reset();
        if (this.httpResponse.parseHeaders(new String(header)) 
                != HttpMessage.ErrorType.ERROR_NORMAL) {
            // (TODO) handle parser error.
            logger.warn("Failed to parse the http header.");
            close(true);
            return;
        }
        this.httpHandler.handleHeaders(this);

        // Check if the message is complete
        String value = null;
        if (this.httpResponse.getStatus() < 200 
                || this.httpResponse.getStatus() == 204
                || this.httpResponse.getStatus() == 304) {
            messageCompleted = true;
        } else if (this.httpResponse.getHeader("Content-Length", value)) {
            int contentLength = Integer.parseInt(value);
            if (contentLength == 0) {
                messageCompleted = true;
            }
        }
        if (messageCompleted == true) {
            this.headerReceived = false;
            this.httpHandler.handleMessage(this);
        }
    }

    private void handleBodyPacket(ChannelBuffer cb) {
        assert (this.headerReceived == true);

        if (cb.readableBytes() < RpcClientHandler.RPC_HEADER_SIZE) {
            return;
        }

        boolean messageCompleted = false;
        // if m_byte_buffer is null, try to allocate one.
        if (byteBuffer == null) {
            packageSize = 
                    this.httpHandler.detectBodyPacketSize(this, cb.array(), cb.readerIndex());
            if (packageSize > 0) {
                if (cb.readableBytes() >= packageSize) {
                    this.httpHandler.handleBodyPacket(
                            this, 
                            cb.array(), 
                            cb.readerIndex());
                    cb.skipBytes(packageSize);
                    packageSize = 0;
                    return;
                } else {
                    byteBuffer = ByteBuffer.allocate(packageSize);
                }
            } else {
                // TODO
                return;
            }
        }
        int length = Math.min(packageSize - byteBuffer.position(), cb.readableBytes());
        byteBuffer.put(cb.array(), cb.readerIndex(), length);
        // move the reader index forward.
        cb.skipBytes(length);

        String value = null;
        if (this.httpResponse.getHeader("Transfer-Encoding", value)) {
            // (TODO) handle transfer encoding.
        } else if (this.httpResponse.getHeader("Content-Length", value)) {
            int contentLength = Integer.parseInt(value);
            if (contentLength == byteBuffer.position()) {
                messageCompleted = true;
            }
        }

        if (byteBuffer.position() == packageSize) {
            this.httpHandler.handleBodyPacket(this, byteBuffer.array(), 0);
            byteBuffer = null;
            packageSize = 0;
        }

        // Check if the message is complete
        if (messageCompleted) {
            this.headerReceived = false;
            this.httpHandler.handleMessage(this);
        }
    }
}
