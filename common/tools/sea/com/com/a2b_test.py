# -*- coding: gbk -*- 

import struct
import unittest

from com import a2b

class Test_a2b(unittest.TestCase):
    def setUp(self):
        pass

    def teardown(self):
        pass

    def test_ip2int(self):
        self.assertTrue(a2b.ip2int("172.29.1.1") == 16850348)
        self.assertTrue(a2b.ip2int("0.0.0.0") == 0)
        self.assertTrue(a2b.ip2int("255.255.255.255") == 4294967295)

    def test_int2ip(self):
        self.assertTrue(a2b.int2ip(16850348) == "172.29.1.1")
        self.assertTrue(a2b.int2ip(0) == "0.0.0.0")
        self.assertTrue(a2b.int2ip(4294967295) == "255.255.255.255")

    def test_longStr2Twoint(self):
        self.assertTrue(a2b.longStr2Twoint("12345678") == (875770417, 943142453))
        self.assertTrue(a2b.longStr2Twoint("00000000") == (808464432, 808464432))
        self.assertTrue(a2b.longStr2Twoint("ffffffff") == (1717986918, 1717986918))

    def test_twoint2longStr(self):
        self.assertTrue(a2b.twoint2longStr(875770417, 943142453) == "12345678")
        self.assertTrue(a2b.twoint2longStr(808464432, 808464432) == "00000000")
        self.assertTrue(a2b.twoint2longStr(1717986918, 1717986918) == "ffffffff")

    def test_byte2hex(self):
        s=struct.pack("i",1234)
        self.assertTrue(a2b.byte2hex(s) == 'd2 04 00 00')
        s=struct.pack("i",0)
        self.assertTrue(a2b.byte2hex(s) == '00 00 00 00')
        s=struct.pack("H",0xffff)
        self.assertTrue(a2b.byte2hex(s) == 'ff ff')
        s="\xd2\x04\x00\x00\x34\x53\x00\x01"
        self.assertTrue(a2b.byte2hex(s) == 'd2 04 00 00 34 53 00 01')
        s='\x00\x00\x00\x00\x01\x00\x00\x0012\t\x00\x00\x00\x04\x00\x00\x00\x05\x00\x00\x00'
        self.assertTrue(a2b.byte2hex(s) == '00 00 00 00 01 00 00 00 31 32 09 00 00 00 04 00 00 00 05 00 00 00')

    def test_hex2byte(self):
        s = repr(struct.pack("i",1234))
        self.assertTrue(repr(a2b.hex2byte('d2 04 00 00')) == s)
        s = repr(struct.pack("i",0))
        self.assertTrue(repr(a2b.hex2byte('00 00 00 00')) == s)
        s = repr(struct.pack("H",0xffff))
        self.assertTrue(repr(a2b.hex2byte('ff ff')) == s)
        s="\xd2\x04\x00\x00\x34\x53\x00\x01"
        self.assertTrue(a2b.hex2byte(a2b.byte2hex(s)) == s)
        s="\x00\x00\x00\x00\x01\x00\x00\x0012\t\x00\x00\x00\x04\x00\x00\x00\x05\x00\x00\x00"
        self.assertTrue(a2b.hex2byte(a2b.byte2hex(s)) == s)


if __name__ == '__main__':
    unittest.main()
        
