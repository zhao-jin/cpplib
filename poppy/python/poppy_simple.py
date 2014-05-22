#!/usr/bin/env python
# -*- coding: cp936 -*-
#

import struct
import socket
from google.protobuf import message
from google.protobuf import service
import rpc_meta_info_pb2

class RpcChannel(service.RpcChannel):
    def __init__(self, server_address):
        self.server_address = server_address
        self.sock = None

    def __del__(self):
        if self.sock:
            self.sock.close()

    def CallMethod(self, method_descriptor, rpc_controller,
            request, response_class, done):
        if not self.sock:
            self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.sock.connect(self.server_address)
            first_request_pack = "POST /__rpc_service__ HTTP/1.1\r\n\r\n"
            self.sock.sendall(first_request_pack)
            first_response_pack = "HTTP/1.1 200 OK\r\nX-Poppy-Compress-Type: 0,1\r\n\r\n"
            if self.sock.recv(1024) != first_response_pack:
                self.sock.close()
                self.sock = None
                return
        meta = rpc_meta_info_pb2.RpcMeta()
        meta.sequence_id = 0
        meta.method = method_descriptor.full_name
        meta_str = ""
        try:
            meta_str = meta.SerializeToString()
        except message.EncodeError:
            print "Failed to serialize the request meta"
            sys.exit(1)
        request_str = ""
        try:
            request_str = request.SerializeToString()
        except message.EncodeError:
            print "Failed to serialize the request message"
            sys.exit(2)
        data = struct.pack('!ii', len(meta_str), len(request_str))
        data += meta_str
        data += request_str
        sent = self.sock.send(data)
        response_data = self.sock.recv(1024)
        headers = struct.unpack('!ii', response_data[:8])
        response = None
        response_meta = rpc_meta_info_pb2.RpcMeta()
        try:
            response_meta.ParseFromString(response_data[8:headers[0]+8])
            if response_meta.failed:
                rpc_controller.SetFailed(response_meta.reason)
            else:
                response = response_class()
                try:
                    response.ParseFromString(response_data[ headers[0]+8 : headers[0]+8+headers[1] ])
                except message.DecodeError:
                    rpc_controller.SetFailed("Failed to parse the response message");
                    response = None
        except message.DecodeError:
            rpc_controller.SetFailed("Failed to parse the response metadata")
            response = None
        if done:
            done(response)
            return None
        else:
            return response

class RpcController(service.RpcController):
    def __init__(self):
        self.failed = False
        self.reason = None

    def Reset(self):
        self.failed = False
        self.reason = None

    def Failed(self):
        return self.failed

    def ErrorText(self):
       return self.reason

    def SetFailed(self, reason):
        self.failed = True
        self.reason = reason

    def StartCancel(self):
        raise NotImplementedError

    def IsCanceled(self):
        raise NotImplementedError

    def NotifyOnCancel(self, callback):
        raise NotImplementedError
