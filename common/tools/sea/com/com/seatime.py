# -*- coding: gbk -*- 

'''
seatime module
@authors  :   U{raymondxie<mailto: raymondxie@tencent.com>}
@copyright:   tencent
@date     :   2010-07-14
@version  :   1.0.0.0
'''

import time;

ISOTIMEFORMAT='%Y-%m-%d %X'; 

def getStrTime(t):
    '''
    @param t: ���� ʱ��
    @type t: int     
    @return: �ַ���ʱ��, ��ʽ: YYYY-MM-DD hh:mm:ss
    @rtype: str
    @note: [example]
          print getStrTime(1266632155)   -> "2010-02-20 10:15:55"    
    '''
    return time.strftime(ISOTIMEFORMAT, time.localtime(int(t)));

def getIntTime(s):
    '''
    @param s:�ַ���ʱ��, ��ʽ: YYYY-MM-DD hh:mm:ss
    @type s: str     
    @return:  ���� ʱ��
    @rtype: int
    @note: [example]
          print getIntTime("2010-02-20 10:15:55")   -> 1266632155  
    '''
    return int(time.mktime(time.strptime(s,ISOTIMEFORMAT)));

'''
----------------------------------------------------------------------------
%a ���ص�������д
%A ���ص�����ȫ��
%b ���ص��·���д
%B ���ص��·�ȫ��
%c ���صĺ��ʵ����ں�ʱ���ʾ��ʽ
%d �·��еĵڼ��죬����Ϊdecimal number��10�������֣�����Χ[01,31]
%f ΢�룬����Ϊdecimal number����Χ[0,999999]��Python 2.6����
%H Сʱ��24���ƣ�������Ϊdecimal number����Χ[00,23]
%I Сʱ��12���ƣ�������Ϊdecimal number����Χ[01,12]
%j һ���еĵڼ��죬����Ϊdecimal number����Χ[001,366]
%m �·ݣ�����Ϊdecimal number����Χ[01,12]
%M ���ӣ�����Ϊdecimal number����Χ[00,59]
%p ���ص����������ı�ʾ��AM��PM����ֻ������Ϊ%I��12���ƣ�ʱ����Ч
%S ���ӣ�����Ϊdecimal number����Χ[00,61]��60��61��Ϊ�˴������룩
%U һ���еĵڼ��ܣ���������Ϊһ�ܵĿ�ʼ��������Ϊdecimal number����Χ[00,53]���ڶȹ�����ʱ��ֱ��һ�ܵ�ȫ��7�춼�ڸ�����ʱ���ż���Ϊ��0�ܡ�ֻ��ָ������ݲ���Ч��
%w ���ڣ�����Ϊdecimal number����Χ[0,6]��0Ϊ������
%W һ���еĵڼ��ܣ�������һΪһ�ܵĿ�ʼ��������Ϊdecimal number����Χ[00,53]���ڶȹ�����ʱ��ֱ��һ�ܵ�ȫ��7�춼�ڸ�����ʱ���ż���Ϊ��0�ܡ�ֻ��ָ������ݲ���Ч��
%x ���صĺ��ʵ����ڱ�ʾ��ʽ
%X ���صĺ��ʵ�ʱ���ʾ��ʽ
%y ȥ�����͵������������Ϊdecimal number����Χ[00,99]
%Y �������͵������������Ϊdecimal number
%Z ʱ�����֣�������ʱ��ʱΪ�գ�
%% ����ת���"%"�ַ�
----------------------------------------------------------------------------
'''
    
