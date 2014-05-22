cc_binary(
    name = 'client',
    srcs = 'client.cpp',
    deps = [
        '//common/system/net:socket',
        '//thirdparty/gflags:gflags',
        '//thirdparty/glog:glog'
    ]
)

cc_binary(
    name = 'set_zk_domain',
    srcs = 'set_zk_domain.cpp',
    deps = [
        '//common/base/string:string',
        '//common/system/net:net',
        '//common/zookeeper/client:zookeeper_client',
        '//thirdparty/gflags:gflags',
        '//thirdparty/glog:glog',
    ]
)
'''
cc_binary(
    name = 'set_quota',
    srcs = 'set_quota.cc',
    deps = [
        '//common/base/string:string',
        '//common/zookeeper/client:zookeeper_client',
        '//taas/sdk:taas_ex',
        '//thirdparty/gflags:gflags',
        '//thirdparty/glog:glog',
    ],
)
'''
