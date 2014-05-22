#!/usr/bin/env python
# -*- coding: cp936 -*-
#

import os
import sys
import time
import datetime
import poppy_simple as poppy
import echo_service_pb2

TTY_ENCODING = sys.getfilesystemencoding()

starttime = 0
total_count = 0
success_count = 0
failed_count = 0

def PrintDebugInfo(request, response):
    print "Send request:"
    print "user    = %s" % request.user
    print "message = %s" % request.message.encode(TTY_ENCODING)
    print "Receive response:"
    print "user    = %s" % response.user
    print "message = %s" % response.message.encode(TTY_ENCODING)
    print

def PrintStatisticsInfo(total_us):
    print "total request count   = %d" % total_count
    print "success request count = %d" % success_count
    print "failed request count  = %d" % failed_count
    print "average request time  = %d us" % (total_us/total_count)
    print "average process speed = %d /s" % (total_count*10**6/total_us)

def UpdateCounters(controller):
    global success_count
    global failed_count
    if controller.Failed():
        failed_count = failed_count + 1
    else:
        success_count = success_count + 1
    if success_count + failed_count == total_count:
        endtime = datetime.datetime.now()
        td = endtime - starttime
        total_us = td.microseconds + (td.seconds + td.days * 24 * 3600) * 10**6
        PrintStatisticsInfo(total_us)

def EchoRequestComplete(controller, request, response):
    UpdateCounters(controller)
    if not controller.Failed():
        if not response:
            print "BUG"
    #    else:
    #        PrintDebugInfo(request, response)
    else:
        print "ERROR %s" % controller.ErrorText()

def main():
    if len(sys.argv) < 2:
        print "Usage: %s total_request_count" % sys.argv[0]
        sys.exit(1)

    global total_count
    total_count = int(sys.argv[1])

    channel = poppy.RpcChannel(("127.0.0.1", 10000))
    stub = echo_service_pb2.EchoServer_Stub(channel)

    global starttime
    starttime = datetime.datetime.now()
    use_callback = True
    i = 0
    while i < total_count:
        controller = poppy.RpcController()
        request = echo_service_pb2.EchoRequest()
        request.user = "ericliu"
        request.message = u"hello,中国人"
        if use_callback:
            stub.Echo(controller, request,
                    lambda response: EchoRequestComplete(controller, request, response))
        else:
            response = stub.Echo(controller, request, None)
            EchoRequestComplete(controller, request, response)
        i = i + 1

    while True:
        if success_count + failed_count == total_count:
            break
        print "sleep 1s"
        time.sleep(1)

if __name__ == "__main__":
    import gc
    gc.set_debug(gc.DEBUG_LEAK)

    main()
