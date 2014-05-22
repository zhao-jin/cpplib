cc_library(
    name = 'zookeeper_client',
    srcs = [
        'watcher_manager.cpp',
        'zookeeper_acl.cpp',
        'zookeeper_client.cpp',
        'zookeeper_node.cpp',
        'zookeeper_node_mock.cpp',
        'zookeeper_session.cpp',
        'zookeeper_types.cpp',
    ],
    deps = [
        '//thirdparty/zookeeper:zookeeper_mt',
        '//thirdparty/glog:glog',
        '//thirdparty/gmock:gmock',
        '//common/base:module',
        '//common/base/string:string',
        '//common/crypto/hash:hash',
        '//common/encoding:encoding',
        '//common/system/concurrency:concurrency',
        '//common/system/io:io',
        '//common/system/net:net',
    ]
)

swig_library(
    name = 'zk_client',
    srcs = 'zk_client.i',
    deps = '//common/zookeeper/client:zookeeper_client',
)
