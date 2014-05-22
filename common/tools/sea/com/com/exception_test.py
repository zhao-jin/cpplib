# -*- coding: gbk -*- 

import re
import struct

import unittest

from com.exception import ComException

def f():
    a
    print "in f"

def g():
    f()
    print "in g"

def h():
    g()
    print "in h"
 

class Test_exception(unittest.TestCase):
    def setUp(self):
        pass

    def teardown(self):
        pass

    def test_getExceptionStr(self):
        try:
            f()
        except:
            s1 = ComException.getExceptionStr()
            m = re.search(r'\[exception_test.py:11\] f                         -->  a',s1)
            self.assertTrue(m)
                
    def test_getExceptionStack(self):
        try:
            f()
        except:
            s2 = ComException.getExceptionStack()
            m = re.search(r'\[exception_test.py:40\] test_getExceptionStack    -->  f()',s2)
            self.assertTrue(m)
            m = re.search(r'\[exception_test.py:11\] f                         -->  a',s2)
            self.assertTrue(m)

        try:
            g()
        except:
            s2 = ComException.getExceptionStack()
            m = re.search(r'\[exception_test.py:49\] test_getExceptionStack    -->  g()',s2)
            self.assertTrue(m)
            m = re.search(r'\[exception_test.py:15\] g                         -->  f()',s2)
            self.assertTrue(m)
            m = re.search(r'\[exception_test.py:11\] f                         -->  a',s2)
            self.assertTrue(m)

        try:
            h()
        except:
            s2 = ComException.getExceptionStack()
            print s2
            m = re.search(r'\[exception_test.py:60\] test_getExceptionStack    -->  h()',s2)
            self.assertTrue(m)
            m = re.search(r'\[exception_test.py:19\] h                         -->  g()',s2)
            self.assertTrue(m)
            m = re.search(r'\[exception_test.py:15\] g                         -->  f()',s2)
            self.assertTrue(m)
            m = re.search(r'\[exception_test.py:11\] f                         -->  a',s2)
            self.assertTrue(m)
if __name__ == '__main__':
    unittest.main()
        
