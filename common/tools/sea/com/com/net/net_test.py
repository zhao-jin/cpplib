# -*- coding: gbk -*-

from com.net.net import *
from com.bft.bft import *
import time
import os
import unittest

root = os.getcwd() + '/tmpfile/'

def handleInput1(self, s):
    buf = s.recv(3)
    c2s = Protocol(root + 'c2s.bft')
    c2s.import_bin(buf)
    print 'buf:',buf
    c2s.list()

def handleInput2(self, s):
    buf = self.recvAll(self,s,headLen=16,bodyOffset=8,bodyOffsetSize=4);
    c2s = Protocol(root + 's2c.bft')
    print 'buf:',buf
    c2s.import_bin(buf)
    c2s.list()

class Test_net(unittest.TestCase):
    def setUp(self):
        pass    

    def teardown(self):
        pass    

    def test_net1(self):
        s = Server(name='TServer',port=8889)
        c = Client(name='TClient',port=8889)

        s.start(handleInput1);
        c2s = Protocol(root+ 'c2s.bft');
        c2s.c =1;
        c2s.d = 2;
        c.connect()
        c.send(c2s.export_bin())
        time.sleep(3)

    def test_net2(self):
        s = Server(name='TServer',port=8888)

        s.start(handleInput2);
        c = Client(name='TClient',port=8888)

        c2s = Protocol(root + 's2c.bft');
        c2s.s2c.head.ver =1;
        c2s.s2c.head.cmd =1;
        c2s.s2c.head.len =5;
        c2s.s2c.head.seq =1;
        c2s.s2c.buf = "abcde";
        c.connect()
        c.send(c2s.export_bin())

        time.sleep(2)

if __name__ == '__main__':
    unittest.main()
    
