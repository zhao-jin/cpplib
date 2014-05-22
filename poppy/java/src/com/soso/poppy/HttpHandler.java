/*
 * @(#)HttpHandler.java	
 * 
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

public abstract class HttpHandler {
    public HttpHandler() {
    }

    // Note: onConnected is only called at client side.
    public void onConnected(HttpConnection connection) {
    }

    public void onClose(HttpConnection connection, int error_code) {
    }

    // Handle a comlete message.
    public void handleMessage(HttpConnection http_connection) {
    }

    // The interface the derived class needs to implement.

    // Handle headers. It's called when http header received.
    public void handleHeaders(HttpConnection http_connection) { }

    // Handle a body packet. It's called when a body packet received.
    public void handleBodyPacket(HttpConnection http_connection, 
            byte[] data, 
            int offset) { }

    // Dectect the body packet size. If unknown, 0 is returned.
    public int detectBodyPacketSize(HttpConnection http_connection,
            byte[] data, 
            int offset) {
        return 0;
    };
}
