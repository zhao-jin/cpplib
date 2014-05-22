/*
 * @(#)RpcController.java
 *
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

import java.net.InetSocketAddress;

import com.google.protobuf.RpcCallback;

public class RpcController
        extends com.soso.poppy.swig.RpcControllerSwig
        implements com.google.protobuf.RpcController {

    public RpcController() {
        super();
    }

    public String errorText() {
        return ErrorText();
    }

    public boolean failed() {
        return Failed();
    }

    public void setFailed(RpcErrorCodeInfo.RpcErrorCode errorCode) {
        SetFailed(errorCode.getNumber());
    }

    public int errorCode() {
        return ErrorCode();
    }

    public void setTimeout(long timeout) {
        SetTimeout(timeout);
    }

    public void setRequestCompressType(RpcOption.CompressType compressType) {
        use_default_request_compress_type = false;
        SetRequestCompressType(compressType.getNumber());
    }

    public void setResponseCompressType(RpcOption.CompressType compressType) {
        use_default_response_compress_type = false;
        SetResponseCompressType(compressType.getNumber());
    }

    public boolean isCanceled() {
        throw new IllegalStateException("Serverside use only.");
    }

    public void notifyOnCancel(RpcCallback<Object> callback) {
        throw new IllegalStateException("Serverside use only.");
    }

    public void reset() {
        Reset();
    }

    public void setFailed(String reason) {
        throw new IllegalStateException("Serverside use only.");
    }

    public void startCancel() {
        throw new IllegalStateException("Not be implemented.");
    }

    public boolean use_default_request_compress_type = true;

    public boolean use_default_response_compress_type = true;
    public InetSocketAddress getRemoteAddress() {
        String address = RemoteAddressString();
        String[] addr = address.split(":");
        return new InetSocketAddress(addr[0], Integer.parseInt(addr[1]));
    }

    @Deprecated
    public InetSocketAddress getSocketAddress() {
        return getRemoteAddress();
    }
}
