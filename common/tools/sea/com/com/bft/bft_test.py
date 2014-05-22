# -*- coding: gbk -*- 

import os
import unittest

from com.bft.bft import *

root = os.getcwd() + '/tmpfile/'

class Test_bft(unittest.TestCase):
    def setUp(self):
        pass

    def teardown(self):
        pass

    def test_read(self):
        a= Protocol(root+'c2s.bft',True)
        #a.setEndian(True);
        import struct

        buf = struct.pack('>hiq',2,4,6)
        f=file('dump','w')
        f.write(buf);
        buf +="test"
        print repr(buf);
        a.list();
        self.assertTrue(int(str(a.c2s.d)) == 0)
        self.assertTrue(int(str(a.c2s.e)) == 2)
        self.assertTrue(int(str(a.c2s.h)) == 0)
        self.assertTrue(str(a.c2s.g) == "")


        a.import_bin(buf);
        a.c2s.g = "test"
        a.list()
        self.assertTrue(int(str(a.c2s.d)) == 2)
        self.assertTrue(int(str(a.c2s.e)) == 4)
        self.assertTrue(int(str(a.c2s.h)) == 6)
        self.assertTrue(str(a.c2s.g) == "test")
        a.c2s.d = 1;
        a.c2s.e = 2;
        a.c2s.h = 3
        self.assertTrue(int(str(a.c2s.d)) == 1)
        self.assertTrue(int(str(a.c2s.e)) == 2)
        self.assertTrue(int(str(a.c2s.h)) == 3)
        print repr(a.export_bin()) ,repr(a.export_bin())

        #b=Protocol(root+'c2s.bft',True);
        b=Protocol(a);
        
        b.import_bin(a.export_bin());
        self.assertTrue(int(str(b.c2s.d)) == 1)
        self.assertTrue(int(str(b.c2s.e)) == 2)
        self.assertTrue(int(str(b.c2s.h)) == 3)
        self.assertTrue(str(b.c2s.g) == "test")
        b.list()
        
        c=Protocol(root+'c2s.bft');
        c.import_bin(a.export_bin());
        self.assertTrue(int(str(c.c2s.d)) == 256)
        self.assertTrue(int(str(c.c2s.e)) == 33554432)
        self.assertTrue(long(str(c.c2s.h)) == 216172782113783808)
        self.assertTrue(str(c.c2s.g) == "test")
        c.list()


if __name__ == '__main__':
        unittest.main()
        
