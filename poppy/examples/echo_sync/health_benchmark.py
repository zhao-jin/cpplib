#!/usr/bin/env python2.7
# -*- coding: cp936 -*-
#

import sys
import datetime
import httplib
import urllib

def main():
    if len(sys.argv) < 2:
        print "Usage:" + sys.argv[0] + " total_request_count"
        sys.exit(1)

    headers = {
        'Connection' : 'Keep-Alive'
    }

    url = "/health.html"

    conn = httplib.HTTPConnection("127.0.0.1:10000")

    starttime = datetime.datetime.now()
    total_count = 0
    success_count = 0
    failed_count = 0
    while total_count < int(sys.argv[1]):
        conn.request("GET", url, None, headers)
        response_stream = conn.getresponse(True)
        data = response_stream.read()
        if data == "OK":
            success_count = success_count + 1
        else:
            failed_count = failed_count + 1
        total_count = total_count + 1

    endtime = datetime.datetime.now()
    td = endtime - starttime
    total_us = td.microseconds + (td.seconds + td.days * 24 * 3600) * 10**6

    conn.close()

    print "total request count   = %d" %(total_count)
    print "success request count = %d" %(success_count)
    print "failed request count  = %d" %(failed_count)
    print "average request time  = %d us" %(total_us/total_count)
    print "average process speed = %d /s" %(total_count*10**6/total_us)

if __name__ == "__main__":
    main()