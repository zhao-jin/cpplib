// Copyright (c) 2012, Tencent Inc. All rights reserved.
// Author: Yubing Yin(yubingyin@tencent.com)

#ifndef COMMON_ZOOKEEPER_SDK_ZOOKEEPER_ERRNO_H
#define COMMON_ZOOKEEPER_SDK_ZOOKEEPER_ERRNO_H

namespace common
{

enum ZKErrorCode
{
    kZKSuccess                  = 0,    // operation success

    kZKSystemErrors             = -1,   // indicate system or server internal error range
    // system error begin
    kZKRuntimeInconsistency     = -2,   // runtime inconsistency found
    kZKDataInconsistency        = -3,   // data inconsistency found
    kZKConnectionLoss           = -4,   // connection lost
    kZKMarshallError            = -5,   // error while marshalling or unmarshalling data
    kZKUnimplemented            = -6,   // operation unimplemented
    kZKOperationTimeout         = -7,   // operation timeout
    kZKInvalidArguments         = -8,   // invalid arguments
    kZKInvalidState             = -9,   // invalid session state
    // system error end

    kZKApiErrors                = -100, // indicate api error range
    // api error begin
    kZKNoNode                   = -101, // node doesn't exist
    kZKNoAuth                   = -102, // not authenticated
    kZKBadVersion               = -103, // version not match
    kZKNoChildForEphemeral      = -108, // ephemeral nodes can't have child
    kZKNodeExist                = -110, // node already exists
    kZKNotEmpty                 = -111, // node has children
    kZKSessionExpired           = -112, // session has expired by server
    kZKInvalidCallback          = -113, // invalid callback
    kZKInvalidACL               = -114, // invalid acl
    kZKAuthFailed               = -115, // authentication failed
    kZKClosing                  = -116, // zk is closing
    kZKNothing                  = -117, // (not error) no server responses to process
    kZKSessionMoved             = -118, // session moved to another server, operation is ignored
    // api error end
};

} // namespace common

#endif // COMMON_ZOOKEEPER_SDK_ZOOKEEPER_ERRNO_H
