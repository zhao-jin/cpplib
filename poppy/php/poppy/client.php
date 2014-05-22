<?php
namespace poppy;

require_once 'DrSlump/Protobuf.php';
\DrSlump\Protobuf::autoload();

include_once 'descriptor.pb.php';
include_once 'rpc_error_code_info.pb.php';
include_once 'rpc_option.pb.php';
include_once 'poppy/poppy_client.php';

$GLOBALS['client'] = new \RpcClient();

class RpcController extends \RpcControllerSwig
{

}

class RpcChannel
{
    private $_innerChannel;

    function __construct($serverAddress)
    {
        $client = $GLOBALS['client'];
        $this->_innerChannel = new \RpcChannelSwig($client, $serverAddress);
    }

    function CallMethod($methodFullName, $controller, $request, $responseClass)
    {
        $controller->set_method_full_name($methodFullName);
        // TODO(ericliu) 判断是否设置超时改为调用has_timeout
        if ($controller->Timeout() <= 0)
            $controller->SetTimeout(10000);

        try {
            $responseData = $this->_innerChannel->RawCallMethod($controller,
                                                                $request->serialize(),
                                                                NULL);
        } catch(\Exception $e) {
            $controller->SetFailed(RpcErrorCode::RPC_ERROR_PARSE_REQUEST_MESSAGE);
        }

        if (!$controller->Failed()) {
            $class = new \ReflectionClass($responseClass);
            $response = $class->newInstance();

            try {
                $response->parse($responseData);
                return $response;
            } catch(Exception $e) {
                $controller->SetFailed(RpcErrorCode::RPC_ERROR_PARSE_RESPONES_MESSAGE);
            }
        }

        return NULL;
    }
}
