
%module(directors="1") zk_client

%{
#include "common/zookeeper/client/zookeeper_acl.h"
#include "common/zookeeper/client/zookeeper_node.h"
#include "common/zookeeper/client/zookeeper_client.h"
%}

%include stdint.i
%include std_string.i
%include std_vector.i
namespace std {
    %template(string_vector) vector<std::string>;
}

%include "common/zookeeper/client/zookeeper_acl.h"
%include "common/zookeeper/client/zookeeper_node.h"
%include "common/zookeeper/client/zookeeper_client.h"

