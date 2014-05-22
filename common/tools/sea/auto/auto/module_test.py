# -*- coding: gbk -*- 

import unittest

from auto.module import *
from com.shell import ShellException
from com.baselog import *
import com.share

class Test_check(unittest.TestCase):
    def setUp(self):
        pass
        
    def teardown(self):
        pass
        
    def test_module(self):
        try:
            module = Module('TS',homePath='~')
            com.share.caseLoger = CaseLog('m1.log')
            module.start('ls -l',True)
            module.stop('pwd',True)
            module.reload('cd ~;pwd',True)
            module.restart('pwd',True)
            module.checkHealth('df -k',True);

            module.start('ls -l',False)
            module.stop('pwd',False)
            module.reload('cd ~;pwd',False)
            module.restart('pwd',False)
            module.checkHealth('df -k',False);

            module.checkHealth('sleep 3',False,True);
            module.start('ls -l',False,True)
            module.stop('pwd',False,True)
            module.reload('cd ~;pwd',False,True)
            module.restart('pwd',False,True)
            module.checkHealth('df -k',False,True);

            module.start('sleep 1',True,True)
            module.stop('sleep 2',True,True)
            module.reload('sleep 3',True,True)
            module.restart('sleep 4',True,True)
            module.checkHealth('sleep 5',True,True);

            module.start('lxxs -l',True)
            module.stop('pwdss',True)
            module.reload('cdss ~;pwd',True)
            module.restart('pwdss',True)
            module.checkHealth('dssf -k',True);

            module.start('lxxs -l',False)
            module.stop('pwdss',False)
            module.reload('cdss ~;pwd',False)
            module.restart('pwdss',False)

        except ShellException:
            self.assertTrue(True)
        except:
            self.assertTrue(False)

if __name__ == '__main__':
    unittest.main()

