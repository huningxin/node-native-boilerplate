#include <nan.h>
#include <node.h>
#include "functions.h"

NAN_METHOD(nothing) {
    NanScope();
    NanReturnUndefined();
}

NAN_METHOD(aString) {
    NanScope();
    NanReturnValue(NanNew<v8::String>("This is a thing."));
}

NAN_METHOD(aBoolean) {
    NanScope();
    NanReturnValue(NanFalse());
}

NAN_METHOD(aNumber) {
    NanScope();
    NanReturnValue(NanNew<v8::Number>(1.75));
}

NAN_METHOD(anObject) {
    NanScope();
    v8::Local<v8::Object> obj = NanNew<v8::Object>();
    obj->Set(NanNew<v8::String>("key"), NanNew<v8::String>("value")); 
    NanReturnValue(obj);
}

NAN_METHOD(anArray) {
    NanScope();
    v8::Local<v8::Array> arr = NanNew<v8::Array>(3);
    arr->Set(0, NanNew<v8::Number>(1));
    arr->Set(1, NanNew<v8::Number>(2));
    arr->Set(2, NanNew<v8::Number>(3));
    NanReturnValue(arr);
}

NAN_METHOD(callback) {
    NanScope();
    v8::Local<v8::Function> callbackHandle = args[0].As<v8::Function>();
    NanMakeCallback(NanGetCurrentContext()->Global(), callbackHandle, 0, NULL);
    NanReturnUndefined();
}

class FillBufferWorker : public NanAsyncWorker {
  public:
    FillBufferWorker(char* buffer, int length, NanCallback *callback)
        : NanAsyncWorker(callback), buffer_(buffer), length_(length) {}
    ~FillBufferWorker() {}
    // Executed inside the worker-thread.
    // It is not safe to access V8, or V8 data structures
    // here, so everything we need for input and output
    // should go on `this`.
    void Execute () {
        for(int i = 0; i < length_; ++i)
            buffer_[i] = 'p';
    }
    // Executed when the async work is complete
    // this function will be run inside the main event loop
    // so it is safe to use V8 again
    void HandleOKCallback () {
        NanScope();
        v8::Local<v8::Value> argv[] = {
            NanNull()
        };
        callback->Call(1, argv);
    }
  private:
    char* buffer_;
    int length_;
};


NAN_METHOD(fillBuffer) {
    NanScope();
    char* buffer = node::Buffer::Data(args[0]);
    int length = args[1]->Uint32Value();
    NanCallback *callback = new NanCallback(args[2].As<v8::Function>());
    buffer[0] = 'a';
    NanAsyncQueueWorker(new FillBufferWorker(buffer, length, callback));
    NanReturnUndefined();
}
