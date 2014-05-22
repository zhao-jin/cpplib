# -*- coding: gb2312 -*- 

'''
conf module
@authors  :   U{raymondxie<mailto: raymondxie@tencent.com>}
@copyright:   tencent
@date     :   2010-07-16
@version  :   1.0.0.0
'''

from StringIO import StringIO

class MStringIO(StringIO):
    def open(self):
        self.seek(0);

    def clear(self):
        self.truncate(0);
        self.flush()
        self.rewind()

    def save(self,fn):
        fid = open(fn,'w');
        self.seek(0);
        fid.write(self.read());
        fid.close();

    def rewind(self):
        self.seek(0);
        
