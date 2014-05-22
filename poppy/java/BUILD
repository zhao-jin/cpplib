java_jar(
    name = 'poppy_java_client',
    srcs = [
        'src/com/soso/poppy'
    ],
    deps = [
        '//poppy:rpc_meta_info_proto',
        '//poppy:rpc_option_proto',
        '//poppy:rpc_message_proto',
        './lib:log4j',
        './lib:netty',
        './lib:protobuf-java',
        './lib:snappy-java',
        './lib:junit',
        './lib:zookeeper'
    ]
)

java_jar(
    name = 'poppy_java_client_test',
    srcs = [
        'test/com/soso/rpc_examples'
    ],
    deps = [
        '//poppy:rpc_meta_info_proto',
        '//poppy:rpc_option_proto',
        '//poppy:rpc_message_proto',
        '//poppy/test:echo_service_proto',
        ':poppy_java_client',
        './lib:log4j',
        './lib:netty',
        './lib:protobuf-java',
        './lib:snappy-java',
        './lib:junit',
        './lib:zookeeper'
    ]
)
