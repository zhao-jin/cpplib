#!/usr/bin/env python
# -*- coding: cp936 -*-
#

import os
import sys
import time
import datetime
import threading
import gc
import profile
import pstats

# 内存泄露检测（比如：循环引用），开发阶段可以开启
def Enable_CheckMemoryLeak():
    gc.set_debug(gc.DEBUG_LEAK)

# 确认没有问题的情况下，发布时可以禁用gc来提高效率
def DisableGC():
    gc.disable()


# 性能profile
def ProfileMain():
    profile.run("main()", "profile.dat")
    p = pstats.Stats("profile.dat")
    p.sort_stats("time").print_stats()


# 发送控制（是否可以继续发送、发送速度控制）
class SendController(object):
    def __init__(self, total_count, limited_speed, enable_threadsafe = False):
        self.total_count = total_count
        self.send_count = 0
        self.limited_speed = limited_speed
        self.prev_time = None
        if enable_threadsafe:
            self.mutex = threading.Lock()
        else:
            self.mutex = None

    def CanSend(self):
        if self.mutex:
            self.mutex.acquire()

        if self.limited_speed:
            if not self.prev_time:
                self.prev_time = datetime.datetime.now()
            else:
                if self.send_count < self.total_count:
                    if seq / self.limited_speed == 0:
                        curr_time = datetime.datetime.now()
                        td = curr_time - self.prev_time
                        td_us = td.microseconds + (td.seconds + td.days * 24 * 3600) * 10**6
                        # wait_seconds = (1000.0*1000.0/self.limited_speed - td_us)/(1000.0*1000.0)
                        wait_seconds = (10**6 - td_us)/(1000.0*1000.0)
                        if wait_seconds > 0:
                            time.sleep(wait_seconds)
                            self.prev_time = datetime.datetime.now()

        send_continue = self.send_count < self.total_count
        self.send_count = self.send_count + 1

        if self.mutex:
            self.mutex.release()
        return send_continue


# 耗时统计
class Stat(object):
    def __init__(self, total_count, enable_threadsafe = False):
        self.total_count = total_count
        self.success_count = 0
        self.failed_count = 0
        self.starttime = 0
        self.total_us = 0
        if enable_threadsafe:
            self.mutex = threading.Lock()
        else:
            self.mutex = None

    def Start(self):
        self.starttime = datetime.datetime.now()

    def Update(self, is_successful):
        if self.mutex:
            self.mutex.acquire()

        if is_successful:
            self.success_count = self.success_count + 1
        else:
            self.failed_count = self.failed_count + 1

        if self.success_count + self.failed_count == self.total_count:
            endtime = datetime.datetime.now()
            td = endtime - self.starttime
            self.total_us = td.microseconds + (td.seconds + td.days * 24 * 3600) * 10**6

        if self.mutex:
            self.mutex.release()

    def Summary(self):
        while True:
            if self.mutex:
                self.mutex.acquire()
            finished = (self.success_count + self.failed_count == self.total_count)
            if self.mutex:
                self.mutex.release()
            if not finished:
                # print "[pthread=%s] sleep 1ms" % threading.current_thread().ident
                time.sleep(0.001)
            else:
                break

        return (self.total_count, \
                self.success_count, \
                self.failed_count, \
                self.total_us/self.total_count, \
                self.total_count*10**6/self.total_us)

def PrintSummary(summary):
    print "total request count   = %d" % summary[0]
    print "success request count = %d" % summary[1]
    print "failed request count  = %d" % summary[2]
    print "average request time  = %d us" % summary[3]
    print "average process speed = %d /s" % summary[4]

def Print(request, response):
    tty_encoding = sys.getfilesystemencoding()
    print "Send request:"
    print "user    = %s" % request.user
    print "message = %s" % request.message.encode(tty_encoding)
    print "Receive response:"
    print "user    = %s" % response.user
    print "message = %s" % response.message.encode(tty_encoding)
    print
