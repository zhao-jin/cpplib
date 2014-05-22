# -*- coding: gbk -*- 

'''
checker module
@authors  :   U{raymondxie<mailto: raymondxie@tencent.com>}
@copyright:   tencent
@date     :   2010-07-19
@version  :   1.0.0.0
'''
from com.exception import ComException
class CheckerException(ComException):
    pass;

class ExpectException(ComException):
    pass;
    
from auto.seakeyword import *

def AssertEqual(s1,s2,msg=''):
    '''
    @param s1:比较对象1      
    @type s1: str  ,int ,bool,list ,tuple  
    @param s2:比较对象2  
    @type s2: str  ,int ,bool,list ,tuple    
    @param msg:不符合期望时的错误信息
    @type msg: str                     

    @note: [example] 判断s1和s2是否相等，两者类型必须一致\n
        checker = Check();
        checker.AssertEqual("鲜花","鲜花","前面两个字符串应该相等"); \n
        checker.AssertEqual(1,1,"前面两个数字应该相等"); \n            
    '''     
    if type(s1) == type(s2):
        if s1 == s2:
            return;
        elif msg == '':
	        raise CheckerException("s1:%s is not Equal to s2:%s" % (s1,s2));
        else:
	        raise CheckerException(msg)
    else:
        raise CheckerException("type(s1):%s is not Equal to type(s2):%s" % (type(s1),type(s2)));

def AssertNotEqual(s1,s2,msg=''):
    '''
    @param s1:比较对象1      
    @type s1: str  ,int ,bool,list ,tuple 
    @param s2:比较对象2  
    @type s2: str  ,int ,bool,list ,tuple 
    @param msg:不符合期望时的错误信息
    @type msg: str                     

    @note: [example] 判断s1和s2是否不等，两者类型必须一致\n
        checker = Check();
        checker.AssertNotEqual("鲜花","鲜花a","前面两个字符串不应该相等"); \n
        checker.AssertNotEqual(1,2,"前面两个数字不应该相等"); \n            
    '''      
    if type(s1) == type(s2):
        if s1 != s2:
            return;
        elif msg == '':
            raise CheckerException("s1:%s is Equal to s2:%s" % (s1,s2));
        else:
            raise CheckerException(msg)
    else:
        raise CheckerException("type(s1):%s is not Equal to type(s2):%s" % (type(s1),type(s2)));        
 
def AssertTrue(s,msg=''):
    '''
    @param s:表达式     
    @type s: str ,int, bool     
    @param msg:不符合期望时的错误信息
    @type msg: str                     

    @note: [example] s对象为假则抛出异常\n
        checker = Check();
        checker.AssertTrue(1,"如果前面一个参数为假，则输出该信息"); \n            
        checker.AssertTrue(True,"如果前面一个参数为假，则输出该信息"); \n            
        checker.AssertTrue('ads',"如果前面一个参数为假，则输出该信息"); \n            
    ''' 

    if type(s) == type(True):
        if s == True:
            return;
    elif type(s) == type(1):
        if s != 0:
            return;
    elif type(s) == type(""):
        if s != '':
            return;
    else:
        raise CheckerException("type of s(%s) is not  supported!!" % (type(s),));
    if msg == '':
        raise CheckerException('s is not true.type(s)=%s,s=%s' % (type(s),s));
    else:
        raise CheckerException(msg);

def AssertFalse(s,msg=''):
    '''
    @param s:表达式     
    @type s: str ,int, bool     
    @param msg:不符合期望时的错误信息
    @type msg: str                     

    @note: [example] s对象为真则抛出异常\n
        checker = Check();
        checker.AssertFalse(1,"如果前面一个参数为真，则输出该信息"); \n            
        checker.AssertFalse(True,"如果前面一个参数为真，则输出该信息"); \n            
        checker.AssertFalse('ads',"如果前面一个参数为真，则输出该信息"); \n            
    '''     
    if type(s) == type(False):
        if s == False:
            return;
    elif type(s) == type(0):
        if s == 0:
            return;
    elif type(s) == type(""):
        if s == '':
            return;
    else:
        raise CheckerException("type of s(%s) is not  supported!!" % (type(s),));
    if msg == '':
        raise CheckerException('s is not false.type(s)=%s,s=%s' % (type(s),s));
    else:
        raise CheckerException(msg);

def AssertIn(s,t,msg=''):
    '''
    @param s:单个对象      
    @type s: str ,int, bool   
    @param t:列表对象 
    @type t: list 
    @param msg:不符合期望时的错误信息
    @type msg: str                     

    @note: [example] 判断s是否属于t, 如果t中不包含s,则抛出异常\n
        checker = Check();
        checker.AssertIn(1,[3,4,6],"如果第一个参数不属于第二个参数对应的集合，则输出该信息");\n
    '''
    if t.count(s) == 0:
        if msg == '':
            raise CheckerException("t do not contain s.type(s)=%s,s=%s,type(t),t=%s" % (type(s),s,type(t),t));
        else:
            raise CheckerException(msg);
    return;

def AssertNotIn(s,t,msg=''):
    '''
    @param s:单个对象      
    @type s: str ,int, bool   
    @param t:列表对象 
    @type t: list 
    @param msg:不符合期望时的错误信息
    @type msg: str                     

    @note: [example] 判断s是否不属于t, 如果t中包含s,则抛出异常\n
        checker = Check();
        checker.AssertIn(1,[3,4,6],"如果第一个参数属于第二个参数对应的集合，则输出该信息");\n
    '''
    if t.count(s) != 0:
        if msg == '':
            raise CheckerException("t contain s.type(s)=%s,s=%s,type(t),t=%s" % (type(s),s,type(t),t));
        else:
            raise CheckerException(msg);
    return;

def AssertInclude(s,t,msg=''):
    '''
    @param s:列表1      
    @type s: list
    @param t:列表2 
    @type t: list 
    @param msg:不符合期望时的错误信息
    @type msg: str                     

    @note: [example] 判断s是否包含t, 如果s中不包含t,则抛出异常\n
        checker = Check();
        checker.AssertInclude([1,2,3,4,6],[1,2],"如果第二个参数对应的列表不包含第一个参数对应的列表，则输出该信息");\n       
    '''                
    if type(s) != type([]) or type(t) != type([]):
        raise CheckerException("the param is not list type.type(s)=%s,type(t)=%s" % (type(s),type(t)));
        
    for i in t:
        if s.count(i) == 0:
            if msg == '':
                raise CheckerException("s do not include t.s=%s,t=%s" % (s,t));
            else:
                raise CheckerException(msg);
    return;

def AssertExclude(s,t,msg=''):
    '''
    @param s:列表1      
    @type s: list
    @param t:列表2
    @type t: list 
    @param msg:不符合期望时的错误信息
    @type msg: str                     

    @note: [example] 判断s是否不包含t, 如果s中包含t,则抛出异常\n
        checker = Check();
        checker.AssertExclude([3,4,6],[1,2],"如果第一个参数对应的列表包含第二个参数对应的列表，则输出该信息");\n
    '''  
    if type(s) != type([]) or type(t) != type([]):
        raise CheckerException("the param is not list type.type(s)=%s,type(t)=%s" % (type(s),type(t)));

    excludeCount = 0;
    for i in t:
        if s.count(i) == 1:
            excludeCount += 1

    if excludeCount == len(t):
        if msg == '':
            raise CheckerException("s include t.s=%s,t=%s" % (s,t));
        else:
            raise CheckerException(msg);        

    return;


