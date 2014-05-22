# -*- coding: gbk -*- 

import unittest

from auto.check import *
from com.baselog import *
import com.share

class Test_check(unittest.TestCase):
    def setUp(self):
        pass

    def teardown(self):
        pass

    def test_assert(self):
        com.share.caseLoger = CaseLog('m1.log')
        AssertTrue(1==1)
        AssertTrue(1)
        AssertTrue("adf")
        try:
            AssertTrue([])
        except CheckerException:
            self.assertTrue(True)
        except:
            self.assertTrue(False)

        AssertFalse(1==0)
        AssertFalse(0)
        AssertFalse("")

        AssertEqual(1,1)
        AssertEqual(True,True)
        AssertEqual('afds','afds')
        AssertEqual([1,'afds'],[1,'afds'],msg='Error')
        AssertEqual((1,'afds'),(1,'afds'),msg='Error')

        AssertNotEqual(1,0)
        AssertNotEqual(True,False)
        AssertNotEqual('afs','afds')
        AssertNotEqual([21,'afds'],[1,'afds'],msg='Error')
        AssertNotEqual((21,'afds'),(1,'afds'),msg='Error')
        try:
            AssertNotEqual(1,"1",msg='Error')
        except CheckerException:
            self.assertTrue(True)
        except:
            self.assertTrue(False)

        AssertIn(1,[1,2,3],'test')
        AssertIn("1",["1",2,3],'test')
        AssertIn(True,["1",True,3],'test')
        
        AssertNotIn(4,[1,2,3],'not in error')
        AssertNotIn("ad",[1,2,3],'not in error')
        AssertNotIn(False,[1,2,3],'not in error')

        AssertInclude([1,2,3],[1,2],'test')
        AssertInclude([1,2,3],[],'test')

        AssertExclude([1,2,3],[5],'not in error')
        AssertExclude([],[5],'not in error')

if __name__ == '__main__':
    unittest.main()
