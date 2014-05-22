#!/usr/bin/env python
# -*- coding: cp936 -*-
#

import sys
from poppy import client as poppy
import echo_service_pb2
import test_utils

def EchoRequestComplete(controller, request, response, stat):
    if not controller.Failed():
        stat.Update(True)
        if not response:
            print "BUG"
        # else:
        #     test_utils.Print(request, response)
    else:
        stat.Update(False)
        print "ERROR %d %s" % (controller.ErrorCode(), controller.ErrorText())

def SendRequest(stub, stat):
    controller = poppy.RpcController()
    controller.SetTimeout(2000)
    controller.SetRequestCompressType(poppy.CompressTypeSnappy)
    controller.SetResponseCompressType(poppy.CompressTypeSnappy)
    request = echo_service_pb2.EchoRequest()
    request.user = "ericliu"
    request.message = u"hi,同步调用"
    response = stub.Echo(controller, request, None)
    EchoRequestComplete(controller, request, response, stat)
    # stub.Echo(controller, request,
    #         lambda response: EchoRequestComplete(controller, request, response, stat))

def main():
    total_count = 1
    if len(sys.argv) > 1:
        total_count = int(sys.argv[1])
    limited_speed = 0
    if len(sys.argv) > 2:
        limited_speed = int(sys.argv[2])
    if total_count <= 0 or limited_speed < 0:
        print "Usage: %s [total_request_count] [limited_speed]" % sys.argv[0]
        print "       total_request_count   default=1"
        print "       limited_speed         default=0 unlimited speed"
        sys.exit(1)

    channel = poppy.RpcChannel("127.0.0.1:10000")
    stub = echo_service_pb2.EchoServer_Stub(channel)
    stat = test_utils.Stat(total_count)
    send_controller = test_utils.SendController(total_count, limited_speed)

    stat.Start()
    while send_controller.CanSend():
        SendRequest(stub, stat)

    summary = stat.Summary()
    test_utils.PrintSummary(summary)

if __name__ == "__main__":
    test_utils.Enable_CheckMemoryLeak()

    main()
    sys.exit(0)

    ProfileMain()
