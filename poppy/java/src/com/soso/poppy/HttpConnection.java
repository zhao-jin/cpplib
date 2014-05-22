/*
 * @(#)HttpConnection.java
 *
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

import java.net.InetSocketAddress;

import org.jboss.netty.buffer.ChannelBuffer;
import org.jboss.netty.buffer.ChannelBuffers;
import org.jboss.netty.channel.ChannelHandlerContext;
import org.jboss.netty.channel.ChannelStateEvent;
import org.jboss.netty.channel.SimpleChannelHandler;
import org.jboss.netty.channel.Channel;

class HttpConnection extends SimpleChannelHandler {
    protected HttpHandler httpHandler;
    protected boolean headerReceived = false;
    protected Channel nettyChannel = null;
    protected HttpRequest httpRequest = new HttpRequest();
    protected HttpResponse httpResponse = new HttpResponse();
    private InetSocketAddress remoteAddress;

    HttpConnection(HttpHandler handler) {
        this.httpHandler = handler;
    }

    void setChannel(Channel channel) {
        this.nettyChannel = channel;
    }

    int getConnectionId() {
        return this.nettyChannel.getId();
    }

    HttpRequest getHttpRequest() {
        return this.httpRequest;
    }

    HttpResponse getHttpResponse() {
        return this.httpResponse;
    }

    // Close the connection.
    void close(boolean immidiate) {
        this.headerReceived = false;
        this.nettyChannel.close();
    }

    boolean sendPacket(byte[] data) {
        ChannelBuffer channelBuffer = ChannelBuffers.buffer(data.length);
        channelBuffer.writeBytes(data);
        return sendPacket(channelBuffer);
    }

    boolean sendPacket(ChannelBuffer channelBuffer) {
        this.nettyChannel.write(channelBuffer);
        return true;
    }

    @Override
    public void channelConnected(ChannelHandlerContext ctx, ChannelStateEvent e) {
        // change status.
        this.httpHandler.onConnected(this);
    }

    @Override
    public void channelClosed(ChannelHandlerContext ctx, ChannelStateEvent e)
            throws Exception {
        super.channelClosed(ctx, e);
    }

    public void setRemoteAddress(InetSocketAddress address) {
        this.remoteAddress = address;
    }

    public InetSocketAddress getRemoteAddress() {
        return this.remoteAddress;
    }
}
