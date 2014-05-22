package com.soso.rpc_examples;

import java.io.IOException;
import java.lang.Process;
import java.lang.Runtime;
import java.util.Map;
import java.util.TreeMap;
import org.apache.log4j.Logger;

import junit.framework.*;

class TestServer {
    Process process;
    String command;

    TestServer(String address) {
        this.command = "./echoserver --alsologtostderr --server_address " + address;
    }
    boolean start() {
        try {
            process = Runtime.getRuntime().exec(this.command);
            return true;
        } catch (IOException e) {
            e.printStackTrace();
            return false;
        }
    }
    boolean stop() {
        if (process != null) {
            process.destroy();
            process = null;
            return true;
        }
        return false;
    }
}

public class PoppyTest extends TestCase {
    static private Logger logger = Logger.getLogger("PoppyTest");
    static final int SERVER_PORT_FROM = 60000;
    static final int SERVER_NUMBER = 2;
    Map<Integer, TestServer> servers;

    public PoppyTest(String name) {
        super(name);
        servers = new TreeMap<Integer, TestServer>();
    }

    void startServer(int index) {
        if (servers.containsKey(index)) {
            TestServer server = servers.get(index);
            server.stop();
            servers.remove(index);
        }
        String address = getServerAddress(index);
        if (address != null) {
            TestServer server = new TestServer(address);
            servers.put(index, server);
            server.start();
        }
    }

    void stopServer(int index) {
        if (servers.containsKey(index)) {
            TestServer server = servers.get(index);
            server.stop();
            servers.remove(index);
        }
    }

    String getServerAddress(int index) {
        String address = null;
        if (index < 0 || index >= SERVER_NUMBER) {
            return address;
        }
        int port = SERVER_PORT_FROM + index;
        address = "127.0.0.1:".concat(Integer.toString(port));
        return address;
    }

    @Override
    protected void setUp() {
        for (int i = 0; i < SERVER_NUMBER; ++i) {
            startServer(i);
        }
    }

    @Override
    protected void tearDown() {
        for (int i = 0; i < SERVER_NUMBER; ++i) {
            stopServer(i);
        }
    }

    public void testBasic() {
        EchoClient client = new EchoClient(getServerAddress(0));
        client.start();
        long throughput = 0;
        String message;
        throughput = client.runLatencyTest(100);
        message = String.format("Latency test: %d/S", throughput);
        logger.info(message);

        throughput = client.runReadTest(500);
        message = String.format("Read throughput: %d/S", throughput);
        logger.info(message);

        throughput = client.runWriteTest(500);
        message = String.format("Write throughput: %d/S", throughput);
        logger.info(message);

        throughput = client.runReadWriteTest(500);
        message = String.format("Read/Write throughput: %d/S", throughput);
        logger.info(message);

        client.runAsyncTest(100);
        client.stop();
    }

    public static Test suite() {
        return new TestSuite(PoppyTest.class);
    }

    public static void main(String args[]) {
        junit.textui.TestRunner.run(suite());
    }
}
