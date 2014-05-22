/*
 * @(#)EchoClient.java
 *
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.rpc_examples;

import java.lang.StringBuilder;
import java.util.ArrayList;
import java.util.concurrent.atomic.AtomicLong;
import java.util.List;
import org.apache.log4j.Logger;
import com.soso.poppy.*;

public class EchoClient {
    static Logger logger = Logger.getLogger("EchoClient");
    String serverName;
    RpcClient rpcClient;
    RpcChannel rpcChannel;
    AtomicLong requestCount = new AtomicLong();

    EchoClient(String server_name) {
        this.serverName = server_name;
        this.rpcClient = new RpcClient();
        this.rpcChannel = new RpcChannel(this.rpcClient, this.serverName);
    }

    long runLatencyTest(int round) {
        EchoService.EchoServer.BlockingInterface echo_client = EchoService.EchoServer
                .newBlockingStub(this.rpcChannel);
        long t = System.currentTimeMillis();
        for (int i = 0; i < round; ++i) {
            RpcController controller = new RpcController();
            EchoService.EchoRequest request = EchoService.EchoRequest.newBuilder().setUser(
                    Integer.toString(i)).setMessage("t").build();
            try {
                echo_client.echo(controller, request);
                if (controller.failed()) {
                    logger.fatal("failed: " + controller.errorText());
                } else {
                }
            } catch (com.google.protobuf.ServiceException e) {
                logger.fatal("failed: " + e.getMessage());
            }
        }
        t = System.currentTimeMillis() - t;
        long throughput = (long) (round / (t / 1000.0));
        return throughput;
    }

    long runReadTest(int round) {
        EchoService.EchoServer.BlockingInterface echo_client = EchoService.EchoServer
                .newBlockingStub(this.rpcChannel);
        long t = System.currentTimeMillis();
        for (int i = 0; i < round; ++i) {
            RpcController controller = new RpcController();
            EchoService.EchoRequest request = EchoService.EchoRequest.newBuilder().setUser("t")
                    .setMessage("t").build();
            try {
                echo_client.get(controller, request);
                if (controller.failed()) {
                    logger.fatal("failed: " + controller.errorText());
                }
            } catch (com.google.protobuf.ServiceException e) {
                logger.fatal("failed: " + e.getMessage());
            }
        }
        t = System.currentTimeMillis() - t;
        long throughput = (long) (round / (t / 1000.0));
        return throughput;
    }

    private String buildString(String str, int times) {
        StringBuilder sb = new StringBuilder(times);
        for (int k = 0; k < times; ++k) {
            sb.append(str);
        }
        return sb.toString();
    }

    long runWriteTest(int round) {
        EchoService.EchoServer.BlockingInterface echo_client = EchoService.EchoServer
                .newBlockingStub(this.rpcChannel);
        long t = System.currentTimeMillis();
        for (int i = 0; i < round; ++i) {
            RpcController controller = new RpcController();
            EchoService.EchoRequest request = EchoService.EchoRequest.newBuilder().setUser("t")
                    .setMessage(buildString("A", 1000 * 1024)).build();

            try {
                echo_client.set(controller, request);
                if (controller.failed()) {
                    logger.fatal("failed: " + controller.errorText());
                }
            } catch (com.google.protobuf.ServiceException e) {
                logger.fatal("failed: " + e.getMessage());
            }
        }
        t = System.currentTimeMillis() - t;
        long throughput = (long) (round / (t / 1000.0));
        return throughput;
    }

    long runReadWriteTest(int round) {
        EchoService.EchoServer.BlockingInterface echo_client = EchoService.EchoServer
                .newBlockingStub(this.rpcChannel);
        long t = System.currentTimeMillis();
        for (int i = 0; i < round; ++i) {
            RpcController controller = new RpcController();
            EchoService.EchoRequest request = EchoService.EchoRequest.newBuilder().setUser("t")
                    .setMessage(buildString("A", 1000 * 1024)).build();

            try {
                echo_client.echo(controller, request);
                if (controller.failed()) {
                    logger.fatal("failed: " + controller.errorText());
                }
            } catch (com.google.protobuf.ServiceException e) {
                logger.fatal("failed: " + e.getMessage());
            }
        }
        t = System.currentTimeMillis() - t;
        long throughput = 2 * (long) (round / (t / 1000.0));
        return throughput;
    }

    long runAsyncTest(int round) {
        long t = System.currentTimeMillis();
        sendAsyncTestRequest(round);
        waitForAsyncTestFinish();
        t = System.currentTimeMillis() - t;
        long throughput = (long) (round / (t / 1000.0));
        return throughput;
    }

    int sendAsyncTestRequest(int round) {
        EchoService.EchoServer.Stub echo_client = EchoService.EchoServer
                .newStub(this.rpcChannel);
        this.requestCount.set(0);
        for (int i = 0; i < round; ++i) {
            RpcController controller = new RpcController();
            EchoService.EchoRequest.Builder build = EchoService.EchoRequest.newBuilder().setUser("t");

            final RpcController ctrl = controller;
            com.google.protobuf.RpcCallback<EchoService.EchoResponse> done = new com.google.protobuf.RpcCallback<EchoService.EchoResponse>() {
                public void run(EchoService.EchoResponse resp) {
                    if (ctrl.failed()) {
                        logger.error("failed: ".concat(ctrl.errorText()));
                    }
                    requestCount.decrementAndGet();
                }
            };

            switch (i % 4) {
            case 0:
                echo_client.echo(controller, build.setMessage("t").build(),
                        done);
                break;
            case 1:
                echo_client
                        .get(controller, build.setMessage("t").build(), done);
                break;
            case 2:
                echo_client.set(controller, build.setMessage(
                        buildString("a", 10)).build(), done);
                break;
            case 3:
                echo_client.echo(controller, build.setMessage(
                        buildString("a", 10)).build(), done);
                break;
            }
            if (i % 100 == 0) {
                try {
                    java.lang.Thread.sleep(1000);
                } catch (Exception e) {
                }
            }
            this.requestCount.incrementAndGet();
        }
        return 0;
    }

    int waitForAsyncTestFinish() {
        for (;;) {
            try {
                java.lang.Thread.sleep(100);
                if (this.requestCount.get() == 0) {
                    break;
                }
            } catch (Exception e) {
            }
        }
        return 0;
    }
}
