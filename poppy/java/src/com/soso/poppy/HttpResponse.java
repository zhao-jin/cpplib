/*
 * @(#)HttpResponse.java	
 * 
 * Copyright 2011, Tencent Inc.
 * Author: Dongping HUANG <dphuang@tencent.com>
 */

package com.soso.poppy;

class ResponseStatus {
    public int statusCode;
    public String statusMessage;
    public String statusDescription;

    public ResponseStatus(int statusCode, String statusMessage,
            String statusDescription) {
        this.statusCode = statusCode;
        this.statusMessage = statusMessage;
        this.statusDescription = statusDescription;
    }
}

public class HttpResponse extends HttpMessage {
    private int status;

    private static ResponseStatus[] S_RESP_STAT = new ResponseStatus[] {
            new ResponseStatus(100, "Continue",
                    "Request received, please continue"),
            new ResponseStatus(101, "Switching Protocols",
                    "Switching to new protocol; obey Upgrade header"),
            new ResponseStatus(200, "OK", "Request fulfilled, document follows"),
            new ResponseStatus(201, "Created", "Document created, URL follows"),
            new ResponseStatus(202, "Accepted",
                    "Request accepted, processing continues off-line"),
            new ResponseStatus(203, "Non-Authoritative Information",
                    "Request fulfilled from cache"),
            new ResponseStatus(204, "No Content",
                    "Request fulfilled, nothing follows"),
            new ResponseStatus(205, "Reset Content",
                    "Clear input form for further input."),
            new ResponseStatus(206, "Partial Content",
                    "Partial content follows."),
            new ResponseStatus(300, "Multiple Choices",
                    "Object has several resources -- see URI list"),
            new ResponseStatus(301, "Moved Permanently",
                    "Object moved permanently -- see URI list"),
            new ResponseStatus(302, "Found",
                    "Object moved temporarily -- see URI list"),
            new ResponseStatus(303, "See Other",
                    "Object moved -- see Method and URL list"),
            new ResponseStatus(304, "Not Modified",
                    "Document has not changed since given time"),
            new ResponseStatus(305, "Use Proxy",
                    "You must use proxy specified in Location to access this resource."),
            new ResponseStatus(307, "Temporary Redirect",
                    "Object moved temporarily -- see URI list"),
            new ResponseStatus(400, "Bad Request",
                    "Bad request syntax or unsupported method"),
            new ResponseStatus(401, "Unauthorized",
                    "No permission -- see authorization schemes"),
            new ResponseStatus(402, "Payment Required",
                    "No payment -- see charging schemes"),
            new ResponseStatus(403, "Forbidden",
                    "Request forbidden -- authorization will not help"),
            new ResponseStatus(404, "Not Found",
                    "Nothing matches the given URI"),
            new ResponseStatus(405, "Method Not Allowed",
                    "Specified method is invalid for this resource."),
            new ResponseStatus(406, "Not Acceptable",
                    "URI not available in preferred format."),
            new ResponseStatus(407, "Proxy Authentication Required",
                    "You must authenticate with this proxy before proceeding."),
            new ResponseStatus(408, "Request Timeout",
                    "Request timed out; try again later."),
            new ResponseStatus(409, "Conflict", "Request conflict."),
            new ResponseStatus(410, "Gone",
                    "URI no longer exists and has been permanently removed."),
            new ResponseStatus(411, "Length Required",
                    "Client must specify Content-Length."),
            new ResponseStatus(412, "Precondition Failed",
                    "Precondition in headers is false."),
            new ResponseStatus(413, "Request Entity Too Large",
                    "Entity is too large."),
            new ResponseStatus(414, "Request-URI Too Long", "URI is too long."),
            new ResponseStatus(415, "Unsupported Media Type",
                    "Entity body in unsupported format."),
            new ResponseStatus(416, "Requested Range Not Satisfiable",
                    "Cannot satisfy request range."),
            new ResponseStatus(417, "Expectation Failed",
                    "Expect condition could not be satisfied."),
            new ResponseStatus(500, "Internal Server Error",
                    "Server got itself in trouble"),
            new ResponseStatus(501, "Not Implemented",
                    "Server does not support this operation"),
            new ResponseStatus(502, "Bad Gateway",
                    "Invalid responses from another server/proxy."),
            new ResponseStatus(503, "Service Unavailable",
                    "The server cannot process the request due to a high load"),
            new ResponseStatus(504, "Gateway Timeout",
                    "The gateway server did not receive a timely response"),
            new ResponseStatus(505, "HTTP Version Not Supported",
                    "Cannot fulfill request."),
            new ResponseStatus(-1, null, null), };

    public HttpResponse() {
        this.status = -1;
    }

    public void reset() {
        super.reset();
        this.status = -1;
    }

    public int getStatus() {
        return status;
    }

    public void setStatus(int status) {
        this.status = status;
    }

    private static String getStatusMessage(int status_code) {
        for (int i = 0; ; ++i) {
            if (S_RESP_STAT[i].statusCode == -1) {
                return null;
            }
            if (S_RESP_STAT[i].statusCode == status_code) {
                return S_RESP_STAT[i].statusMessage;
            }
        }

    }

    public String generateStartLine() {
        // status line
        StringBuilder sb = new StringBuilder();
        sb.append(getVersionString(httpVersion));
        sb.append(" ");
        sb.append(Integer.toString(this.status));
        sb.append(" ");
        sb.append(getStatusMessage(this.status));
        return sb.toString();
    }

    public ErrorType parseStartLine(String data) {
        String[] fields = data.split(" ");
        if (fields.length < 2) {
            return ErrorType.ERROR_START_LINE_NOT_COMPLETE;
        }

        httpVersion = getVersionNumber(fields[0]);
        if (httpVersion == Version.VERSION_UNKNOWN) {
            return ErrorType.ERROR_VERSION_UNSUPPORTED;
        }

        this.status = Integer.parseInt(fields[1]);
        if (getStatusMessage(status) == null) {
            return ErrorType.ERROR_RESPONSE_STATUS_NOT_FOUND;
        }

        return ErrorType.ERROR_NORMAL;
    }
}
