// Copyright 2012, Tencent Inc.
// Author: Xiaokang Liu <hsiaokangliu@tencent.com>

#include <string>
#include "poppy/address_resolver/tns_address_convertor.h"
#include "thirdparty/gtest/gtest.h"

TEST(ResolverFactory, PathConvertor)
{
    std::string tns_path = "/tns/tjnl-avivi/xcube/hsiaokangliu_job?port=poppy";
    std::string zk_path;

    poppy::ConvertTnsPathToZkPath(tns_path, &zk_path);
    ASSERT_EQ("/zk/tjnl/tborg/tjnl-avivi/tns/xcube/hsiaokangliu_job?port=poppy", zk_path);

    std::string tmp;
    zk_path = "/zk/tjnl/tborg/tjnl-avivi/tns/xcube/hsiaokangliu_job?port=poppy";
    poppy::ConvertZkPathToTnsPath(zk_path, &tmp);
    ASSERT_EQ(tns_path, tmp);

    tns_path = "/tns/tjd2-wob-test/xcube/test_job?port=poppy";
    poppy::ConvertTnsPathToZkPath(tns_path, &zk_path);
    ASSERT_EQ("/zk/tjd2/tborg/tjd2-wob-test/tns/xcube/test_job?port=poppy", zk_path);

    zk_path = "/zk/tjd2/tborg/tjd2-wob-test/tns/xcube/test_job?port=poppy";
    poppy::ConvertZkPathToTnsPath(zk_path, &tmp);
    ASSERT_EQ(tns_path, tmp);
}

