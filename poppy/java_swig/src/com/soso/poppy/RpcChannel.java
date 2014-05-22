package com.soso.poppy;

import java.util.TreeMap;
import com.google.protobuf.Descriptors.MethodDescriptor;
import com.google.protobuf.InvalidProtocolBufferException;
import com.google.protobuf.Message;
import com.google.protobuf.RpcCallback;

public class RpcChannel implements com.google.protobuf.RpcChannel,
        com.google.protobuf.BlockingRpcChannel {
    com.soso.poppy.swig.RpcChannelSwig impl;
    class EmbededMetaInfo {
        String methodFullName;
        RpcOption.CompressType requestCompressType;
        RpcOption.CompressType responseCompressType;
        long timeout;
    }
    TreeMap<String, EmbededMetaInfo> metaMap;

    public RpcChannel(RpcClient client, String serverAddress) {
        impl = new com.soso.poppy.swig.RpcChannelSwig(client, serverAddress);
        metaMap = new TreeMap<String, EmbededMetaInfo>();
    }

    EmbededMetaInfo getMetaInfo(
            final com.google.protobuf.Descriptors.MethodDescriptor method) {
        String methodFullName = method.getFullName();
        EmbededMetaInfo meta = metaMap.get(methodFullName);
        if (meta == null) {
            // Calculate the expected timeout.
            long timeout = 0;
            if (method.getOptions().hasExtension(RpcOption.methodTimeout)) {
                timeout = method.getOptions().getExtension(RpcOption.methodTimeout);
            } else {
                timeout = method.getService().getOptions()
                        .getExtension(RpcOption.serviceTimeout);
            }
            if (timeout <= 0) {
                timeout = 10000;
            }
            RpcOption.CompressType requestCompressType = RpcOption.CompressType.CompressTypeNone;
            if (method.getOptions().hasExtension(RpcOption.requestCompressType)) {
                requestCompressType = method.getOptions()
                        .getExtension(RpcOption.requestCompressType);
            }
            RpcOption.CompressType responseCompressType = RpcOption.CompressType.CompressTypeNone;
            if (method.getOptions().hasExtension(RpcOption.responseCompressType)) {
                responseCompressType = method.getOptions()
                        .getExtension(RpcOption.responseCompressType);
            }

            meta = new EmbededMetaInfo();
            meta.methodFullName = methodFullName;
            meta.timeout = timeout;
            meta.requestCompressType = requestCompressType;
            meta.responseCompressType = responseCompressType;

            metaMap.put(methodFullName, meta);
        }
        return meta;
    }
    /*
     * Implements the com.google.protobuf.RpcChannel interface. If the done is
     * NULL, it's a synchronous call, or it's an asynchronous and uses the
     * callback inform the completion (or failure). It's only called by user's
     * thread.
     */
    public void callMethod(
            final MethodDescriptor method,
            com.google.protobuf.RpcController controller,
            final Message request,
            Message responsePrototype,
            RpcCallback<Message> done) {
        RpcController rpcController = (RpcController)controller;

        EmbededMetaInfo meta = getMetaInfo(method);
        rpcController.set_method_full_name(meta.methodFullName);
        if (rpcController.Timeout() <= 0)
            rpcController.setTimeout(meta.timeout);
        if (rpcController.use_default_request_compress_type) {
            rpcController.setRequestCompressType(meta.requestCompressType);
        }
        if (rpcController.use_default_response_compress_type) {
            rpcController.setResponseCompressType(meta.responseCompressType);
        }

        ResponseHandler responseHandler = new ResponseHandler(
                rpcController,
                responsePrototype,
                done);

        impl.RawCallMethod(rpcController, request.toByteArray(), responseHandler);
    }

    public com.google.protobuf.Message callBlockingMethod(
            final MethodDescriptor method,
            com.google.protobuf.RpcController controller,
            Message request,
            Message responsePrototype)
            throws com.google.protobuf.ServiceException {

        RpcController rpcController = (RpcController)controller;
        EmbededMetaInfo meta = getMetaInfo(method);
        rpcController.set_method_full_name(meta.methodFullName);
        if (rpcController.Timeout() <= 0)
            rpcController.setTimeout(meta.timeout);
        if (rpcController.use_default_request_compress_type) {
            rpcController.setRequestCompressType(meta.requestCompressType);
        }
        if (rpcController.use_default_response_compress_type) {
            rpcController.setResponseCompressType(meta.responseCompressType);
        }

        byte[] responseBytes =
            impl.RawCallMethod(rpcController, request.toByteArray(), null);
        if (controller.failed()) {
            throw new com.google.protobuf.ServiceException(
                    controller.errorText());
        }

        try {
            // parse response object from response_data.
            Message.Builder builder = responsePrototype.newBuilderForType();
            builder.mergeFrom(responseBytes);
            Message response = builder.build();

            return response;
        } catch (InvalidProtocolBufferException e) {
            throw new com.google.protobuf.ServiceException(
                    e.getMessage());
        }
    }
}

