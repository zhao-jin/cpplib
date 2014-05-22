/*
 * @(#)PoppyRpcCallback.java	
 * 
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

import com.google.protobuf.RpcCallback;
import com.google.protobuf.Message;

public class PoppyRpcCallback implements RpcCallback<Message> {

    private boolean done = false;
    private Message message;

    public void run(Message message) {
        this.message = message;
        synchronized (this) {
            done = true;
            notify();
        }
    }

    public Message getMessage() {
        return message;
    }

    public boolean isDone() {
        return done;
    }
}
