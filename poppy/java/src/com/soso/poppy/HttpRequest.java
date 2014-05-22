/*
 * @(#)HttpRequest.java	
 * 
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

class MethodName {
    int method;
    String methodName;

    public MethodName(int method, String methodName) {
        this.method = method;
        this.methodName = methodName;
    }
}

public class HttpRequest extends HttpMessage {
    private int methodId;
    private String path;

    public static class MethodType {
        public static int METHOD_UNKNOWN = -1;
        public static int METHOD_HEAD = 0;
        public static int METHOD_GET = 1;
        public static int METHOD_POST = 2;
        public static int METHOD_PUT = 3;
        public static int METHOD_DELETE = 4;
        public static int METHOD_OPTIONS = 5;
        public static int METHOD_TRACE = 6;
        public static int METHOD_CONNECT = 7;
        public static int METHOD_UPPER_BOUND = 100;
    };

    @Override
    public void reset() {
        super.reset();
        this.methodId = MethodType.METHOD_UNKNOWN;
        this.path = "/";
    }

    private static MethodName[] S_METHOD_NAMES = new MethodName[] {
            new MethodName(MethodType.METHOD_HEAD, "HEAD"),
            new MethodName(MethodType.METHOD_GET, "GET"),
            new MethodName(MethodType.METHOD_POST, "POST"),
            new MethodName(MethodType.METHOD_PUT, "PUT"),
            new MethodName(MethodType.METHOD_CONNECT, "CONNECT"),
            new MethodName(MethodType.METHOD_DELETE, "DELETE"),
            new MethodName(MethodType.METHOD_OPTIONS, "OPTIONS"),
            new MethodName(MethodType.METHOD_TRACE, "TRACE"),
            new MethodName(MethodType.METHOD_UNKNOWN, null), };

    // static
    public static int getMethodByName(String methodName) {
        for (int i = 0;; ++i) {
            if (S_METHOD_NAMES[i].methodName == null) {
                return MethodType.METHOD_UNKNOWN;
            }
            // Method is case sensitive.
            if (methodName.compareTo(S_METHOD_NAMES[i].methodName) == 0) {
                return S_METHOD_NAMES[i].method;
            }
        }
    }

    // static
    public static String getMethodName(int method) {
        if (method <= MethodType.METHOD_UNKNOWN
                || method >= MethodType.METHOD_UPPER_BOUND) {
            return null;
        }
        return S_METHOD_NAMES[method].methodName;
    }

    @Override
    public ErrorType parseStartLine(String data) {
        String[] fields = data.split(" ");
        if (fields.length != 2 && fields.length != 3) {
            return ErrorType.ERROR_START_LINE_NOT_COMPLETE;
        }

        this.methodId = getMethodByName(fields[0]);
        if (this.methodId == MethodType.METHOD_UNKNOWN) {
            return ErrorType.ERROR_METHOD_NOT_FOUND;
        }
        this.path = fields[1];

        if (fields.length == 3) {
            httpVersion = getVersionNumber(fields[2]);
            if (httpVersion == Version.VERSION_UNKNOWN) {
                return ErrorType.ERROR_VERSION_UNSUPPORTED;
            }
        }
        return ErrorType.ERROR_NORMAL;
    }

    @Override
    public String generateStartLine() {
        // request line
        StringBuilder sb = new StringBuilder();
        sb.append(getMethodName(this.methodId));
        sb.append(" ");
        sb.append(this.path);
        sb.append(" ");
        sb.append(getVersionString(httpVersion));
        return sb.toString();
    }
}
