proto_library(
    name = 'echo_service_proto',
    srcs = 'echo_service.proto',
    deps = '//poppy:rpc_option_proto'
)

cc_binary(
    name = 'echoclient',
    srcs = [
        'echo_client.cc'
    ],
    deps = [
        ':echo_service_proto',
        '//poppy:poppy'
    ]
)

resource_library(
    name = 'static_resource',
    srcs = [
        'index.html'
    ]
)

cc_binary(
    name = 'echoserver',
    srcs = [
        'echo_server.cc'
    ],
    deps = [
        ':echo_service_proto',
        ':static_resource',
        '//poppy:poppy'
    ]
)

cc_test(
    name = 'pb_json_benchmark',
    srcs = [
        'pb_json_benchmark.cc'
    ],
    deps = [
        ':echo_service_proto',
        '//common/system/time:time',
        '//common/encoding:encoding',
        '//thirdparty/glog:glog'
    ]
)

cc_binary(
    name = 'json_benchmark',
    srcs = [
        'json_benchmark.cc'
    ],
    deps = [
        '//common/system/net:net',
        '//common/system/time:time',
        '//thirdparty/gflags:gflags',
        '//thirdparty/glog:glog'
    ]
)
