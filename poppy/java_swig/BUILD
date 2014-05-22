#"""
java_jar(
    name = 'poppy_java_client',
    srcs = [
        'src/com/soso/poppy'
    ],
    deps = [
        '//poppy:rpc_option_proto',
        '//poppy:rpc_message_proto',
        '//poppy:poppy_client',
        './lib:protobuf-java',
        './lib:junit',
    ]
)

java_jar(
    name = 'poppy_java_client_test',
    srcs = [
        'test/com/soso/rpc_examples'
    ],
    deps = [
        '//poppy/test:echo_service_proto',
        ':poppy_java_client',
        './lib:log4j',
        './lib:junit',
        './lib:protobuf-java',
    ]
)
#"""
