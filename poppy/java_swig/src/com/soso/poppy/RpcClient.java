package com.soso.poppy;

import java.net.URL;
import java.util.zip.ZipFile;
import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;

public class RpcClient extends com.soso.poppy.swig.RpcClient {
    final static String libraryName = "libpoppy_client_java.so";
    static {
        // load library.
        // The library 'libraryName' must be loaded before any JNI operation.
        loadLibrary();
    }
    static void loadLibrary(){
        try {
            // get the class object for this class, and get the location of it
            final Class c = RpcClient.class;
            final URL location = c.getProtectionDomain().getCodeSource().getLocation();

            // jars are just zip files, get the input stream for the lib
            ZipFile zf = new ZipFile(location.getPath());
            InputStream in = zf.getInputStream(zf.getEntry(libraryName));

            // create a temp file and an input stream for it
            File f = File.createTempFile("JARLIB-", libraryName);
            FileOutputStream out = new FileOutputStream(f);

            // copy the lib to the temp file
            byte[] buf = new byte[1024];
            int len;
            while ((len = in.read(buf)) > 0)
                out.write(buf, 0, len);
            in.close();
            out.close();

            // load the lib specified by its absolute path and delete it
            System.load(f.getAbsolutePath());
            f.delete();

        } catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
    }

    public RpcClient() {
        super();
    }

    public void start() {
    }

    public void stop() throws Exception {
    }
}

