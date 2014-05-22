/*
 * @(#)SyncClient.java
 *
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.rpc_examples;

public class SyncClient {
    public static void main(String[] argv) {
        EchoClient client = new EchoClient(argv[0]);
        client.start();
        long throughput = client.runLatencyTest(Integer.parseInt(argv[1]));
        System.out.println("finished with throughput: ".concat(Long.toString(throughput)));
        client.stop();
    }
}
