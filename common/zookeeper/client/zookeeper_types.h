// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_TYPES_H
#define COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_TYPES_H

#include <string.h>

namespace common {

enum ZookeeperErrorCode
{
    ZK_OK = 0, /*!< Everything is OK */

    /** client wrapper defined errors. */
    ZK_INVALIDCLIENTID = 1,    /*!< Client id is not valid. */
    ZK_INVALIDDATA = 2,        /*!< Data on the node is not valid. */
    ZK_ALREADYLOCKED = 3,      /*!< Try lock a locked node. */
    ZK_DISCONNECTED = 4,       /*!< Session is not connected. */
    ZK_INVALIDPATH = 5,        /*!< Invalid path. */
    ZK_READONLY = 6,           /*!< Client is readonly. */

    /** System and server-side errors.
     * This is never thrown by the server, it shouldn't be used other than
     * to indicate a range. Specifically error codes greater than this
     * value, but lesser than {@link #ZAPIERROR}, are system errors. */

    ZK_SYSTEMERROR = -1,
    ZK_RUNTIMEINCONSISTENCY = -2,/*!< A runtime inconsistency was found */
    ZK_DATAINCONSISTENCY = -3, /*!< A data inconsistency was found */
    ZK_CONNECTIONLOSS = -4,    /*!< Connection to the server has been lost */
    ZK_MARSHALLINGERROR = -5,  /*!< Error while marshalling or unmarshalling data */
    ZK_UNIMPLEMENTED = -6,     /*!< Operation is unimplemented */
    ZK_OPERATIONTIMEOUT = -7,  /*!< Operation timeout */
    ZK_BADARGUMENTS = -8,      /*!< Invalid arguments */
    ZK_INVALIDSTATE = -9,      /*!< Invliad zhandle state */
    ZK_DNSFAILURE = -10,       /*!< Error occurs when dns lookup  */

    /** API errors.
     * This is never thrown by the server, it shouldn't be used other than
     * to indicate a range. Specifically error codes greater than this
     * value are API errors (while values less than this indicate a
     * {@link #ZSYSTEMERROR}).   */

    ZK_APIERROR = -100,
    ZK_NONODE = -101,          /*!< Node does not exist */
    ZK_NOAUTH = -102,          /*!< Not authenticated */
    ZK_BADVERSION = -103,      /*!< Version conflict */
    ZK_NOCHILDRENFOREPHEMERALS = -108, /*!< Ephemeral nodes may not have children */
    ZK_NODEEXISTS = -110,      /*!< The node already exists */
    ZK_NOTEMPTY = -111,        /*!< The node has children */
    ZK_SESSIONEXPIRED = -112,  /*!< The session has been expired by the server */
    ZK_INVALIDCALLBACK = -113, /*!< Invalid callback specified */
    ZK_INVALIDACL = -114,      /*!< Invalid ACL specified */
    ZK_AUTHFAILED = -115,      /*!< Client authentication failed */
    ZK_CLOSING = -116,         /*!< ZooKeeper is closing */
    ZK_NOTHING = -117,         /*!< (not error) no server responses to process */
    ZK_SESSIONMOVED = -118,    /*!< session moved to another server, so operation is ignored */
    ZK_NOQUOTA = -119,         /*!< quota is not enough. */
    ZK_SERVEROVERLOAD = -120,  /*!< server overload. */
};

enum ZookeeperEvent
{
    /// session events.
    ZK_SESSION_CONNECTING_EVENT = 11,   // session is connecting(reconnecting) to server
    ZK_SESSION_CONNECTED_EVENT = 12,    // session has been established(re-established)
    ZK_SESSION_EXPIRED_EVENT = 13,      // session has expired
    ZK_SESSION_CLOSED_EVENT = 14,       // session has been closed(not used now).
    ZK_SESSION_AUTH_FAILED_EVENT = 15,  // auth failed when try to open a session
    /// node events.
    ZK_NODE_CREATED_EVENT = 1,          // node created
    ZK_NODE_DELETED_EVENT = 2,          // node deleted
    ZK_NODE_CHANGED_EVENT = 3,          // node data changed
    ZK_CHILD_CHANGED_EVENT = 4,         // children number changed.
};

const char* ZookeeperErrorString(ZookeeperErrorCode error);
const char* ZookeeperEventString(ZookeeperEvent event);
bool StringToZookeeperErrorCode(const char* error_string, int* error_code);

} // namespace common

#endif // COMMON_ZOOKEEPER_CLIENT_ZOOKEEPER_TYPES_H
