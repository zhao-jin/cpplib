/*
 * @(#)RpcClient.java
 *
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

import java.lang.RuntimeException;
import java.util.ArrayList;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import java.util.concurrent.TimeUnit;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Timer;
import java.util.TreeMap;

import org.jboss.netty.channel.socket.nio.NioClientSocketChannelFactory;

public class RpcClient {
    final int CHANNEL_CACHE_LIMIT = 100;
    private boolean started;
    private Object mutex;
    private Timer timerManager;
    private ExecutorService threadPool; // Thread pool to run callbacks.
    private List<RpcChannelImpl> channelList;
    private List<RpcChannelImpl> channelCache;
    // mapping from hash value of addresses to channel impl.
    Map<Integer, RpcChannelImpl> channelMap;
    private NioClientSocketChannelFactory m_netty_channel_factory;
    // Address resolver.
    private AddressPublisher addressPublisher;

    public RpcClient() {
        super();
        this.started = false;
        this.mutex = new Object();
        this.channelList = new ArrayList<RpcChannelImpl>();
        this.channelCache = new ArrayList<RpcChannelImpl>();
        this.channelMap = new TreeMap<Integer, RpcChannelImpl>();
        this.addressPublisher = new AddressPublisher();
    }

    public void start() {
        if (!this.started) {
            this.timerManager = new Timer();
            this.threadPool = Executors.newFixedThreadPool(4);

            m_netty_channel_factory = new NioClientSocketChannelFactory(
                    Executors.newCachedThreadPool(),
                    Executors.newCachedThreadPool());

            this.started = true;
        }
    }

    public void stop() throws Exception {
        if (this.started) {
            for (RpcChannelImpl impl : this.channelList) {
                impl.close();
            }

            for (RpcChannelImpl impl : this.channelCache) {
                impl.close();
            }

            m_netty_channel_factory.releaseExternalResources();
            m_netty_channel_factory = null;

            this.threadPool.shutdown();
            this.threadPool.awaitTermination(3, TimeUnit.SECONDS);
            this.threadPool = null;

            this.timerManager.cancel();
            this.timerManager.purge();
            this.timerManager = null;

            this.channelList.clear();
            this.channelCache.clear();
            this.channelMap.clear();

            this.started = false;
        }
    }

    Timer getTimerManager() {
        return this.timerManager;
    }

    ExecutorService getThreadPool() {
        return this.threadPool;
    }

    NioClientSocketChannelFactory getNettyChannelFactory() {
        return m_netty_channel_factory;
    }

    RpcChannelImpl getChannelImpl(String name) throws RuntimeException {
        // Calculate hash value based on service name.
        RpcChannelImpl impl = null;
        List<String> servers = new ArrayList<String>();
        int hashValue = name.hashCode();
        synchronized (this.mutex) {
            if (this.channelMap.containsKey(hashValue)) {
                impl = this.channelMap.get(hashValue);
            } else if (!this.addressPublisher.resolveAddress(name, servers)) {
                throw new RuntimeException("fail to resolve address: " + name);
            } else {
                impl = new RpcChannelImpl(this, name, servers);
                this.addressPublisher.subscribe(name, impl);
                this.channelMap.put(hashValue, impl);
            }

            // if the impl object is in using list, move it to the end of list.
            if (this.channelList.remove(impl)) {
                this.channelList.add(impl);
                return impl;
            }

            // if it's in channel cache, move it back into in using list.
            this.channelCache.remove(impl);

            this.channelList.add(impl);

            int size = this.channelCache.size();
            Iterator<RpcChannelImpl> iterator = this.channelCache.iterator();
            while (size > CHANNEL_CACHE_LIMIT && iterator.hasNext()) {
                RpcChannelImpl current = iterator.next();
                if (current.getConnectStatus() == RpcConnection.ConnectStatus.Disconnected) {
                    this.addressPublisher.unsubscribe(name, current);
                    iterator.remove();
                    --size;
                }
            }
        }
        return impl;
    }

}
