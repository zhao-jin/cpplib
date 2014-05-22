# -*- coding: gbk -*- 

'''
a2b module
@authors  :   U{raymondxie<mailto: raymondxie@tencent.com>}
@copyright:   tencent
@date     :   2010-07-14
@version  :   1.0.0.0
'''

import time;
import struct;
import socket;
import random;


def ip2int(ip):    
    """
    @param ip: 字符串 ip,格式:xxx.xxx.xxx.xxx
    @type ip: str     
    @return: 整型ip
    @rtype: int
    @note: [example]
          print ip2int("172.29.1.1")  -> 16850348
    """
    return struct.unpack("<I",socket.inet_aton(ip))[0];

def int2ip(s):
    '''
    @param s: 整型 ip
    @type s: int     
    @return: 字符串ip, 格式:xxx.xxx.xxx.xxx
    @rtype: str
    @note: [example]
          print int2ip(16850348)   -> "172.29.1.1"    
    '''
    return str(socket.inet_ntoa(struct.pack("<I",int(s))));

def longStr2Twoint(s):
    '''
    @param s: 8字节字符串
    @type s: str     
    @return: (int,int)
    @rtype: tuple
    @note: [example]
        print longStr2Twoint("12345678")  -> (875770417, 943142453)
    '''
    return struct.unpack("ii", str(s)) 

def twoint2longStr(a,b):
    '''
    @param a: 
    @type s: int  
    @param b: 
    @type s: int  
    @return: str
    @rtype: 8字节字符串
    @note: [example]
        print twoint2longStr(875770417, 943142453) -> "12345678"
    '''
    return struct.pack("ii", a, b);

def byte2hex(s):
    '''
    @param s: 二进制字节流
    @type s: byte buffer  
    @return: 格式化的字符串
    @rtype: str
    @note: [example]\n
        s=struct.pack("i",1234)\n    
        print s -> '\\xd2\\x04\\x00\\x00'
        print byte2hex(s) -> \'d2 04 00 00\'
    '''
    return ''.join( [ "%02x " % ord( x ) for x in s ] ).strip();

def hex2byte(hexStr):
    '''
    @param hexStr: 格式化的字符串
    @type hexStr:  str
    @return: 二进制字节流
    @rtype: byte buffer 
    @note: [example]\n
        s='d2 04 00 00' \n    
        print hex2byte(s) -> '\\xd2\\x04\\x00\\x00'
    '''
    bytes=[];
    hexStr = ''.join(hexStr.split(' '));
    for i in range(0,len(hexStr),2):
        bytes.append(chr(int(hexStr[i:i+2],16)));

    return ''.join(bytes);
