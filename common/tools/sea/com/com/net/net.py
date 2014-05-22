# -*- coding: gbk -*-

'''
@authors  :   U{raymondxie<mailto: raymondxie@tencent.com>}
@copyright:   tencent
@date     :   2010-07-19
@version  :   1.0.0.0
'''

import os;
import re;
import sys;
import time;
import socket;
import select;
import struct;
import threading;

import com.share

from datetime import datetime;
from com.exception import ComException;
from com.baselog import NetLog;
try:
    from common.bft.bft import open_bft_log;
except:
    pass

com.share.netLogger = NetLog('netlib.log');    

class SendException(ComException):
    pass;

class RecvException(ComException):
    pass;    

class SendTimeoutException(ComException):
    pass;  

class RecvTimeoutException(ComException):
    pass;  
    
class ResetException(ComException):
    pass;   

class NetException(ComException):
    pass;      
    
netLogger = com.share.netLogger;

class Sender(object):
    def send(self, m,s,buf):
        socketInfo = "";
        bufLen = len(buf);
        sendBytes = 0;
        
        try:
            if isinstance (m, Server):
                socketInfo = str( s.getpeername() );
            else:
                socketInfo = "("+ m.ip+ ", "+  str(m.port) + ")";

            netLogger.info("[%s] begin to send %d bytes from fd:%d ..." %(m.name,bufLen,s.fileno())); 
            while  sendBytes < bufLen:
                sent = s.send( buf[sendBytes:] );
                if sent == 0:
                    sent = s.send( buf[sendBytes:] );
                    if sent == 0:                
                        netLogger.warn("[%s] socket fd:%d is broken." %(m.name,s.fileno()));
                        sfn = s.fileno();
                        s.shutdown(socket.SHUT_RDWR);   
                        s.close();
                        raise ResetException,"[%s] socket fd:%d is broken." %(m.name,sfn);  
                sendBytes += sent;
        except socket.error, e:
            netLogger.error("[%s] send %d bytes fail." %(m.name,bufLen));        
            raise SendException, "[%s] send to %s %d bytes fail.socketfd:%d. %s" %(m.name,socketInfo,bufLen,s.fileno(), str(e));

        except socket.timeout, e:
            netLogger.error("[%s] send %d bytes timeout." %(m.name,bufLen));        
            raise SendTimeoutException, "[%s] send to %s %d bytes timeout.socketfd:%d. %s" %(m.name,socketInfo,bufLen,s.fileno(), str(e));

        netLogger.info("[%s] send %d bytes succ." %(m.name,bufLen));
    
class Receiver(object):
    def recv(self, m,s, size):
        returnBuf = "";
        socketInfo = "";
        recvBytes = 0;
        buf='';

        try:
            if isinstance (m, Server):
                socketInfo = str( s.getpeername() );
            else:
                socketInfo = "("+ m.ip+ ", "+  str(m.port) + ")";

            netLogger.info("[%s] begin to recv %d bytes from fd:%d ..." %(m.name,size,s.fileno()));                            
            
            while  recvBytes < size:
                returnBuf = s.recv( size - recvBytes );
                recvLen = len(returnBuf);
                if recvLen == 0:
                    returnBuf = s.recv( size - recvBytes );
                    recvLen = len(returnBuf);
                    if recvLen == 0: 
                        netLogger.warn("[%s] socket fd:%d is broken." %(m.name,s.fileno()));
                        sfn = s.fileno();
                        s.shutdown(socket.SHUT_RDWR);   
                        s.close();
                        raise ResetException,"[%s] socket fd:%d is broken." %(m.name,sfn);             
                recvBytes += recvLen;
                buf += returnBuf;

        except socket.error, e:
            netLogger.error("[%s] recv %d bytes fail.%s" %(m.name,size,str(e)));        
            raise RecvException, "[%s] recv from %s %d bytes fail. socketfd:%d. %s" %(m.name,socketInfo,size,s.fileno(), str(e));

        except socket.timeout, e:
            netLogger.error("[%s] recv %d bytes timeout." %(m.name,size));
            raise RecvTimeoutException, "[%s] recv from %s %d bytes timeout. socketfd:%d. %s" %(m.name,socketInfo,size, 
                                                                          s.fileno(),
                                                                          str(e));
        netLogger.info("[%s] recv %d bytes succ." %(m.name,size));
        return buf;

class SendProtocol(object):
     def sendAll(self,mock,sock,buf):
         mock.sender.send(mock,sock,buf);

