/*
 * @(#)AsyncClient.java
 *
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.rpc_examples;

public class AsyncClient {
    public static void main(String[] argv) {
        if (argv.length < 2) {
            System.out.println("usage: ");
            return;
        }
        EchoClient client = new EchoClient(argv[0]);
        client.start();
        long throughput = client.runAsyncTest(Integer.parseInt(argv[1]));
        System.out.println("finished with throughput: "
                .concat(Long.toString(throughput)));
        client.stop();
    }
}
