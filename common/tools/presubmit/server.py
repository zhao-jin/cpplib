#!/usr/bin/env python
#
# Copyright 2012 Tencent Inc.
#
# Authors: simonwang <simonwang@tencent.com>
#
# A simple http server to receive presubmit.py's upload patch data

from BaseHTTPServer import HTTPServer, BaseHTTPRequestHandler
from SocketServer import ThreadingMixIn
from pyDes import *
from random import Random
import Cookie
import base64
import cgi
import ldap
import os
import signal
import sys
import threading
import time
import traceback

DEFAULT_DATA_DIR = "/data/presubmit/diff/"

LDAP_URL = "LDAP://10.6.18.41:389"

# DEC Encrypt Initial Key
KEY_INITIAL_VALUE = "\0\1\3\6\2\4\5\7"

def _parse_server_address(server_address):
    """Returns server ip and port from server_adderess"""
    port_index = None
    try:
        port_index = server_address.index(':')
    except ValueError,e:
        pass
    if port_index is None:
        return (None, None)
    server_ip = server_address[:port_index]
    server_port = server_address[port_index + 1 : ]
    return (server_ip, server_port)


def _file_write(filename, content, mode='w'):
    f = open(filename, mode)
    try:
        f.write(content)
    finally:
        f.close()


def _error_exit(msg, code = 1):
    msg = "SimpleServer(error): " + msg
    print >>sys.stderr, msg
    sys.exit(code)


def _random_str(str_length):
    str = ''
    random = Random();
    for i in range(str_length):
        str += chr(random.randint(0, 25) + ord('a'))
    return str


def _auth_user_from_ldap(username=None, password=None):
    """Auth user in LDAP server."""
    try:
        conn = ldap.initialize(LDAP_URL)
        conn.bind(username + '@tencent.com', password)
        time.sleep(0.1)
        resid = conn.search(
            'DC=tencent,DC=com',
            ldap.SCOPE_SUBTREE,
            '(sAMAccountName=%s)' % username,
            ['mailNickname'])
        restype, resdata = conn.result(resid, 0)
        conn.unbind()
        rescn = resdata[0][0]
        resmailnickname = resdata[0][1]['mailNickname'][0]
        if username == resmailnickname:
            return True
        else:
            return False
    except Exception, e:
        return False


class PreSubmitHandler(BaseHTTPRequestHandler):
    """Handler class for presubmit.py's http request."""
    def do_GET(self):
        self.send_response(200)
        self.end_headers()
        message = threading.currentThread().getName()
        self.wfile.write(message)
        self.wfile.write('\n')
        return

    def __auth_user(self, form, cipher=False):
        username = form.getfirst('username', None)
        password = form.getfirst('password', None)
        if username is None or password is None:
            return False
        else:
            if cipher is True:
                k = des("DESCRYPT", CBC, KEY_INITIAL_VALUE, pad=None, padmode=PAD_PKCS5)
                try:
                    data = base64.b64decode(password)
                    password = k.decrypt(data)
                    password = password[10:]
                    print "decrypt password:", password
                except Exception, e:
                    print "decrypt error"
                    return False
            # go to ldap to verify it
            print "username,", username
            print "password,", password
            return _auth_user_from_ldap(username, password)

    def __upload_file(self, form):
        if self.__auth_user(form, True) is False:
            self.send_response(403)
            self.end_headers()
            self.wfile.write("auth error")
            return
        jobname = None
        try:
            jobname = form["jobname"].value
        except:
            pass
        if jobname is None:
            self.send_response(403)
            self.end_headers()
            self.wfile.write("no jobname")
            return
        self.send_response(200)
        self.end_headers()
        self.wfile.write("status ok")
        print "jobname,", jobname
        data_folder = os.path.join(DEFAULT_DATA_DIR, form["jobname"].value)

        for field in form.keys():
            field_item = form[field]
            if field_item.filename:
                file_data = field_item.file.read()
                file_len = len(file_data)
                _file_write(os.path.join(data_folder, field_item.filename), file_data)
                del file_data
            else:
                print "%s=%s" % (field, form[field].value)


    def __do_login(self, form):
        if self.__auth_user(form) is False:
            self.send_response(403)
            self.end_headers()
            self.wfile.write("auth error")
        else:
            self.send_response(200)
            self.end_headers()
            password = form["password"].value
            # add a random str before password
            rand_str = _random_str(10)
            password = rand_str + password
            k = des("DESCRYPT", CBC, KEY_INITIAL_VALUE, pad=None, padmode=PAD_PKCS5)
            encrypt_value = k.encrypt(password)
            base64_encrypt_value = base64.b64encode(encrypt_value)
            self.wfile.write(base64_encrypt_value)

    def do_POST(self):
        form = cgi.FieldStorage(
                fp=self.rfile,
                headers=self.headers,
                environ={'REQUEST_METHOD':'POST',
                         'CONTENT_TYPE':self.headers['Content-Type'],
                         })

        if (self.path == "/login"):
            self.__do_login(form)
        elif (self.path == "/upload"):
            self.__upload_file(form)


class ThreadedHTTPServer(ThreadingMixIn, HTTPServer):
    """Handle requests in a separate thread."""


def _main():
    if len(sys.argv) < 2:
        _error_exit("usage: " + sys.argv[0] + " host:port")
    server_address = sys.argv[1]
    (server_ip, server_port) = _parse_server_address(server_address)
    if (server_ip, server_port) is (None, None):
        _error_exit("usage: " + sys.argv[0] + " host:port")
    server = ThreadedHTTPServer((server_ip, int(server_port)), PreSubmitHandler)
    print "Starting server at " + server_address + ", use <Ctrl-C> to stop"
    server.serve_forever()


def main():
    error_code = 0
    try:
        error_code = _main()
    except SystemExit, e:
        error_code = e.code
    except KeyboardInterrupt:
        _error_exit("Keyboard Interrupted. ", -signal.SIGINT)
    except:
        _error_exit(traceback.format_exc())
    sys.exit(error_code)


if __name__ == "__main__":
    main()