class RecvProtocol(object):
    def recvAll(self,mock,sock,headLen= 4,bodyOffset=None,bodyOffsetSize=None,totalOffset=None,totalOffsetSize=None,endian='LITTLE'):
        '''
        @param mock: 模拟端句柄，可以是Server或Client的实例
        @type mock: instance  
        @param sock: socket对象
        @type sock: instance    
        @param headLen: 包头长度
        @type headLen: int 
        @param bodyOffset: 包体长度字段在包头的偏移
        @type bodyOffset: int 
        @param bodyOffsetSize: 包体长度字段的长度
        @type bodyOffsetSize: int 
        @param totalOffset: 整包长度字段在包头的偏移
        @type totalOffset: int 
        @param totalOffsetSize: 整包长度字段的长度
        @type totalOffsetSize: int         
        @param endian: 大小端字符串标识,默认为'LITTLE',表示小端,'BIG'表示大端
        @type endian: str 
       
        @return: 接收到的二进制字节流
        @rtype: byte buffer            
        @note: [example] body和total不能同时生效\n
            rp = RecvProtocol() \n
            rp.recvAll(m,s,headLen=8,bodyOffset=4,bodyOffsetSize=4) \n
            rp.recvAll(m,s,headLen=8,totalOffset=0,totalOffsetSize=4) \n
        '''  
        flag = 'I';
        offsetFlag = 'BODY'; #['BODY','TOTAL']
        if totalOffset and totalOffsetSize:
            offsetFlag = 'TOTAL';
            offset = totalOffset
            offsetSize = totalOffsetSize
        else:
            offset = bodyOffset
            offsetSize = bodyOffsetSize

        if(offsetSize == 1):
	        flag = 'B';
        elif(offsetSize == 2):
	        flag = 'H';
        elif(offsetSize == 3):
	        flag = 'BH';        
        elif(offsetSize == 4):
	        flag = 'I';
        elif(offsetSize == 5):
          flag = 'BI';
        elif(offsetSize == 6):
 	        flag = 'HI';
        elif(offsetSize == 7):
	        flag = 'BHI';
        elif(offsetSize == 8):
	        flag = 'L';        
        else:
            if offsetFlag == 'BODY':
                raise NetException,"The bodyOffsetSize:%d is not supported" %(bodyOffsetSize,);
    	    else:
    	        raise NetException,"The totalOffsetSize:%d is not supported" %(totalOffsetSize,);    	    

        format = str(offset) + 'x ' + str(offsetSize) + 's ' + str(headLen - offset - offsetSize) + 'x'; 
        # recv head     
        if isinstance (mock, Server):
            buf1 = mock.recv(sock,headLen);
        elif isinstance (mock, Client):
            buf1 = mock.recv(headLen);

        bodyLenBuf = struct.unpack(format,buf1)[0];
        if endian == 'LITTLE':
            bodyLenList = struct.unpack('<' + flag,bodyLenBuf); #the bodyLen is little-endian
        else:
            bodyLenList = struct.unpack('>' + flag,bodyLenBuf); #the bodyLen is big-endian

        bodyLen = 0;
        if flag == 'B' or flag == 'H' or flag == 'I' or flag == 'L':
            bodyLen = bodyLenList[0];
        elif flag == 'BH': #flag 3
            bodyLen = bodyLenList[0] * 256 ** 2 + bodyLenList[1];
        elif flag == 'BI': #flag 5
            bodyLen = bodyLenList[0] * 256 ** 4 + bodyLenList[1];
        elif flag == 'HI': #flag 6
            bodyLen = bodyLenList[0] * 256 ** 4 + bodyLenList[1];
        elif flag == 'BHI': #flag 7
            bodyLen = bodyLenList[0] * 256 ** 6 + bodyLenList[1] * 256 ** 4 + bodyLenList[2];
        
        if offsetFlag == 'TOTAL':
            bodyLen = bodyLen - headLen;
	           
        # recv body
        if isinstance (mock, Server):
            buf2 = mock.recv(sock,bodyLen); 
        elif isinstance (mock, Client):
            buf2 = mock.recv(bodyLen); 

        return ''.join([buf1 ,buf2]);        
	        
class Mock(object):
    def __init__(self ,port ,ip,name="Mock",mode="TCP",keepAlive=False):
        self.port = port
        self.ip = ip
        self.name = name;

        self.sender = Sender()
        self.receiver = Receiver()
        self.sendProtocol = SendProtocol()
        self.recvProtocol = RecvProtocol()        

        self.mode = mode
        self.keepAlive = keepAlive

    def setSender(self, s):
        self.sender = s

    def setReceiver(self, r):
        self.receiver = r

    def setSendProtocol(self, sp):
        self.sendProtocol = s

    def setRecvProtocol(self, rp):
        self.recvProtocol = r

    def getSendProtocol(self):
        return self.sendProtocol

    def getRecvProtocol(self):
        return self.recvProtocol

    def __getattr__(self,name):
        try:
            import re
            if re.search('^recvAll$',name):
                return object.__getattribute__(self.recvProtocol,name);
            if re.search('^sendAll$',name):
                return object.__getattribute__(self.sendProtocol,name);
        except Exception,e:
            return object.__getattribute__(self,name);
            
        
