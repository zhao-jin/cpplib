/*
 * @(#)RpcConnectionGroup.java	
 * 
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.TreeMap;
import java.util.Random;
import java.util.concurrent.atomic.AtomicLong;

/*
 * RPC Connection Group. It's main task is to maintain status for all HTTP
 * connections in the same channel.
 */
public class RpcConnectionGroup {
    class RpcConnectionInfo {
        HttpConnection httpConnection;
        int connectStatus;
        String serverAddress; // target address. ip:port
        long connectionId; // connection id. socket
        AtomicLong requestCount; // request sent to this connection
        AtomicLong responseCount; // response get from this connection
        AtomicLong cancelCount; // canceled request count
        // request count canceled because of timeout
        AtomicLong timeoutCount;
        AtomicLong pendingCount; // request count not confirmed yet
        // record times of connecting failure
        // which will be reset after connected
        AtomicLong connectFailTimes;

        RpcConnectionInfo() {
            this.httpConnection = null;
            this.connectStatus = RpcConnection.ConnectStatus.Disconnected;
            this.connectionId = 0;
            this.requestCount = new AtomicLong();
            this.responseCount = new AtomicLong();
            this.cancelCount = new AtomicLong();
            this.timeoutCount = new AtomicLong();
            this.pendingCount = new AtomicLong();
            this.connectFailTimes = new AtomicLong();
        }

        boolean IsMoreIdleThan(final RpcConnectionInfo rhs) {
            return this.pendingCount.get() < rhs.pendingCount.get();
        }
    }

    List<RpcConnectionInfo> activeConnections;
    // The mutex to protect state
    Object mutex;
    TreeMap<Long, RpcConnectionInfo> connectionMap;
    TreeMap<String, RpcConnectionInfo> connectionCache;
    // for statistics purpose
    AtomicLong requestCount;
    AtomicLong responseCount;
    AtomicLong cancelCount;
    AtomicLong timeoutCount;
    AtomicLong pendingCount;

    Random randGenerator;
    
    public RpcConnectionGroup() {
        this.activeConnections = new ArrayList<RpcConnectionInfo>();
        this.mutex = new Object();
        this.connectionMap = new TreeMap<Long, RpcConnectionInfo>();
        this.connectionCache = new TreeMap<String, RpcConnectionInfo>();
        this.requestCount = new AtomicLong();
        this.responseCount = new AtomicLong();
        this.cancelCount = new AtomicLong();
        this.timeoutCount = new AtomicLong();
        this.pendingCount = new AtomicLong();
        this.randGenerator = new Random();
    }

    // add a new connection. this is called when a new http_connection has
    // just been created.
    boolean insertConnection(String server_address, long connection_id,
            HttpConnection http_connection) {
        synchronized (this.mutex) {
            RpcConnectionInfo info = this.connectionCache.get(server_address);
            if (info == null) {
                info = new RpcConnectionInfo();
                info.serverAddress = server_address;
                this.connectionCache.put(server_address, info);
            }
            info.httpConnection = http_connection;
            info.connectStatus = RpcConnection.ConnectStatus.Disconnected;
            info.connectionId = connection_id;
            this.connectionCache.put(server_address, info);
            this.connectionMap.put(connection_id, info);
        }
        return true;
    }

    // erase a connection. this is called when a connection has been closed
    boolean removeConnection(long connection_id) {
        synchronized (this.mutex) {
            RpcConnectionInfo info = this.connectionMap.get(connection_id);
            if (info == null) {
                return false;
            }
            info.connectionId = -1;
            info.httpConnection = null;
            info.connectStatus = RpcConnection.ConnectStatus.Disconnected;
            this.connectionMap.remove(connection_id);
        }
        return true;
    }

    HttpConnection getMostIdleConnection() {
        synchronized (this.mutex) {
            if (this.activeConnections.isEmpty()) {
                return null;
            }
            int size = this.activeConnections.size();
            int start = this.randGenerator.nextInt() % size;
            if (start < 0) {
                start += size;
            }
            RpcConnectionInfo info = this.activeConnections.get(start);

            for (int k = 1; k <= 3 && (start + k) % size != start; ++k) {
                if (this.activeConnections.get((start + k) % size)
                        .IsMoreIdleThan(info)) {
                    info = this.activeConnections.get((start + k) % size);
                }
            }
            return info.httpConnection;
        }
    }

