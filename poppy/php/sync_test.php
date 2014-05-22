<?php

require_once 'Benchmark/Profiler.php';
include_once 'poppy/client.php';
include_once 'echo_service.pb.php';

use poppy\RpcErrorCode;
use poppy\CompressType;
use poppy\RpcController;
use poppy\RpcChannel;
use rpc_examples\EchoRequest;
use rpc_examples\EchoServer_Stub;

function sendRequest($stub)
{
    $controller = new RpcController();
    $controller->SetTimeout(2000);
    $controller->SetRequestCompressType(CompressType::CompressTypeSnappy);
    $controller->SetResponseCompressType(CompressType::CompressTypeSnappy);
    $request = new EchoRequest();
    $request->setUser('ericliu');
    $request->setMessage('hi');
    $request->setSfix64('823695487788725597');
    $response = $stub->EchoMessage($controller, $request);
    if (!$controller->Failed()) {
        // echo "Success\n";
        // echo "response.user    : " . $response->getUser() . "\n";
        // echo "response.message : " . $response->getMessage() . "\n";
        // echo "response.sfix64  : " . $response->getsFix64() . "\n";
    } else {
        echo "Failed, " . $controller->ErrorCode() . " " . $controller->ErrorText() . "\n";
    }
}

if ($argc < 2)
    echo "Usage: " . $argv[0] . " total_request_count\n";

$totalRequestCount = (int)$argv[1];
$channel = new RpcChannel("127.0.0.1:10000");
$stub = new EchoServer_Stub($channel);
$profiler = new Benchmark_Profiler(true);
$sentCount = 0;
while ($sentCount < $totalRequestCount) {
    $profiler->enterSection("sync_test");
    sendRequest($stub);
    $profiler->leaveSection("sync_test");
    $sentCount++;
}
$profiler->stop();
$profiler->display();

?>

