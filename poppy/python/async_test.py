#!/usr/bin/env python
# -*- coding: cp936 -*-
#

import sys
import uuid
from functools import partial
from poppy import client as poppy
import echo_service_pb2
import test_utils

def EchoRequestComplete(stub, controller, request, stat, send_controller, response):
    if not controller.Failed():
        stat.Update(True)
        if not response:
            print "BUG"
        else:
            test_utils.Print(request, response)
    else:
        stat.Update(False)
        print "ERROR %d %s" % (controller.ErrorCode(), controller.ErrorText())
    if send_controller.CanSend():
        SendRequest(stub, stat, send_controller)

def SendRequest(stub, stat, send_controller):
    controller = poppy.RpcController()
    controller.SetTimeout(2000)
    controller.SetRequestCompressType(poppy.CompressTypeSnappy)
    controller.SetResponseCompressType(poppy.CompressTypeSnappy)
    request = echo_service_pb2.EchoRequest()
    request.user = "ericliu"
    request.message = u"hi,异步调用 " + uuid.uuid4().get_hex()
    stub.Echo(controller, request,
            lambda response: EchoRequestComplete(stub, controller, request, stat, send_controller, response))

def main():
    total_count = 1
    if len(sys.argv) > 1:
        total_count = int(sys.argv[1])
    initial_request_count = 1
    if len(sys.argv) > 2:
        initial_request_count = int(sys.argv[2])
    limited_speed = 0
    if len(sys.argv) > 3:
        limited_speed = int(sys.argv[3])
    if total_count <= 0 or limited_speed < 0 or initial_request_count <= 0 or \
        initial_request_count > total_count:
        print "Usage: %s [total_request_count] [initial_request_count] [limited_speed]" \
                % sys.argv[0]
        print "       total_request_count   default=1"
        print "       initial_request_count default=1 less than or equal total_request_count"
        print "       limited_speed         default=0 unlimited speed"
        sys.exit(1)

    channel = poppy.RpcChannel("127.0.0.1:10000")
    stub = echo_service_pb2.EchoServer_Stub(channel)
    stat = test_utils.Stat(total_count, True)
    send_controller = test_utils.SendController(total_count, limited_speed, True)

    stat.Start()
    i = 0
    while i < initial_request_count:
        if not send_controller.CanSend():
            break
        # SendRequest(stub, stat, send_controller)
        controller = poppy.RpcController()
        controller.SetTimeout(2000)
        controller.SetRequestCompressType(poppy.CompressTypeSnappy)
        controller.SetResponseCompressType(poppy.CompressTypeSnappy)
        request = echo_service_pb2.EchoRequest()
        request.user = "ericliu"
        request.message = u"hi,异步调用 " + uuid.uuid4().get_hex()

        # 在循环中用lambda表达式惰性求值会导致绑定的参数controller和request错误
        # 用partial则没有问题，另外如果将构建lambda封装在一个函数也没有问题
        # http://stackoverflow.com/questions/938429/scope-of-python-lambda-functions-and-their-parameters
        # http://hi.baidu.com/fiber212121/blog/item/60eb628b26350a1bc9fc7a4d.html
        # stub.Echo(controller, request,
        #         lambda response: EchoRequestComplete(stub, controller, request, stat, send_controller, response))
        stub.Echo(controller, request,
                partial(EchoRequestComplete, stub, controller, request, stat, send_controller))
        i = i + 1

    summary = stat.Summary()
    test_utils.PrintSummary(summary)

if __name__ == "__main__":
    # test_utils.Enable_CheckMemoryLeak()

    main()
    sys.exit(0)

    ProfileMain()