class Server(Mock):
    def __init__(self, port,ip = "",name="",mode="TCP",keepAlive=False,processor=""):
        '''
        @param port:要连接的服务器端口
        @type port: int  
        @param ip:要连接的服务器ip
        @type ip: str     
        @param name:客户端名字
        @type name: str  
        @param mode:客户端模式，有'TCP' 和'UDP'  2种，默认为'TCP'
        @type mode: str 
        @param keepAlive:是否保持连接标识，True表示保持连接，默认为False
        @type keepAlive: bool 
        @param processor:连接处理器
        @type processor: str 

        @note: [example] \n
            ms = Server(port=8888) \n
            ms = Server(port=8888,name="XServer",mode='UDP') \n
        '''         
        if name== "": name = 'Server'
        Mock.__init__(self,port,ip,name,mode,keepAlive);    
        self.port = port;
        self.ip = ip;
        self.timeout = 0.005;

        self.processorDict = {'Selector'     : self.Selector,
                              'Epollor'      : self.Epollor,
                              'ThreadPool'   : self.ThreadPool,
                              'SelectPool'   : self.SelectPool,
                              'EpollPool'    : self.EpollPool};

        try:
            self.processor = self.processorDict[processor];
        except:
            self.processor = self.processorDict['Selector'];
        
        self.onRecv = None;
        self.isStop = False;

    # s : client socket
    # this method must be implemented by the subServer.
    def handleInput(self, s):
        pass

    def start(self, onRecv = None ):
        '''
        @param onRecv:回调函数，格式为def myHandle(mock, sock)
        @type onRecv: func  
          
        @note: [example] 启动服务器\n
            ms = Server(port=8888) \n
            ms.start() \n
            ms.start(onRecv=myHandle) \n
        '''      
        self.isStop = False;
        if onRecv == None:
            onRecv = self.handleInput;
        self.onRecv = onRecv;


        try:
            host = socket.gethostname();
            self.serverSocket = socket.socket(socket.AF_INET,socket.SOCK_STREAM);
            self.serverSocket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR,1);
            self.serverSocket.bind((self.ip, self.port));
            self.serverSocket.listen(10);
            netLogger.info("[%s] %s,fd:%d inited." %(self.name,
                                                           self.serverSocket.getsockname(),
                                                           self.serverSocket.fileno()));
        except socket.error,arg:
            (errno, err_msg) = arg;
            netLogger.error("[%s] %s, errno=%d" % (self.name,err_msg, errno));
            raise Exception,'[%s] port:%d. %s' %(self.name,self.port,str(arg));

        self.inputs = [self.serverSocket];
        self.serverThread = threading.Thread(target = self.processor);
        self.serverThread.setDaemon(1);
        netLogger.info("[%s] started. Service ThreadName:%s" %(self.name,self.serverThread.getName()))        
        self.serverThread.start();


    def closeSocket(self,s):
        try:
            netLogger.info("[%s] socket:%d closed" %(self.name,s.fileno()))
            self.inputs.remove(s);        
            if s != self.serverSocket: 
                s.shutdown(socket.SHUT_RDWR);        
            s.close();
        except Exception,e:
            print 'closeSocket:',e;
            pass;
        
    def stop(self):
        '''          
        @note: [example] stop server, close all connections, exit thread and stop listen\n
            ms = Server(port=8888) \n
            ms.stop() \n
        '''       
        if not self.isStop:
            netLogger.info("[%s] stopped. " %(self.name,));

        self.isStop = True;
        if self.serverThread.isAlive():
            self.serverThread.join();
            
        for s  in self.inputs:
            self.closeSocket(s);


    def restart(self, onRecv = None):
        '''
        @param onRecv:回调函数，格式为def myHandle(mock, sock)
        @type onRecv: func  
          
        @note: [example] 重启服务器\n
            ms = Server(port=8888) \n
            ms.restart() \n
            ms.restart(onRecv=myHandle) \n
        '''  
        if self.isStop == True:
            self.stop();
            self.isStop = False;
        self.start(onRecv);
        netLogger.info("[%s] restarted. " %(self.name,));

    # s : clientsocket
    # buf : data to send 
    def send(self, s, buf ):
        '''
        @param s:socket句柄
        @type s: instance     
        @param buf:发送的二进制字节流
        @type buf: byte buffer        
         
        @note: [example] 发送网络字节\n
            ms = Server(port=8888) \n
            ms.send(sock,buf) #发送buf, 该函数在handleInput回调函数中调用\n
        '''      
        self.sender.send(self ,s , buf);

    def recv(self,s,bufLen):
        '''
        @param s:socket句柄
        @type s: instance     
        @param bufLen:接收字节数
        @type bufLen: int      
        @return: 接收到的二进制字节流
        @rtype: byte buffer  

        @note: [example] 发送网络字节\n
            ms = Server(port=8888) \n
            buf = ms.recv(sock,100) #接收100个字节到buf, 该函数在handleInput回调函数中调用\n
        '''          
        return self.receiver.recv(self ,s,bufLen);

    def Selector(self):
        try:
            open_bft_log();
        except:
            pass    
        
        netLogger.info("[%s] running... " %(self.name,))
        while self.isStop == False:
             rs = []
             try:
                 if self.inputs == None or self.inputs == []:
                     continue;
                 rs, ws, es = select.select(self.inputs, [], [],self.timeout);
                 for r in rs:
                     if r is self.serverSocket:
                         clientsock, addr = self.serverSocket.accept();
                         netLogger.info('[%s] Get connection from:%s,fd:%d' %(self.name, addr,clientsock.fileno()));
                         self.inputs.append(clientsock);
                     else:
                         if self.onRecv == self.handleInput:
                             self.onRecv( r );
                         else:
                             self.onRecv(self, r);  
                         if self.keepAlive == 0:
                             netLogger.info("[%s] close the connection from:%s, fd:%d" % (self.name,addr,clientsock.fileno()));                                                         
                             self.closeSocket(r);
             except ResetException,e:
                 pass; # for reset connection ,doesn't need to process

             except (SendException,RecvException),e0:
                 pass; # for send or recv exception,process in the concrete subclass.
             except AttributeError,e1:
                 pass; 
             except socket.error,e2:
                 (errno, err_msg) = e2;
                 if errno == 9:
                     self.inputs=[self.serverSocket];
             except Exception,e2:
                 import traceback
                 traceback.print_exc();
                 print self.name,self.serverThread.getName(),self.inputs,self.serverSocket
                 raise e2;

    def Epollor(self):
        pass;

    def ThreadPool(self):
        pass;

    def SelectPool(self):
        pass;


    def EpollPool(self):
        pass;
        
