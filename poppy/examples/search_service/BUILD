proto_library(
    name = 'search_service_proto',
    srcs = 'search_service.proto',
    deps = '//poppy:rpc_option_proto'
)

cc_binary(
    name = 'search_client',
    srcs = [
        'search_client.cc'
    ],
    deps = [
        ':search_service_proto',
        '//poppy:poppy'
    ]
)

cc_binary(
    name = 'search_server',
    srcs = [
        'search_server.cc'
    ],
    deps = [
        ':search_service_proto',
        '//poppy:poppy'
    ]
)
