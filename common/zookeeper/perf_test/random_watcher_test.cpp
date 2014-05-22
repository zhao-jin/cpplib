#include <algorithm>
#include <string>
#include <signal.h>
#include "common/system/time/timestamp.h"
#include "common/zookeeper/perf_test/watcher.h"
#include "common/zookeeper/perf_test/zk_client.h"
#include "thirdparty/gflags/gflags.h"
#include "thirdparty/glog/logging.h"
// using namespace common;

DEFINE_string(server, "tjzt.zk.oa.com:2181", "zk server address");
DEFINE_string(tree_path, "/tree", "zk node tree prefix name");
DEFINE_int32(node_number, 100, "watched nodes number");

bool g_quit = false;

static void SignalIntHandler(int sig)
{
    g_quit = true;
}

int main(int argc, char** argv)
{
    FLAGS_log_dir = ".";
    FLAGS_v = 0;
    FLAGS_logbuflevel = -1;

    google::ParseCommandLineFlags(&argc, &argv, true);
    google::InitGoogleLogging(argv[0]);

    signal(SIGINT, SignalIntHandler);
    signal(SIGTERM, SignalIntHandler);

    ZkClient client(FLAGS_server);

    std::vector<std::string> children;
    LOG(INFO) << "Begin to getchildren...";
    int64_t t1 = GetTimeStampInMs();
    client.GetChildren(FLAGS_tree_path, &children);
    int64_t t2 = GetTimeStampInMs();
    LOG(INFO) << "GetChildren costs: " << t2 - t1 << " ms";

    std::random_shuffle(children.begin(), children.end());
    LOG(INFO) << "Begin to set watchers...";
    t2 = GetTimeStampInMs();
    for (int i = 0; i < FLAGS_node_number; ++i) {
        client.SetWatch(children[i], ZOO_DELETED_EVENT);
    }
    int64_t t3 = GetTimeStampInMs();
    LOG(INFO) << "SetWatcher costs: " << t3 - t2 << " ms";

    LOG(INFO) << "Waiting for watchers trigger...";
    while (!g_quit) {
        sleep(1);
        if (g_delete_nodes >= FLAGS_node_number) {
            break;
        }
    }
    int64_t t4 = GetTimeStampInMs();
    LOG(INFO) << "Run over: " << t4 - t3 << " ms";

    return EXIT_SUCCESS;
}
