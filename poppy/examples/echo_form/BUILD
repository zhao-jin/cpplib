proto_library(
    name = 'ad_proto',
    srcs = [
        'ad.proto',
    ],
    deps = '//poppy:rpc_message_proto'
)

proto_library(
    name = 'echo_service_proto',
    srcs = 'echo_service.proto',
    deps = [
        ':ad_proto',
        '//poppy:rpc_option_proto'
    ]
)

cc_binary(
    name = 'echoserver',
    srcs = [
        'echo_server.cc'
    ],
    deps = [
        ':echo_service_proto',
        '//poppy:poppy'
    ]
)

