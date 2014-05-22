// Copyright 2011, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#ifndef POPPY_STREAMING_PACKET_H
#define POPPY_STREAMING_PACKET_H

#include <string>
#include "common/base/closure.h"
#include "common/base/stdint.h"
#include "poppy/streaming/stream.h"
#include "poppy/streaming/streaming_service_info.pb.h"

namespace poppy {
namespace streaming {

struct Packet
{
    enum Type {
        TYPE_UNKNOWN = 0,
        TYPE_READ,
        TYPE_WRITE,
        TYPE_EOF,
        TYPE_ABORT,
        TYPE_CLOSE,
    };
    Packet() : type(TYPE_UNKNOWN), id(-1), data(NULL), callback(NULL), timer_id(-1) {}
    Type type;
    int64_t id;
    std::string* data;
    CompletionCallback* callback;
    uint64_t timer_id;
};

} // namespace streaming
} // namespace poppy

#endif // POPPY_STREAMING_PACKET_H
