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
    @param t: 整型 时间
    @type t: int     
    @return: 字符串时间, 格式: YYYY-MM-DD hh:mm:ss
    @rtype: str
    @note: [example]
          print getStrTime(1266632155)   -> "2010-02-20 10:15:55"    
    '''
    return time.strftime(ISOTIMEFORMAT, time.localtime(int(t)));

def getIntTime(s):
    '''
    @param s:字符串时间, 格式: YYYY-MM-DD hh:mm:ss
    @type s: str     
    @return:  整型 时间
    @rtype: int
    @note: [example]
          print getIntTime("2010-02-20 10:15:55")   -> 1266632155  
    '''
    return int(time.mktime(time.strptime(s,ISOTIMEFORMAT)));

'''
----------------------------------------------------------------------------
%a 本地的星期缩写
%A 本地的星期全称
%b 本地的月份缩写
%B 本地的月份全称
%c 本地的合适的日期和时间表示形式
%d 月份中的第几天，类型为decimal number（10进制数字），范围[01,31]
%f 微秒，类型为decimal number，范围[0,999999]，Python 2.6新增
%H 小时（24进制），类型为decimal number，范围[00,23]
%I 小时（12进制），类型为decimal number，范围[01,12]
%j 一年中的第几天，类型为decimal number，范围[001,366]
%m 月份，类型为decimal number，范围[01,12]
%M 分钟，类型为decimal number，范围[00,59]
%p 本地的上午或下午的表示（AM或PM），只当设置为%I（12进制）时才有效
%S 秒钟，类型为decimal number，范围[00,61]（60和61是为了处理闰秒）
%U 一年中的第几周（以星期日为一周的开始），类型为decimal number，范围[00,53]。在度过新年时，直到一周的全部7天都在该年中时，才计算为第0周。只当指定了年份才有效。
%w 星期，类型为decimal number，范围[0,6]，0为星期日
%W 一年中的第几周（以星期一为一周的开始），类型为decimal number，范围[00,53]。在度过新年时，直到一周的全部7天都在该年中时，才计算为第0周。只当指定了年份才有效。
%x 本地的合适的日期表示形式
%X 本地的合适的时间表示形式
%y 去掉世纪的年份数，类型为decimal number，范围[00,99]
%Y 带有世纪的年份数，类型为decimal number
%Z 时区名字（不存在时区时为空）
%% 代表转义的"%"字符
----------------------------------------------------------------------------
'''
    