    boolean changeStatus(long connection_id, int status) {
        synchronized (this.mutex) {
            RpcConnectionInfo info = this.connectionMap.get(connection_id);
            if (info == null) {
                return false;
            }
            switch (status) {
            case RpcConnection.ConnectStatus.Disconnecting:
            case RpcConnection.ConnectStatus.Disconnected:
            case RpcConnection.ConnectStatus.Failed: {
                if (info.connectStatus == RpcConnection.ConnectStatus.Connecting) {
                    info.connectFailTimes.incrementAndGet();
                }
                // remove this connection from active list
                this.activeConnections.remove(info);
            }
                break;
            case RpcConnection.ConnectStatus.Connected:
                if (info.connectStatus == RpcConnection.ConnectStatus.Connecting) {
                    info.connectFailTimes.set(0);
                }
                this.activeConnections.add(info);
                break;
            default:
                break;
            }
            info.connectStatus = status;
        }
        return true;
    }

    String getAddressFromConnectionId(long connection_id) {
        synchronized (this.mutex) {
            RpcConnectionInfo info = this.connectionMap.get(connection_id);
            if (info == null) {
                return null;
            }
            return info.serverAddress;
        }
    }

    long getConnectionIdFromAddress(String address) {
        synchronized (this.mutex) {
            RpcConnectionInfo info = this.connectionCache.get(address);
            if (info == null) {
                return -1;
            }
            return info.connectionId;
        }
    }

    boolean addRequest(long connection_id) {
        synchronized (this.mutex) {
            RpcConnectionInfo info = this.connectionMap.get(connection_id);
            if (info == null) {
                return false;
            }
            if (info.connectStatus != RpcConnection.ConnectStatus.Connected) {
                return false;
            }
            info.requestCount.incrementAndGet();
            info.pendingCount.incrementAndGet();
            this.requestCount.incrementAndGet();
            this.pendingCount.incrementAndGet();
            return true;
        }
    }

    boolean confirmRequest(long connection_id, int type) {
        synchronized (this.mutex) {
            RpcConnectionInfo info = this.connectionMap.get(connection_id);
            if (info == null) {
                return false;
            }
            if (info.connectStatus != RpcConnection.ConnectStatus.Connected) {
                return false;
            }
            switch (type) {
            case RpcConnection.RequestConfirmType.Response:
                info.responseCount.incrementAndGet();
                this.responseCount.incrementAndGet();
                break;
            case RpcConnection.RequestConfirmType.Canceled:
                info.cancelCount.incrementAndGet();
                this.cancelCount.incrementAndGet();
                break;
            case RpcConnection.RequestConfirmType.Timeout:
                info.timeoutCount.incrementAndGet();
                this.timeoutCount.incrementAndGet();
                break;
            default:
                return false;
            }
            info.pendingCount.decrementAndGet();
            this.pendingCount.decrementAndGet();
            return true;
        }
    }

    void closeAllSessions() {
        synchronized (this.mutex) {
            // NOTE: cannot use this.connectionMap.values() in for loop directly.
            // the call to netty::close() will return directly, so the onClosed to
            // RpcChannelImpl::onClosed is called by main thread, and so on the
            // related connection info entry is removed.
            // unfortunately, the lock in java is reentrantlock, so a
            // ConcurrentModificationException will be thrown.
            List<RpcConnectionInfo> http_connections = new ArrayList<RpcConnectionInfo>();
            http_connections.addAll(this.connectionMap.values());
            for (RpcConnectionInfo info : http_connections) {
                info.httpConnection.close(true);
            }
        }
    }

    boolean isAvailable() {
        synchronized (this.mutex) {
            return !this.connectionMap.isEmpty();
        }
    }

    boolean isHealthy(long connection_id) {
        synchronized (this.mutex) {
            RpcConnectionInfo info = this.connectionMap.get(connection_id);
            if (info == null) {
                return false;
            }
            return info.connectStatus != RpcConnection.ConnectStatus.Failed;
        }
    }

    boolean noPendingRequest() {
        synchronized (this.mutex) {
            return this.pendingCount.get() == 0;
        }
    }

    String dump() {
        StringBuilder sb = new StringBuilder();
        sb.append("Total request count: ").append(this.requestCount).append("\n");
        sb.append("\tresponse count: ").append(this.responseCount).append("\n");
        sb.append("\tcanceled count: ").append(this.cancelCount).append("\n");
        sb.append("\ttime out count: ").append(this.timeoutCount).append("\n");
        return sb.toString();
    }

}
