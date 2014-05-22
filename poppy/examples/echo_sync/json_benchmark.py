#!/usr/bin/env python2.7
# -*- coding: cp936 -*-
#

import os
import sys
import datetime
import httplib
import urllib
import minjson as json

TTY_ENCODING = sys.getfilesystemencoding()

def PrintDebugInfo(url, request, response):
    print "\nSend json request :"
    print "POST " + url
    print "user    = " + request["user"]
    print "message = " + urllib.unquote_plus(
        request["message"]).decode("utf-8").encode(TTY_ENCODING)
    print "\nReceive json response:"
    print "user    = " + response["user"]
    print "message = " + urllib.unquote_plus(
        response["message"]).decode("utf-8").encode(TTY_ENCODING)
    print

def main():
    if len(sys.argv) < 2:
        print "Usage: %s total_request_count" % sys.argv[0]
        sys.exit(1)

    # params = urllib.urlencode({
    #         'method' : 'rpc_examples.EchoServer.EchoMessage'
    #     })
    #
    # url = "/rpc?" + params

    url = "/rpc/rpc_examples.EchoServer.EchoMessage?e=1"

    headers = {
        'Content-Type' : 'application/json; charset=utf-8',
        'Connection' : 'Keep-Alive'
    }

    conn = httplib.HTTPConnection("127.0.0.1:10000")

    starttime = datetime.datetime.now()
    total_count = 0
    failed_count = 0
    success_count = 0
    while total_count < int(sys.argv[1]):
        try:
            data = {}
            data["user"] = "ericliu"
            data["message"] = urllib.quote_plus(u'hello,中国人'.encode("utf-8"))
            body = json.write(data)
            conn.request("POST", url, body, headers)
            response = conn.getresponse(True)
            if response.status == httplib.OK:
                json_body = response.read()
                result = json.read(json_body)
                PrintDebugInfo(url, data, result)
                success_count = success_count + 1
            else:
                error_text = response.read()
                print "Server Internal Error:"
                print "%d: %s, %s" % (response.status, response.reason, error_text)
                failed_count = failed_count + 1
        except json.ReadException:
            failed_count = failed_count + 1
        total_count = total_count + 1

    endtime = datetime.datetime.now()
    td = endtime - starttime
    total_us = td.microseconds + (td.seconds + td.days * 24 * 3600) * 10**6

    conn.close()

    print "total request count   = %d" % total_count
    print "success request count = %d" % success_count
    print "failed request count  = %d" % failed_count
    print "average request time  = %d us" % (total_us/total_count)
    print "average process speed = %d /s" % (total_count*10**6/total_us)

if __name__ == "__main__":
    main()

# curl -H 'Content-Type:application/json; charset=utf-8' -H 'Connection:Keep-Alive' -d '{"user": "ericliu", "message": "hello,中国人"}' http://127.0.0.1:10000/rpc/rpc_examples.EchoServer.EchoMessage
