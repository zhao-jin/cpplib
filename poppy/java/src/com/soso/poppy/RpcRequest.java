/*
 * @(#)RpcRequest.java	
 * 
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

/*
 * All data about one rpc request.
 * We don't use closure as we need to re-schedule a rpc request to another
 * session, and then can't bind rpc request to a session tightly.
 */
public class RpcRequest {
    // The id of the request, which is unique within the session pool.
    long sequenceId;
    // The expected time when the request should finish, in microseconds.
    long expectedTimeout;
    final com.google.protobuf.Descriptors.MethodDescriptor method;
    RpcController controller;
    final com.google.protobuf.Message request;
    com.google.protobuf.Message response;
    com.google.protobuf.RpcCallback<com.google.protobuf.Message> done;

    RpcRequest(long sequenceId, 
            long expectedTimeout,
            final com.google.protobuf.Descriptors.MethodDescriptor method,
            RpcController controller,
            final com.google.protobuf.Message request,
            com.google.protobuf.Message response,
            com.google.protobuf.RpcCallback<com.google.protobuf.Message> done) {
        this.sequenceId = sequenceId;
        this.expectedTimeout = expectedTimeout;
        this.method = method;
        this.controller = controller;
        this.request = request;
        this.response = response;
        this.done = done;
        this.controller.setSequenceId(sequenceId);
    }
}
