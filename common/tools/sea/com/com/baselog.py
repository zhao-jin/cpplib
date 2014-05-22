# -*- coding: gbk -*- 

'''
baselog module
@authors  :   U{raymondxie<mailto: raymondxie@tencent.com>}
@copyright:   tencent
@date     :   2010-07-14
@version  :   1.0.0.0
'''

import os;
import logging;
import com.share
from com.exception import ComException

class BaseLogException(ComException):
    pass; 
    
class BaseLog(object):
    def __init__(self,fn,showLevel='NOTSET'):
        self.fn = fn;
        self.logger = None;
        self.hdlr = None;
        levelDict = { 'NOTSET':logging.NOTSET,
                      'INFO':logging.INFO,
                      'DEBUG':logging.INFO,
                      'WARN':logging.WARN,
                      'ERROR':logging.ERROR,
                      'FATAL':logging.FATAL 
                     }
                      

        self.showLevel = levelDict[showLevel];
        self.openLog();
        
	def getLogger(self):
	    pass
        
    def openLog(self):
        self.logger,self.hdlr = self.getLogger();

    def reopenLog(self,path):
        self.closeLog();
        try:
            self.fn = path + '/' + os.path.basename(self.fn);
        except:
            self.fn = './' + os.path.basename(self.fn);    
        self.openLog();   
        
    def closeLog(self):
        try:
            self.hdlr.close();
            self.logger.removeHandler(self.hdlr);
            del self.logger;
        except:
            pass;

    def __getLogInfo__(self,msg):
        try:
            import   traceback;
            lineNo = traceback.extract_stack()[-3][1];
            funcName = traceback.extract_stack()[-3][2];
            fileName = traceback.extract_stack()[-3][0];
            fileName = os.path.basename(fileName);
         
            import inspect;
            curFuncName = inspect.currentframe().f_back.f_code.co_name;
         
            s = """self.logger.""" + curFuncName + "('''" + msg + """ ---> """ + fileName + """ """ + funcName + """ """ + str(lineNo) + """ """ +  """'''); self.hdlr.flush();""";                        
            c = compile(s,'','exec')
            return c;
        except Exception,e:
            raise BaseLogException,ComException.getExceptionStr();
            return '';

    def debug(self,msg):
        try:
            exec self.__getLogInfo__(msg);
        except :
            print ComException.getExceptionStack();   
            pass;    

    def info(self,msg):
        try:
            exec self.__getLogInfo__(msg);
        except:
            print ComException.getExceptionStack();       
            pass;   

    def warn(self,msg):
        try:
            exec self.__getLogInfo__(msg);
        except :
            print ComException.getExceptionStack();         
            pass;   

    def error(self,msg):
        try:
            exec self.__getLogInfo__(msg);
        except:
            print ComException.getExceptionStack();       
            pass;   


class SeaLog(BaseLog):
    def getLogger(self):
        logger = logging.Logger('funcLog');
        hdlr = logging.FileHandler(self.fn)
        formatter = logging.Formatter('%(asctime)s %(levelname)s %(filename)s %(funcName)s %(lineno)d %(message)s');
        hdlr.setFormatter(formatter)
        logger.addHandler(hdlr)
        logger.setLevel(self.showLevel)
        logging.raiseExceptions = 0;
        
        return logger,hdlr;
	    
class CaseLog(BaseLog):
    def getLogger(self):
        logger = logging.Logger('methodLog');
        hdlr = logging.FileHandler(self.fn)
        formatter = logging.Formatter('%(asctime)s %(levelname)s %(message)s');
        hdlr.setFormatter(formatter)
        logger.addHandler(hdlr)
        logger.setLevel(self.showLevel)
        logging.raiseExceptions = 0;
        
        return logger,hdlr;
	    
class NetLog(BaseLog):
    def getLogger(self):
        self.logger = None
        self.hdlr = None
        logger = logging.Logger('netlog');
        hdlr = logging.FileHandler(self.fn)
        formatter = logging.Formatter('%(asctime)s %(levelname)s %(message)s');
        hdlr.setFormatter(formatter)
        logger.addHandler(hdlr)
        logger.setLevel(self.showLevel)
        logging.raiseExceptions = 0;        
    
        return logger,hdlr;

