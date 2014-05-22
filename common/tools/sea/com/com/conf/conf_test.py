# -*- coding: gbk -*- 

import os
import unittest

from conf import Conf,ConfException,IniConf
from mstringio import MStringIO

root = os.getcwd() + '/tmpfile/'
s = MStringIO()
s.write(
        '''
        k1:v1
        k2:v3
        [test]
        kk:vv
        ''')

class Test_conf(unittest.TestCase):
    def setUp(self):
        pass

    def teardown(self):
        pass
        
    def test_sep_comment(self):
        conf = Conf(root+'all.conf',sep='(:|=)',comment='(#|;|//)')
        assert conf.get('k1') == '11'
        assert conf.get('extra') == '1'
        conf.set('k8',1221)
        assert conf.get('k8') == '1221'
        conf.delete('k8')
        assert conf.get('k4') == '1,2,3'
        assert conf.get('k5') == "'dfd','adfs',adfsdf"

        c1 = Conf(root+'1.conf',sep='=',comment='#')
        assert c1.get('k1') == '1'
        assert c1.get('k4') == '1,2,3'
        assert c1.get('k5') == "'12','12','23'"

        c1 = Conf(root+'2.conf',sep=':',comment='#')
        assert c1.get('k1') == '1'
        assert c1.get('k4') == '1,2,3'
        assert c1.get('k5') == "'12','12'"

        conf = IniConf(root+'all.ini',sep='(:|=)',comment='(#|;|//)')
        assert conf.get('k1') == '11'
        assert conf.get('extra') == '1'
        #conf.set('k8',1221)
        #print conf.get('k8'),type(conf.get('k8'))

        assert conf.get('k11','s1') == '"adfs","kjlafdjkl","2323"'
        assert conf.get('k111','s1','s11') == '''"asdf",'afds\''''
        conf.set('k12','adfs','s1')
        assert conf.get('k12','s1') == 'adfs'
        conf.delete('k12','s1')
        assert conf.hasKey('k12','s1') == False
        conf.set('k112','afs','s1','s11')
        assert conf.get('k112','s1','s11') == 'afs'
        conf.delete('k112','s1','s11')
        conf.hasKey('k112','s1','s11') == False

    def test_get_set(self):
        s.open()
        conf = Conf(s)
        assert conf.get('k1') == 'v1'
        conf.set('k3',1)
        conf.get('k3') == '1'
        s.save('t1.conf')
            
    def test_delete(self):
        c2 = Conf(root+'t1.conf',sep=':',comment='#');
        assert c2.get('k2') == 'v3'
        c2.delete('k1')
        assert c2.hasKey('k1') == False

        
    def test_setBatch(self):
        conf = Conf(root+'all.conf',sep='(:|=)',comment='(#|;|//)')
        dict = {'k21':'23','k22':'24'}
        if conf.hasKey('k21'):   conf.delete('k21')
        if conf.hasKey('k22'):   conf.delete('k22')
        conf.setBatch(dict)
        assert conf.get('k21') == '23'
        assert conf.get('k22') == '24'

    def test_hasKey(self):
        pass

    def test_ini_getset(self):
        pass

    def test_ini_setBatch(self):
        conf = IniConf(root+'all.ini',sep='(:|=)',comment='(#|;|//)')
        dict = {():{'k21':'23'}}
        dict1={('s1'):{'k22':'24'}}
        dict2={('s1','s11'):{'k112':'43'}}
        if conf.hasKey('k21'):   conf.delete('k21')
        if conf.hasKey('k22','s1'):   conf.delete('k22','s1')
        if conf.hasKey('k122','s1','s11'):   conf.delete('k22','s1','s11')
        conf.setBatch(dict)
        conf.setBatch(dict1)
        conf.setBatch(dict2)
        
        assert conf.get('k21') == '23'
        assert conf.get('k22','s1') == '24'
        assert conf.get('k112','s1','s11') == '43'
                
    def test_ini_hasKey(self):
        pass
     
if __name__ == '__main__':
    unittest.main()
