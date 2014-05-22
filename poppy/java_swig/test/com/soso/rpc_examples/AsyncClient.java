/*
 * @(#)AsyncClient.java
 *
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.rpc_examples;

import java.util.concurrent.atomic.AtomicLong;
import org.apache.log4j.Logger;
import com.soso.poppy.*;

class EchoCallback implements com.google.protobuf.RpcCallback<EchoService.EchoResponse> {
    static Logger logger = Logger.getLogger("EchoCallback");
    AsyncClient client;
    RpcController controller;

    EchoCallback(AsyncClient client, RpcController controller) {
        this.client = client;
        this.controller = controller;
    }

    public void run(EchoService.EchoResponse resp) {
        if (this.controller.failed()) {
            logger.error("failed: ".concat(this.controller.errorText()));
        }
        client.requestCount.decrementAndGet();
        client.runTest(1);
    }
}

public class AsyncClient {
    static Logger logger = Logger.getLogger("AsyncClient");
    String serverName;
    RpcClient rpcClient;
    RpcChannel rpcChannel;
    AtomicLong requestCount;
    long limit = 100;
    EchoService.EchoServer.Stub stub;

    AsyncClient(String server_name, long limit) {
        this.serverName = server_name;
        this.rpcClient = new RpcClient();
        this.rpcChannel = new RpcChannel(this.rpcClient, this.serverName);
        this.limit = limit;
        this.stub = EchoService.EchoServer.newStub(this.rpcChannel);
        this.requestCount = new AtomicLong();
    }

    void runTest() {
        runTest(this.limit);
    }

    void runTest(long limit) {
        for (long i = 0; i < limit; ++i) {
            RpcController controller = new RpcController();
            EchoService.EchoRequest.Builder build =
                EchoService.EchoRequest.newBuilder().setUser("t");

            final RpcController ctrl = controller;
            com.google.protobuf.RpcCallback<EchoService.EchoResponse> done =
                new com.google.protobuf.RpcCallback<EchoService.EchoResponse>() {
                public void run(EchoService.EchoResponse resp) {
                    if (ctrl.failed()) {
                        logger.error("failed: ".concat(ctrl.errorText()));
                    }
                    requestCount.decrementAndGet();
                    runTest(1);
                }
            };

            stub.echo(controller, build.setMessage("t").build(),done);
            requestCount.incrementAndGet();
        }
    }

    public static void main(String[] argv) {
        if (argv.length < 2) {
            System.out.println("usage: ");
            return;
        }
        AsyncClient client = new AsyncClient(argv[0], Long.parseLong(argv[1]));
        client.runTest();
        try {
            while(true) {
                Thread.sleep(10);
            }
        } catch (Exception e) {
        }
    }
}
