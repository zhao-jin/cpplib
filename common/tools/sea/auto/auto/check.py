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
    @param s1:�Ƚ϶���1      
    @type s1: str  ,int ,bool,list ,tuple  
    @param s2:�Ƚ϶���2  
    @type s2: str  ,int ,bool,list ,tuple    
    @param msg:����������ʱ�Ĵ�����Ϣ
    @type msg: str                     

    @note: [example] �ж�s1��s2�Ƿ���ȣ��������ͱ���һ��\n
        checker = Check();
        checker.AssertEqual("�ʻ�","�ʻ�","ǰ�������ַ���Ӧ�����"); \n
        checker.AssertEqual(1,1,"ǰ����������Ӧ�����"); \n            
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
    @param s1:�Ƚ϶���1      
    @type s1: str  ,int ,bool,list ,tuple 
    @param s2:�Ƚ϶���2  
    @type s2: str  ,int ,bool,list ,tuple 
    @param msg:����������ʱ�Ĵ�����Ϣ
    @type msg: str                     

    @note: [example] �ж�s1��s2�Ƿ񲻵ȣ��������ͱ���һ��\n
        checker = Check();
        checker.AssertNotEqual("�ʻ�","�ʻ�a","ǰ�������ַ�����Ӧ�����"); \n
        checker.AssertNotEqual(1,2,"ǰ���������ֲ�Ӧ�����"); \n            
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
    @param s:���ʽ     
    @type s: str ,int, bool     
    @param msg:����������ʱ�Ĵ�����Ϣ
    @type msg: str                     

    @note: [example] s����Ϊ�����׳��쳣\n
        checker = Check();
        checker.AssertTrue(1,"���ǰ��һ������Ϊ�٣����������Ϣ"); \n            
        checker.AssertTrue(True,"���ǰ��һ������Ϊ�٣����������Ϣ"); \n            
        checker.AssertTrue('ads',"���ǰ��һ������Ϊ�٣����������Ϣ"); \n            
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
    @param s:���ʽ     
    @type s: str ,int, bool     
    @param msg:����������ʱ�Ĵ�����Ϣ
    @type msg: str                     

    @note: [example] s����Ϊ�����׳��쳣\n
        checker = Check();
        checker.AssertFalse(1,"���ǰ��һ������Ϊ�棬���������Ϣ"); \n            
        checker.AssertFalse(True,"���ǰ��һ������Ϊ�棬���������Ϣ"); \n            
        checker.AssertFalse('ads',"���ǰ��һ������Ϊ�棬���������Ϣ"); \n            
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
    @param s:��������      
    @type s: str ,int, bool   
    @param t:�б���� 
    @type t: list 
    @param msg:����������ʱ�Ĵ�����Ϣ
    @type msg: str                     

    @note: [example] �ж�s�Ƿ�����t, ���t�в�����s,���׳��쳣\n
        checker = Check();
        checker.AssertIn(1,[3,4,6],"�����һ�����������ڵڶ���������Ӧ�ļ��ϣ����������Ϣ");\n
    '''
    if t.count(s) == 0:
        if msg == '':
            raise CheckerException("t do not contain s.type(s)=%s,s=%s,type(t),t=%s" % (type(s),s,type(t),t));
        else:
            raise CheckerException(msg);
    return;

def AssertNotIn(s,t,msg=''):
    '''
    @param s:��������      
    @type s: str ,int, bool   
    @param t:�б���� 
    @type t: list 
    @param msg:����������ʱ�Ĵ�����Ϣ
    @type msg: str                     

    @note: [example] �ж�s�Ƿ�����t, ���t�а���s,���׳��쳣\n
        checker = Check();
        checker.AssertIn(1,[3,4,6],"�����һ���������ڵڶ���������Ӧ�ļ��ϣ����������Ϣ");\n
    '''
    if t.count(s) != 0:
        if msg == '':
            raise CheckerException("t contain s.type(s)=%s,s=%s,type(t),t=%s" % (type(s),s,type(t),t));
        else:
            raise CheckerException(msg);
    return;

def AssertInclude(s,t,msg=''):
    '''
    @param s:�б�1      
    @type s: list
    @param t:�б�2 
    @type t: list 
    @param msg:����������ʱ�Ĵ�����Ϣ
    @type msg: str                     

    @note: [example] �ж�s�Ƿ����t, ���s�в�����t,���׳��쳣\n
        checker = Check();
        checker.AssertInclude([1,2,3,4,6],[1,2],"����ڶ���������Ӧ���б�������һ��������Ӧ���б����������Ϣ");\n       
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
    @param s:�б�1      
    @type s: list
    @param t:�б�2
    @type t: list 
    @param msg:����������ʱ�Ĵ�����Ϣ
    @type msg: str                     

    @note: [example] �ж�s�Ƿ񲻰���t, ���s�а���t,���׳��쳣\n
        checker = Check();
        checker.AssertExclude([3,4,6],[1,2],"�����һ��������Ӧ���б�����ڶ���������Ӧ���б����������Ϣ");\n
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


