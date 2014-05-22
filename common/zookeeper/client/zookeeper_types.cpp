// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include "common/zookeeper/client/zookeeper_types.h"

namespace common {

const char* ZookeeperErrorString(ZookeeperErrorCode error)
{
    switch (error)
    {
    case ZK_OK:
        return "ok";
    case ZK_INVALIDCLIENTID:
        return "client id is not valid";
    case ZK_INVALIDDATA:
        return "node data is not valid";
    case ZK_ALREADYLOCKED:
        return "try to lock a locked node";
    case ZK_DISCONNECTED:
        return "not connected to server";
    case ZK_INVALIDPATH:
        return "invalid path";
    case ZK_READONLY:
        return "client is readonly";
    case ZK_SYSTEMERROR:
        return "system error";
    case ZK_RUNTIMEINCONSISTENCY:
        return "runtime inconsistency";
    case ZK_DATAINCONSISTENCY:
        return "data inconsistency";
    case ZK_CONNECTIONLOSS:
        return "connection loss";
    case ZK_MARSHALLINGERROR:
        return "marshalling error";
    case ZK_UNIMPLEMENTED:
        return "unimplemented";
    case ZK_OPERATIONTIMEOUT:
        return "operation timeout";
    case ZK_BADARGUMENTS:
        return "bad arguments";
    case ZK_INVALIDSTATE:
        return "invalid session state";
    case ZK_DNSFAILURE:
        return "error occurs when dns lookup";
    case ZK_APIERROR:
        return "api error";
    case ZK_NONODE:
        return "no node";
    case ZK_NOAUTH:
        return "not authenticated";
    case ZK_BADVERSION:
        return "bad version";
    case ZK_NOCHILDRENFOREPHEMERALS:
        return "no children for ephemerals";
    case ZK_NODEEXISTS:
        return "node exists";
    case ZK_NOTEMPTY:
        return "not empty";
    case ZK_SESSIONEXPIRED:
        return "session expired";
    case ZK_INVALIDCALLBACK:
        return "invalid callback";
    case ZK_INVALIDACL:
        return "invalid acl";
    case ZK_AUTHFAILED:
        return "authentication failed";
    case ZK_CLOSING:
        return "zookeeper is closing";
    case ZK_NOTHING:
        return "(not error) no server responses to process";
    case ZK_SESSIONMOVED:
        return "session moved to another server, so operation is ignored";
    case ZK_NOQUOTA:
        return "quota not enough";
    case ZK_SERVEROVERLOAD:
        return "server overload";
    }
    return "unknown error";
}

const char* ZookeeperEventString(ZookeeperEvent event)
{
    switch (event)
    {
    case ZK_SESSION_CONNECTING_EVENT:
        return "session connecting event";
    case ZK_SESSION_CONNECTED_EVENT:
        return "session connected event";
    case ZK_SESSION_EXPIRED_EVENT:
        return "session expired event";
    case ZK_SESSION_CLOSED_EVENT:
        return "session closing event";
    case ZK_SESSION_AUTH_FAILED_EVENT:
        return "session auth failed";
    case ZK_NODE_CREATED_EVENT:
        return "node created event";
    case ZK_NODE_DELETED_EVENT:
        return "node deleted event";
    case ZK_NODE_CHANGED_EVENT:
        return "node changed event";
    case ZK_CHILD_CHANGED_EVENT:
        return "node children changed event";
    }
    return "unknown event";
}

struct StringErrorCodePair {
    const char* error_string;
    int code;
};

#define ZK_ERROR_STRING_CODE_ENTRY(code) { #code, code }
StringErrorCodePair error_string_code_pair[] = {
    ZK_ERROR_STRING_CODE_ENTRY(ZK_OK),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_INVALIDCLIENTID),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_INVALIDDATA),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_ALREADYLOCKED),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_DISCONNECTED),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_INVALIDPATH),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_READONLY),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_SYSTEMERROR),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_RUNTIMEINCONSISTENCY),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_DATAINCONSISTENCY),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_CONNECTIONLOSS),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_MARSHALLINGERROR),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_UNIMPLEMENTED),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_OPERATIONTIMEOUT),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_BADARGUMENTS),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_INVALIDSTATE),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_DNSFAILURE),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_APIERROR),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_NONODE),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_NOAUTH),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_BADVERSION),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_NOCHILDRENFOREPHEMERALS),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_NODEEXISTS),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_NOTEMPTY),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_SESSIONEXPIRED),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_INVALIDCALLBACK),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_INVALIDACL),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_AUTHFAILED),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_CLOSING),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_NOTHING),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_SESSIONMOVED),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_NOQUOTA),
    ZK_ERROR_STRING_CODE_ENTRY(ZK_SERVEROVERLOAD),
};
#undef ZK_ERROR_STRING_CODE_ENTRY

bool StringToZookeeperErrorCode(const char* error_string, int* error_code)
{
    int size = sizeof(error_string_code_pair) / sizeof(StringErrorCodePair);
    for (int i = 0; i < size; ++i) {
        if (strcmp(error_string_code_pair[i].error_string, error_string) == 0) {
            *error_code = error_string_code_pair[i].code;
            return true;
        }
    }
    return false;
}

} // namespace common

