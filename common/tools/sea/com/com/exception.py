# -*- coding: gbk -*- 

'''
exception module
@authors  :   U{raymondxie<mailto: raymondxie@tencent.com>}
@copyright:   tencent
@date     :   2010-07-14
@version  :   1.0.0.0
'''

import os
import re
import sys
import linecache
import traceback

class ComException(Exception):
    def __init__(self, msg):
        '''
        @param msg:异常信息
        @type msg: str     
        @note: [example] \n
            ce = ComException('XXXX Error') 
        '''      
        Exception.__init__(self,msg);
        self.msg = msg;
        
    def __str__(self):
        print '==============================='
        return str(self.msg);

    @classmethod
    def getExceptionStr(cls):
        '''
        @note: [example] 类方法，获得异常堆栈的最近一层信息 \n
            print ComException.getExceptionStr()
        '''       
        e_type, e_value, e_tb = sys.exc_info();
        msg = ''
        msgList = traceback.extract_tb(e_tb)
        filename, lineno, name, line = msgList[-1]
        if line != None :
            m1 = '[' + os.path.basename(str(filename)) + ':' + str(lineno) + '] ';
            msg += m1.ljust(22) + str(name).ljust(25) + ' -->  ' + str(line) + '\n';
        
        preList = str(e_value).split('\n');
        
        eStrType = str(e_type);
        m0 = re.match("<(type|class) '([^)]*)'>",str(e_type));
        if m0:
            eStrType = m0.group(2); 
    
        m = re.match('^[ \t]*([a-zA-Z0-9\.]*(Exception|Error)): ',preList[-1]);
        if not m:
            msg += eStrType + ': ' + preList[-1];
        else:
            msg += preList[-1];

    
        return msg;     

    @classmethod
    def getExceptionStack(cls):
        '''
        @note: [example] 类方法，获得异常堆栈信息 \n
            print ComException.getExceptionStack()
        '''        
        e_type, e_value, e_tb = sys.exc_info()
        msgList = traceback.extract_tb(e_tb)
        msg = '';
        for filename, lineno, name, line in msgList:
            #if line != None and not re.search('raise',line):
            if line != None :
                m1 = '[' + os.path.basename(str(filename)) + ':' + str(lineno) + '] ';
                msg += m1.ljust(22) + str(name).ljust(25) + ' -->  ' + str(line) + '\n';
    
        eStrType = str(e_type);
        m0 = re.match("<(type|class) '([^)]*)'>",str(e_type));
        if m0:
            eStrType = m0.group(2);
                        
        preList = str(e_value).split('\n');
        m = re.match('^[ \t]*([a-zA-Z0-9\.]*(Exception|Error)): ',preList[-1]);
        if not m: 
            preList[-1] = eStrType + ': ' + preList[-1];
    
        msg += '\n'.join(preList); 
    
        return msg;


   

