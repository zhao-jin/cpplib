proto_library(
    name = 'snapshot_service_proto',
    srcs = 'snapshot_service.proto',
    deps = [
        '//poppy:rpc_option_proto',
        '//poppy:rpc_message_proto'
    ]
)

cc_binary(
    name = 'snapshotclient',
    srcs = [
        'snapshot_client.cc'
    ],
    deps = [
        ':snapshot_service_proto',
        '//poppy:poppy'
    ]
)

cc_binary(
    name = 'snapshotserver',
    srcs = [
        'snapshot_server.cc'
    ],
    deps = [
        ':snapshot_service_proto',
        '//poppy:poppy'
    ]
)
