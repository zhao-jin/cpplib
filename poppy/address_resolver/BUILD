cc_library(
    name = 'address_resolver',
    srcs = [
        'domain_resolver.cc',
        'ip_list_resolver.cc',
        'resolver_factory.cc',
        'tns_address_convertor.cc',
        'tns_job_resolver.cc',
        'tns_task_resolver.cc',
        'zk_dir_path_resolver.cc',
        'zk_path_resolver.cc',
    ],
    deps = [
        '//common/base/string:string',
        '//common/system/net:net',
        '//common/zookeeper/client:zookeeper_client',
    ]
)

cc_test(
    name = 'ip_list_resolver_test',
    srcs = 'ip_list_resolver_test.cc',
    deps = [
        '//poppy/address_resolver:address_resolver',
    ],
    exclusive = True
)

cc_test(
    name = 'domain_resolver_test',
    srcs = 'domain_resolver_test.cc',
    deps = [
        '//poppy/address_resolver:address_resolver',
    ],
    exclusive = True
)

cc_test(
    name = 'zk_dir_path_resolver_test',
    srcs = 'zk_dir_path_resolver_test.cc',
    deps = [
        '//poppy/address_resolver:address_resolver',
    ],
    exclusive = True
)

cc_test(
    name = 'zk_path_resolver_test',
    srcs = 'zk_path_resolver_test.cc',
    deps = [
        '//poppy/address_resolver:address_resolver',
    ],
    exclusive = True
)

cc_test(
    name = 'tns_task_resolver_test',
    srcs = 'tns_task_resolver_test.cc',
    deps = [
        '//poppy/address_resolver:address_resolver',
    ],
    exclusive = True
)

cc_test(
    name = 'tns_job_resolver_test',
    srcs = 'tns_job_resolver_test.cc',
    deps = [
        '//poppy/address_resolver:address_resolver',
    ],
    exclusive = True
)
