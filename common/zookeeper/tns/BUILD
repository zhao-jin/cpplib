proto_library(
    name = 'tns_proto',
    srcs = 'tns.proto'
)

cc_library(
    name = 'tns',
    srcs = [
        'tns.cpp',
    ],
    deps = [
        ':tns_proto',
        '//common/base/string:string',
        '//common/zookeeper/client:zookeeper_client',
        '//thirdparty/gflags:gflags',
        '//thirdparty/glog:glog',
    ]
)
'''
cc_test(
    name = 'tns_test',
    srcs = [
        'tns_test.cpp',
    ],
    deps = [
        ':tns',
        '//common/system/net:ip_address',
        '//taas/sdk:taas_ex',
    ],
    dynamic_link = 1,
)
'''