class Client(Mock):
    def __init__(self, port,ip = "",name="",mode="TCP",keepAlive=False):
        '''
        @param port:要连接的服务器端口
        @type port: int  
        @param ip:要连接的服务器ip
        @type ip: str     
        @param name:客户端名字
        @type name: str  
        @param mode:客户端模式，有'TCP' 和'UDP'  2种，默认为'TCP'
        @type mode: str 
        @param keepAlive:是否保持连接标识，True表示保持连接，默认为False
        @type keepAlive: bool 
          
        @note: [example] \n
            mc = Client(port=8888) \n
            mc = Client(port=8888,ip="172.23.12.1",name="XClient",mode='UDP') \n
        '''      
        if name== "": name = 'Client'    
        Mock.__init__(self,port,ip,name,mode,keepAlive);  
        self.connected = False;
        netLogger.info("[%s] inited." %(self.name,));


    def connect (self):
        '''
        @note: [example] 连接到服务器\n
            mc = Client(port=8888) \n
            mc.connect() \n
        '''       
        try:
            if self.mode == 'TCP':
                mode = socket.SOCK_STREAM;
            else:
                mode = socket.SOCK_DGRAM;
                
            self.clientSocket = socket.socket( socket.AF_INET, mode );
            self.clientSocket.connect ( (self.ip, self.port) );
            self.connected = True;
        except socket.error, e:
            (errno, err_msg) = e;
            netLogger.error("[%s] %s, errno=%d" % (self.name,err_msg, errno));
            raise e;

    def getSocket(self):
        return self.clientSocket;
        
    def getSocketStatus(self):
        return self.connected;

    def send (self, buf):
        '''
        @param buf:发送的二进制字节流
        @type buf: byte buffer        
         
        @note: [example] 发送网络字节\n
            mc = Client(port=8888) \n
            mc.send(buf) #发送buf\n
        '''       
        self.sender.send(self ,self.clientSocket ,buf);
            
       
    def recv(self, size):
        '''
        @param size:接收字节数
        @type size: int        
        @return: 接收到的二进制字节流
        @rtype: byte buffer            
        @note: [example] 接收网络字节\n
            mc = Client(port=8888) \n
            buf = mc.recv(100) #接收100个字节\n
        '''      
        return self.receiver.recv(self ,self.clientSocket , size);
            
    def closeSocket(self):
        try:
            self.clientSocket = None;
            self.connected = False;
            self.clientSocket.shutdown(socket.SHUT_RDWR);        
            self.clientSocket.close();        
        except:
            pass;
        

