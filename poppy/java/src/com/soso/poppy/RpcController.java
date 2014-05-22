/*
 * @(#)RpcController.java
 *
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

import java.net.InetSocketAddress;

import com.google.protobuf.RpcCallback;

public class RpcController implements com.google.protobuf.RpcController {
    private boolean failed = false;
    private String reason = "";

    private int connectionId = -1;
    private long sequenceId = -1;
    private int errorCode = -1;
    private boolean sync = false;
    private InetSocketAddress remoteAddress;
    private int timeout = -1;

    public void notifyOnCancel(RpcCallback<Object> callback) {
        throw new IllegalStateException("Serverside use only.");
    }

    public void reset() {
        this.connectionId = -1;
        this.sequenceId = -1;
        this.errorCode = -1;
        this.failed = false;
        this.reason = "";
        this.sync = false;
        this.timeout = -1;
    }

    public void startCancel() {
    }

    public boolean isCanceled() {
        throw new IllegalStateException("Serverside use only.");
    }

    public void setFailed(String reason) {
        this.reason = reason;
        this.failed = true;
    }

    public String errorText() {
        return this.reason;
    }

    public boolean failed() {
        return this.failed;
    }

    int getConnectionId() {
        return this.connectionId;
    }

    void setConnectionId(int connectionId) {
        this.connectionId = connectionId;
    }

    long getSequenceId() {
        return this.sequenceId;
    }

    void setSequenceId(long sequenceId) {
        this.sequenceId = sequenceId;
    }

    // TODO(hansye): Define and implement error code.
    int getErrorCode() {
        return this.errorCode;
    }

    void setErrorCode(int errorCode) {
        this.errorCode = errorCode;
    }

    // sync or async call.
    void setSync(boolean sync) {
        this.sync = sync;
    }

    boolean isSync() {
        return this.sync;
    }

    void setRemoteAddress(InetSocketAddress address) {
        this.remoteAddress = address;
    }

    public InetSocketAddress getRemoteAddress() {
        return this.remoteAddress;
    }

    @Deprecated
    public InetSocketAddress getSocketAddress() {
        return this.remoteAddress;
    }

    public int getTimeout() {
        return this.timeout;
    }

    public void setTimeout(int timeous_ms) {
        this.timeout = timeous_ms;
    }
}
