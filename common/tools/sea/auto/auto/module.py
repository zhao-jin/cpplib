# -*- coding: gbk -*- 

'''
module module
@authors  :   U{raymondxie<mailto: raymondxie@tencent.com>}
@copyright:   tencent
@date     :   2010-07-19
@version  :   1.0.0.0
'''

import os;
import re;
import sys;
import time; 

import com.shell
import com.share;
from com.exception import ComException

class ModuleException(ComException):
    pass;
    
class Module(object):
    def __init__ (self,name='',port =None,reloadPort=None,
                  homePath = '',binPath = '',confPath='',logPath='',dictPath='',dbPath=''):
        '''
        @param name:ģ����
        @type name: str   
        @param port:ģ������˿�
        @type port: int  
        @param reloadPort:ģ��reload���ö˿�
        @type reloadPort: int  
        @param homePath:ģ���Ŀ¼
        @type homePath: str  
        @param binPath:ģ��binĿ¼������û�У�Ĭ��Ϊ $homePath/bin/
        @type binPath: str  
        @param confPath:ģ��confĿ¼������û�У�Ĭ��Ϊ $homePath/conf/
        @type confPath: str                                  
        @param logPath:ģ��logĿ¼������û�У�Ĭ��Ϊ $homePath/log/
        @type logPath: str  
        @param dictPath:ģ������Ŀ¼������û�У�Ĭ��Ϊ $homePath/data/
        @type dictPath: str  
        @param dbPath:ģ����ļ�Ŀ¼������û�У�Ĭ��Ϊ $homePath/db/
        @type dbPath: str  
                                
        @note: [example] \n
            m = Module(name='CB',port=8999,homePath="/home/work/is/") \n
        ''' 
        self.name = name;
        self.port = port
        self.reloadPort = reloadPort
        
        self.homePath = homePath

        if confPath == '':
            self.binPath = self.homePath + '/bin/';
        else:
            self.binPath = binPath;
            
        if confPath == '':
            self.confPath = self.homePath + '/conf/';
        else:
            self.confPath = confPath;

        if logPath == '':    
            self.logPath = self.homePath + '/log/';
        else:
            self.logPath = logPath;

        if dictPath == '':    
            self.dictPath = self.homePath + '/data/';
        else:
            self.dictPath = dictPath;     

        if dbPath == '':    
            self.dbPath = self.homePath + '/db/';
        else:
            self.dbPath = dbPath;              


    def start(self,cmd,outputFlag=True,backend=False):
        '''
        @param cmd:�����ַ���
        @type cmd: str     
        @param outputFlag:�Ƿ��������ִ�н����ʶ��Ĭ�����
        @type outputFlag: bool  
        @param backend:�Ƿ��Ǻ�̨���Ĭ��Ϊǰ̨����
        @type backend: bool           
        @note: [example] \n
            m = Module(port=8999,homePath="/home/work/is/") \n
            m.start("ls") \n 
            m.start("ls",False) \n 
        '''      
        return com.share.cmder.execute(cmd,outputFlag,backend);

    def stop(self,cmd,outputFlag=True,backend=False):
        '''
        @param cmd:�����ַ���
        @type cmd: str     
        @param outputFlag:�Ƿ��������ִ�н����ʶ��Ĭ�����
        @type outputFlag: bool  
        @param backend:�Ƿ��Ǻ�̨���Ĭ��Ϊǰ̨����
        @type backend: bool           
        @note: [example] \n
            m = Module(port=8999,homePath="/home/work/is/") \n
            m.stop("ls") \n 
            m.stop("ls",False) \n 
        '''      
        return com.share.cmder.execute(cmd,outputFlag,backend);

    def restart(self,cmd,outputFlag=True,backend=False):
        '''
        @param cmd:�����ַ���
        @type cmd: str     
        @param outputFlag:�Ƿ��������ִ�н����ʶ��Ĭ�����
        @type outputFlag: bool  
        @param backend:�Ƿ��Ǻ�̨���Ĭ��Ϊǰ̨����
        @type backend: bool           
        @note: [example] \n
            m = Module(port=8999,homePath="/home/work/is/") \n
            m.restart("ls") \n 
            m.restart("ls",False) \n 
        '''      
        return com.share.cmder.execute(cmd,outputFlag,backend);

    def reload(self,cmd,reloadPort=None,outputFlag=True,backend=False):
        '''
        @param cmd:�����ַ���
        @type cmd: str     
        @param outputFlag:�Ƿ��������ִ�н����ʶ��Ĭ�����
        @type outputFlag: bool  
        @param backend:�Ƿ��Ǻ�̨���Ĭ��Ϊǰ̨����
        @type backend: bool           
        @note: [example] \n
            m = Module(port=8999,homePath="/home/work/is/") \n
            m.reload("ls") \n 
            m.reload("ls",reloadPort=9999) \n 
            m.reload("ls",outputFlag=False) \n 
        '''     
        self.reloadPort = reloadPort;
        return com.share.cmder.execute(cmd,outputFlag,backend);

    def checkHealth(self,cmd,outputFlag=True,backend=False):
        '''
        @param cmd:�����ַ���
        @type cmd: str     
        @param outputFlag:�Ƿ��������ִ�н����ʶ��Ĭ�����
        @type outputFlag: bool  
        @param backend:�Ƿ��Ǻ�̨���Ĭ��Ϊǰ̨����
        @type backend: bool           
        @note: [example] \n
            m = Module(port=8999,homePath="/home/work/is/") \n
            m.checkHealth("ls") \n 
            m.checkHealth("ls",False) \n 
        '''      
        return com.share.cmder.execute(cmd,outputFlag,backend);

