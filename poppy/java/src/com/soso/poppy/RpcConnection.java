/*
 * @(#)RpcConnection.java	
 * 
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

public class RpcConnection {
    public class ConnectStatus {
        public final static int Disconnected = 0;
        public final static int Connecting = 1;
        public final static int Connected = 2;
        public final static int Disconnecting = 3;
        public final static int Failed = 4;
    }

    public class RequestConfirmType {
        public final static int Response = 0;
        public final static int Canceled = 1;
        public final static int Timeout = 2;
    }
}
