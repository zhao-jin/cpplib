/*
 * @(#)RpcChannel.java
 *
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

import java.lang.RuntimeException;

public class RpcChannel implements com.google.protobuf.RpcChannel,
        com.google.protobuf.BlockingRpcChannel {
    protected RpcChannelImpl impl;

    public RpcChannel(RpcClient rpcClient, String serverName) throws RuntimeException {
        this.impl = rpcClient.getChannelImpl(serverName);
    }

    /*
     * Implements the com.google.protobuf.RpcChannel interface. If the done is
     * NULL, it's a synchronous call, or it's an asynchronous and uses the
     * callback inform the completion (or failure). It's only called by user's
     * thread.
     */
    public void callMethod(
            final com.google.protobuf.Descriptors.MethodDescriptor method,
            com.google.protobuf.RpcController controller,
            final com.google.protobuf.Message request,
            com.google.protobuf.Message responsePrototype,
            com.google.protobuf.RpcCallback<com.google.protobuf.Message> done) {
        RpcController ctrl = (RpcController)controller;
        ctrl.setSync(true);
        this.impl.callMethod(method, controller, request,
                responsePrototype, done);
    }

    public com.google.protobuf.Message callBlockingMethod(
            final com.google.protobuf.Descriptors.MethodDescriptor method,
            com.google.protobuf.RpcController controller,
            com.google.protobuf.Message request,
            com.google.protobuf.Message responsePrototype)
            throws com.google.protobuf.ServiceException {

        PoppyRpcCallback callback = new PoppyRpcCallback();

        callMethod(method, controller, request, responsePrototype, callback);
        if (!callback.isDone()) {
            synchronized (callback) {
                while (!callback.isDone()) {
                    try {
                        callback.wait();
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                    }
                }
            }
        }
        if (controller.failed()) {
            throw new com.google.protobuf.ServiceException(
                    controller.errorText());
        }
        return callback.getMessage();
    }
}
