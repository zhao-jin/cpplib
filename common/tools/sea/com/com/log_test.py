# -*- coding: gbk -*- 

import struct
import shutil
import os
import unittest

from com import log

root = os.getcwd() + '/tmpfile/'
class Test_log(unittest.TestCase):
    def setUp(self):
        pass

    def teardown(self):
        pass

    def test_init(self):
        try:
            mylog = log.Log(root+ 'test1.log')
        except log.LogException:
            self.assertTrue(True);
        try:
            mylog = log.Log()
        except log.LogException:
            self.assertTrue(True);
        
    def test_match(self):
        mylog = log.Log(root+ 'test.log')
        self.assertTrue(mylog.match('End run cases') == True)
        self.assertTrue(mylog.match('End cases') == False)
        self.assertTrue(mylog.match('__[^_]*__') == True)
        
        mylog = log.Log(root+ 'test2.log')
        self.assertTrue(mylog.match('__[^_]*__') != True)
                
    def test_get(self):
        mylog = log.Log(root+ 'test.log')
        self.assertTrue(mylog.get('ctlr.py','rn','=') == ['1','3'])
        self.assertTrue(mylog.get('test','rn','=') == ['1'])
        self.assertTrue(mylog.get('\[[^t]*?\]','rn','=') == ['3'])
        
        mylog = log.Log(root+ 'test2.log')
        self.assertTrue(mylog.get('ctlr.py','rn','=') != ['1','3'])

    def test_clear(self):
        shutil.copyfile(root+'test.log',root+'test1.log');
        mylog = log.Log(root+ 'test.log')
        self.assertTrue(mylog.match('End run cases') == True)
        self.assertTrue(mylog.get('ctlr.py','rn','=') == ['1','3'])
        mylog.clear()        
        self.assertTrue(mylog.match('End run cases') != True)
        self.assertTrue(mylog.get('ctlr.py','rn','=') != ['1','3'])
        shutil.copyfile(root+'test1.log',root+'test.log');

if __name__ == '__main__':
    unittest.main()
        
