# -*- coding: gbk -*- 

'''
log module
@authors  :   U{raymondxie<mailto: raymondxie@tencent.com>}
@copyright:   tencent
@date     :   2010-07-14
@version  :   1.0.0.0
'''

import os, re, sys
from com.exception import ComException

class LogException(ComException):
    pass;
    
class Log(object):
    def __init__ (self , fn = ""):
        '''
        @param fn:文件名
        @type fn: str     
        @note: [example] \n
            mylog = Log('test.log') 
        '''        
        if not os.path.exists( os.path.abspath((fn)) ):
            raise LogException ,"file:%s is not existed" %(fn,)
            
        self.fn = fn

    def match(self,regex):
        '''
        @param regex:正则表达式
        @type regex: str    
        @return: 匹配，返回True; 否则返回False
        @rtype: bool
        @note: [example] \n
            mylog = Log('test.log') \n
            mylog.match('hello') \n
            mylog.match('[.*?]')
        '''        
        flag = False;
        fileid = open(self.fn , "r")
        for line in fileid.readlines():
            if re.search(regex,line):
                flag = True;
                break;        
        fileid.close()

        return flag;

    def get(self,regex,key,sep='='):
        '''
        @param regex:正则表达式
        @type regex: str    
        @param key:匹配的正则key
        @type key: str   
        @param sep:key 与value的分隔符,默认为=
        @type sep: str           
        @return: 返回匹配到的值，可能有多个
        @rtype: list
        @note: [example] \n
            mylog = Log('test.log') \n
            mylog.get('hello','test') \n
            mylog.get('\[(.+?)\]',"tn",sep=":")
        '''     
        res =[];
        fileid = open(self.fn , "r")
        for line in fileid.readlines():
            if not re.search(regex,line):
                continue;

            m = re.findall(key+'\s*'+sep+'\s*'+'(.+?)\s',line);
            for i in range(0,len(m)):
                res.append(m[i]);
                
        fileid.close()       
        return res;
    

    def clear(self):
        '''
        @note: [example] 清除日志\n
            mylog = Log('test.log') \n
            mylog.clear()
        '''        
        fileid = open(self.fn , "w")
        fileid.writelines("");        
        fileid.close()

