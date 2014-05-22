# -*- coding: gbk -*- 

'''
keyword module
@authors  :   U{raymondxie<mailto: raymondxie@tencent.com>}
@copyright:   tencent
@date     :   2010-07-19
@version  :   1.0.0.0
'''

import socket;
from com.baselog import BaseLog

from com.exception import ComException
from com.net.net import SendException
from com.net.net import RecvException
from auto.check import ExpectException
import com.share 

class KeywordException(ComException):
    pass;

def __genOutputFuncInfo(self,func,arg1,arg2):
    outList = [];
    rpOutList = [];
    try:
        if(self != None):
            outList.append(self.__class__.__name__ + '.' + func.func_name);
            outList.append('(');
        else:
            outList.append(func.func_name);
            outList.append('(');
            
        rpOutList = outList[:];
        vars = func.func_code.co_varnames;   

        hasTupleFlag = False;
        if len(arg1) > 0:            
            hasTupleFlag = True;
        for i in range(0,len(arg1)):
            if i > 0:
                outList.append(',');
                rpOutList.append(',');

            if self:  param = vars[i+1] + '=';rpParam = vars[i+1] + '=';
            else: param = vars[i] + '=';rpParam = vars[i] + '=';
            
            if isinstance(arg1[i],int):
                param += str(arg1[i]);
                rpParam += str(arg1[i]);
            elif isinstance(arg1[i],str):
                param += '"' + str(arg1[i]) + '"';
                rpParam +=  str(arg1[i]);
            elif isinstance(arg1[i],socket.socket):
                param += '"socketObject:' + str(id(arg1[i])) + '"';
                rpParam +=  'socketObject:' + str(id(arg1[i]));                
            else:
                param += str(arg1[i]);
                rpParam += str(arg1[i]);
            outList.append(param);
            rpOutList.append(rpParam);


        count =0;
        for k,v in arg2.items():
            if count == 0:
                if hasTupleFlag:
                    outList.append(',');
                    rpOutList.append(',');
            else:
                outList.append(',');
                rpOutList.append(',');
            if isinstance(v,int):
                param = str(k) + '=' + str(v);
                rpParam = str(k) + '=' + str(v);
            elif isinstance(v,str):
                param = str(k) + '="' + str(v) +'"';
                rpParam = str(k) + '=' + str(v);
            elif isinstance(v,socket.socket):
                param += '"socketObject:' + str(id(v)) + '"';
                rpParam +=  'socketObject:' + str(id(v));                   
            else:    
                param = str(k) + '=' + str(v);
                rpParam = str(k) + '=' + str(v);

            count += 1;
            outList.append(param);                        
            rpOutList.append(rpParam);                        
    except Exception,e:
        print '__genOutputFuncInfo Exception:',e;
        outList = [];
        outList.append(func.func_name+ '(');
        rpOutList = [];
        rpOutList.append(func.func_name+ '(');
    return outList,rpOutList;
    
def methodDecorator(needRp=True):
    def newDecorator(func):
        def replaceMethodDecorator(self,*arg1,**arg2):
            try:
                _logger = com.share.caseLoger;
            except:
                _logger = None;

            succ = True;
            exp = None;
            try:
                return func(self,*arg1,**arg2);
            except Exception,e:      
                succ = False;
                exp = e;
            finally:
                outList,rpOutList = __genOutputFuncInfo(self,func,arg1,arg2);

                if(succ):
                    outList.append(') OK.');
                    if _logger: _logger.info(''.join(outList));
                else:
                    outList.append(') FAIL.Exception:');
                    outList.append(ComException.getExceptionStr());
                    if _logger: _logger.error(''.join(outList));
                    if _logger: _logger.error('\n'.join(['================Exception Stack begin==============',
                                                 ComException.getExceptionStack()]));
                    if _logger: _logger.error('================Exception Stack end  ==============\n');
                                   
                    if isinstance(exp,SendException):
                        print ComException.getExceptionStack();
                        raise SendException,ComException.getExceptionStack();
                    elif isinstance(exp,RecvException):
                        print ComException.getExceptionStack();
                        raise RecvException,ComException.getExceptionStack();
                    else:
                        raise KeywordException,ComException.getExceptionStack();


        def simpleReplaceMethodDecorator(self,*arg1,**arg2):
             return func(self,*arg1,**arg2);
                    
        return replaceMethodDecorator
    return newDecorator

def funcDecorator(needRp=True):
    def newFuncDecoator(func):
        def replaceFuncDecorator(*arg1,**arg2):
            try:
                _logger = com.share.caseLoger
            except:
                _logger = None;

            succ = True;
            exp = None;            
            expectExceptionFlag = False;
            try:
                return func(*arg1,**arg2);
            except ExpectException,e:
                expectExceptionFlag = True;
            except Exception,e:     
                succ = False;
                exp = e;                
            finally:
                outList,rpOutList = __genOutputFuncInfo(None,func,arg1,arg2);

                if(succ):
                    outList.append(') OK.');
                    if _logger: _logger.info(''.join(outList));                
                else:
                    outList.append(') FAIL.Exception:');
                    outList.append(ComException.getExceptionStr());
                    if _logger: _logger.error(''.join(outList));
                    if _logger: _logger.error('\n'.join(['================Exception Stack begin==============',
                                                 ComException.getExceptionStack()]));
                    if _logger: _logger.error('================Exception Stack end  ==============\n');

                    if isinstance(exp,SendException):
                        print ComException.getExceptionStack();
                        raise SendException,ComException.getExceptionStack();
                    elif isinstance(exp,RecvException):
                        print ComException.getExceptionStack();
                        raise RecvException,ComException.getExceptionStack();
                    elif not expectExceptionFlag:
                        raise KeywordException,ComException.getExceptionStack();                        

        def simpleReplaceFuncDecorator(*arg1,**arg2):
             return func(*arg1,**arg2);    
             
        return replaceFuncDecorator
    return newFuncDecoator


