package com.soso.poppy;

import com.google.protobuf.Message;
import com.google.protobuf.RpcCallback;
import com.google.protobuf.InvalidProtocolBufferException;

public class ResponseHandler extends com.soso.poppy.swig.ResponseHandler {
    RpcController controller;
    Message responsePrototype;
    RpcCallback<com.google.protobuf.Message> done;

    public ResponseHandler(
            RpcController controller,
            Message responsePrototype,
            RpcCallback<com.google.protobuf.Message> done) {
        super();
        this.controller = controller;
        this.responsePrototype = responsePrototype;
        this.done = done;
    }

    public void Run(byte[] response_data) {
        try {
            Message response = responsePrototype;
            // parse response object from response_data.
            if (!controller.failed()) {
                Message.Builder builder = this.responsePrototype.newBuilderForType();
                builder.mergeFrom(response_data);
                response = builder.build();
            }

            // call the RpcCallback.
            done.run(response);
        } catch (InvalidProtocolBufferException e) {
            this.controller.setFailed(
                RpcErrorCodeInfo.RpcErrorCode.RPC_ERROR_PARSE_RESPONES_MESSAGE);
        }
    }


    public void Run(String response_data) {
        Run(response_data.getBytes());
    }

}

