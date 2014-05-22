#!/usr/bin/python
# -*- coding: gb2312 -*- 

from pybft import *;
from string import *;

class MyStr(object):
    def __init__(self,bft,repr):
        object.__setattr__(self,'bft',bft)
        object.__setattr__(self,'repr',repr)

    def __str__(self):
        repr = object.__getattribute__(self,'repr')
        bft = object.__getattribute__(self,'bft') 
        return bft.get(repr); 

    def __repr__(self):
        return object.__getattribute__(self,'repr')

    def __getattribute__(self,name):
        repr = object.__getattribute__(self,'repr')
        bft = object.__getattribute__(self,'bft')
        curClass = object.__getattribute__(self,'__class__')
        return curClass(bft,repr + '.' + name)

    def __setattr__(self,name,value):
        repr = object.__getattribute__(self,'repr')
        bft = object.__getattribute__(self,'bft')
        repr += '.' + name
        bft.set(repr,str(value));

    def __getitem__(self,key):
        repr = object.__getattribute__(self,'repr')
        bft = object.__getattribute__(self,'bft')
        curClass = object.__getattribute__(self,'__class__')
        return curClass(bft,repr + '[' + str(key) + ']')

    def __setitem__(self, key, value):
        repr = object.__getattribute__(self,'repr')
        bft = object.__getattribute__(self,'bft')
        repr += '[' + str(key) + ']'
        bft.set(repr,str(value));

class BFT(object):
    def __init__(self,*args):
        if len(args) == 0:
            object.__setattr__(self,'bft',CBFT())
        elif len(args) == 1:
            if isinstance(args[0],Protocol):
                p = object.__getattribute__(args[0],'protocol')
                b = object.__getattribute__(p,'bft')
                object.__setattr__(self,'bft',CBFT(b));
            else:
                object.__setattr__(self,'bft',CBFT(args[0]));
        else:
            object.__setattr__(self,'bft',CBFT(args[0],args[1]));

    def __getattribute__(self,name):
        try:
            bft = object.__getattribute__(self,'bft')
            return object.__getattribute__(bft,name)
        except:
            bft = object.__getattribute__(self,'bft')
            return MyStr(bft,name)

class  Protocol(object):
    def __init__(self,s,isBig=False):
        if s != None:
            if(isinstance(s,str)):
                object.__setattr__(self,'protocol',BFT(s,isBig))
            else:
                object.__setattr__(self,'protocol',BFT(s))
        else:
            object.__setattr__(self,'protocol',BFT())

    def __getattribute__(self,name):
        try:
            return object.__getattribute__(self,name)
        except Exception,e:
            p = object.__getattribute__(self,'protocol')
            return BFT.__getattribute__(p,name)
            pass

    def __setattr__(self,name,value):
        try:
            p = object.__getattribute__(self,'protocol')
            if not p.hasKey(name):
            #if(p.get(name) == None):
                object.__setattr__(self,name,value);
            else:
                p.set(name,str(value));
        except:
            object.__setattr__(self,name,value);


