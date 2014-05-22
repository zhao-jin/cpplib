# -*- coding: gbk -*- 

'''
shell module
@authors  :   U{raymondxie<mailto: raymondxie@tencent.com>}
@copyright:   tencent
@date     :   2010-07-14
@version  :   1.0.0.0
'''

import os;
import subprocess
import com.share
from com.exception import ComException

class ShellException(ComException):
    pass;

class Shell(object):
    def __init__(self):
        self.MAX_PIPE_BUF = 10240;
        self.WRITE_SIZE = 0;    
        
        self.shellRecv,self.appSend = os.pipe();
        self.appRecv,self.shellSend = os.pipe();   
       
        self.childPid = os.fork();
        if self.childPid == 0: #in shell
            self.__run();

    def __run(self):
        while True:
            cmdstr = os.read(self.shellRecv,self.MAX_PIPE_BUF);
            cmdList = cmdstr.rsplit('\n',1);            
            res = subprocess.Popen(cmdList[0], shell=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
            if cmdList[1] == 'False':
                output = (res.wait(),res.stdout.read())
            else:
                output = (res.wait(),'')

            self.WRITE_SIZE = len(str(output[0])) + 1 + len(output[1]);
            os.write(self.shellSend,str(output[0]) + '\n' + output[1]); 
            
    def execute(self,cmd,outputFlag=True,backend=False):
        '''
        @param cmd:命令字符串
        @type cmd: str     
        @param outputFlag:是否输出命令执行结果标识，默认输出
        @type outputFlag: bool  
        @param backend:是否是后台命令，默认为前台命令
        @type backend: bool          
        @note: [example] \n
            cmder = Shell() \n
            cmder.execute("ls")  
            cmder.execute("sleep 100&",backend=True)  
            cmder.execute("sleep 100&",outputFlag=False,backend=True)  
        '''    
        if outputFlag:
            print 'Execute command:',cmd;

        if backend:
            if not cmd.strip().endswith('&'):
                cmd = cmd + '&'
   
        cmdstr = cmd + '\n' + str(backend);      
        
        os.write(self.appSend,cmdstr);
        outBuf = os.read(self.appRecv,self.MAX_PIPE_BUF);
    
        outBufList = outBuf.split('\n',1);
        if outputFlag:
            print 'Command excute result code:',outBufList[0];
            print outBufList[1],'\n';

        if outBufList[0] != '0':
            raise ShellException('\nCommand excute result code: %s is not 0.\nCommand excute result: %s' %(outBufList[0],outBufList[1]));
            
        return outBufList[1];


com.share.cmder = Shell(); # global cmd object

