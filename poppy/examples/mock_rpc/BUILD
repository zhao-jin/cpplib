proto_library(
    name = 'word_count_service_proto',
    srcs = 'word_count_service.proto',
    deps = '//poppy:rpc_option_proto'
)

cc_library(
    name = 'word_count_client',
    srcs = 'word_count_client.cc',
    deps = [
        ':word_count_service_proto',
        '//poppy:poppy',
    ]

)

cc_test(
    name = 'word_count_client_test',
    srcs = 'word_count_client_test.cc',
    deps = [
        ':word_count_client',
        '//poppy:poppy_mock',
    ]
)

