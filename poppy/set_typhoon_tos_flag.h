// Copyright (c) 2011, Tencent Inc.
#ifndef POPPY_SET_TYPHOON_TOS_FLAG_H
#define POPPY_SET_TYPHOON_TOS_FLAG_H

#include "thirdparty/gflags/gflags.h"

DECLARE_bool(typhoon_enable_vpn);
DECLARE_int32(poppy_default_tos);

inline void SetTyphoonTosIfNecessary() {
    if (FLAGS_typhoon_enable_vpn && FLAGS_poppy_default_tos == 96) {
        FLAGS_poppy_default_tos = 95;
    }
}

#endif // POPPY_SET_TYPHOON_TOS_FLAG_H
