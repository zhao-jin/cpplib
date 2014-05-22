/*
 * @(#)HttpMessage.java
 *
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map;

class HttpVersion {
    public int versionNumber;
    public String versionString;

    public HttpVersion(int versionNumber, String versionString) {
        this.versionNumber = versionNumber;
        this.versionString = versionString;
    }
}

public abstract class HttpMessage {
    public static class Version {
        public static int VERSION_UNKNOWN = 0;
        public static int VERSION_0_9 = 9;
        public static int VERSION_1_0 = 10;
        public static int VERSION_1_1 = 11;

        public int version = VERSION_UNKNOWN;

        Version(int version) {
            this.version = version;
        }

        public String toString() {
            return Integer.toString(this.version);
        }
    }

    enum ErrorType {
        ERROR_NORMAL,
        ERROR_NO_START_LINE,
        ERROR_START_LINE_NOT_COMPLETE,
        ERROR_VERSION_UNSUPPORTED,
        ERROR_RESPONSE_STATUS_NOT_FOUND,
        ERROR_FIELD_NOT_COMPLETE,
        ERROR_METHOD_NOT_FOUND;
    };

    protected int httpVersion;
    private List<Map.Entry<String, String>> headers;
    private byte[] httpBody;

    static HttpVersion[] s_http_versions = new HttpVersion[] {
            new HttpVersion(Version.VERSION_0_9, "HTTP/0.9"),
            new HttpVersion(Version.VERSION_1_0, "HTTP/1.0"),
            new HttpVersion(Version.VERSION_1_1, "HTTP/1.1"),
            new HttpVersion(Version.VERSION_UNKNOWN, null) };

    public HttpMessage() {
        this.httpVersion = Version.VERSION_1_1;
        this.headers = new ArrayList<Map.Entry<String, String>>();
    }

    public void reset() {
        this.httpVersion = Version.VERSION_1_1;
        this.httpBody = null;
        this.headers.clear();
    }

    public abstract String generateStartLine();

    public abstract ErrorType parseStartLine(String data);

    public String headersToString() {
        StringBuilder sb = new StringBuilder();
        sb.append(generateStartLine());
        sb.append("\r\n");

        for (Iterator<Map.Entry<String, String>> it = this.headers.iterator();
                it.hasNext(); ) {
            Map.Entry<String, String> entry = it.next();
            sb.append(entry.getKey());
            sb.append(": ");
            sb.append(entry.getValue());
            sb.append("\r\n");
        }
        sb.append("\r\n");
        return sb.toString();
    }

    /*
     * Get a header value. return false if it does not exist.
     * the header name is not case sensitive.
     */
    public boolean getHeader(String header_name, String header_value) {
        for (Iterator<Map.Entry<String, String>> it = this.headers.iterator();
                it.hasNext(); ) {
            Map.Entry<String, String> entry = it.next();
            if (header_name.compareToIgnoreCase(entry.getKey()) == 0) {
                header_value = entry.getValue();
                return true;
            }
        }
        return false;
    }

    /*
     * Used when a http header appears multiple times.
     * return false if it doesn't exist.
     */
    public boolean getHeaders(String header_name, List<String> header_values) {
        header_values.clear();
        for (Iterator<Map.Entry<String, String>> it = this.headers.iterator();
                it.hasNext(); ) {
            Map.Entry<String, String> entry = it.next();
            if (header_name.compareToIgnoreCase(entry.getKey()) == 0) {
                header_values.add(entry.getValue());
            }
        }
        return !header_values.isEmpty();
    }

    // Set a header field. if it exists, overwrite the header value.
    public void setHeader(String header_name, String header_value) {
        boolean field_exists = false;
        for (Iterator<Map.Entry<String, String>> it = this.headers.iterator();
                it.hasNext();) {
            Map.Entry<String, String> entry = it.next();
            if (header_name.compareToIgnoreCase(entry.getKey()) == 0) {
                if (field_exists == true) {
                    it.remove();
                } else {
                    entry.setValue(header_value);
                }
            }
        }
        if (!field_exists) {
            // the specific field doesn't exist.
            addHeader(header_name, header_value);
        }
    }

    // Add a header field, just append, no overwrite.
    public void addHeader(String header_name, String header_value) {
        this.headers.add(new Pair<String, String>(header_name, header_value));
    }

    public void removeHeader(String header_name) {
        for (Iterator<Map.Entry<String, String>> it = this.headers.iterator();
                it.hasNext();) {
            Map.Entry<String, String> entry = it.next();
            if (header_name.compareToIgnoreCase(entry.getKey()) == 0) {
                it.remove();
            }
        }
    }

    String getVersionString(int version) {
        for (int i = 0;; ++i) {
            if (s_http_versions[i].versionNumber == Version.VERSION_UNKNOWN) {
                return null;
            }
            if (version == s_http_versions[i].versionNumber) {
                return s_http_versions[i].versionString;
            }
        }
    }

    int getVersionNumber(String http_version) {
        for (int i = 0;; ++i) {
            if (s_http_versions[i].versionString == null) {
                return Version.VERSION_UNKNOWN;
            }
            if (s_http_versions[i].versionString
                    .compareToIgnoreCase(http_version) == 0) {
                return s_http_versions[i].versionNumber;
            }
        }
    }

    // Parse http headers (including the start line) from data.
    // return: error code which is defined as ErrorType.
    public ErrorType parseHeaders(String data) {
        this.headers.clear();
        // using "\r\n" as delimiter
        String[] lines = data.split("\r\n");
        if (lines.length == 0) {
            return ErrorType.ERROR_NO_START_LINE;
        }

        ErrorType retval = parseStartLine(lines[0]);
        if (retval != ErrorType.ERROR_NORMAL) {
            return retval;
        }
        // Skip the head line and the last line(empty but '\n')
        for (int i = 1; i < lines.length - 1; ++i) {
            String[] tokens = lines[i].split(":");
            if (tokens.length != 2) {
                return ErrorType.ERROR_FIELD_NOT_COMPLETE;
            }
            this.headers.add(
                    new Pair<String, String>(tokens[0].trim(), tokens[1].trim()));
        }
        return ErrorType.ERROR_NORMAL;
    }

    public int getContentLength() {
        String content_length = null;
        if (!getHeader("Content-Length", content_length)) {
            return -1;
        }
        int length = Integer.parseInt(content_length);
        return length >= 0 ? length : -1;
    };

    boolean isKeepAlive() {
        if (this.httpVersion < Version.VERSION_1_1) {
            return false;
        }
        String alive = null;
        if (!getHeader("Connection", alive)) {
            return true;
        }
        return ("keep-alive".compareToIgnoreCase(alive) == 0) ? true : false;
    }

    public int getHttpVersion() {
        // return m_http_version.m_version;
        return this.httpVersion;
    }

    public void setHttpVersion(int version) {
        this.httpVersion = version;
    }

    public byte[] getHttpBody() {
        return this.httpBody;
    }

    public void setHttpBody(byte[] body) {
        this.httpBody = body;
    }
}
