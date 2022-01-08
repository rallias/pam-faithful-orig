#include <nan.h>
#include <security/pam_appl.h>

NAN_METHOD(PamStart)
{
    // pam_start = (service: string, user: string, convCallback: ({message_style: int, message: string}) => string) => pam_handle
    if (info.Length() < 3)
    {
        Nan::ThrowTypeError("Wrong number of arguments.");
        return;
    }

    v8::Local<v8::Value> serviceVal(info[0]);
    if (!serviceVal->IsString())
    {
        Nan::ThrowTypeError("Argument 0 (service) must be a string.");
        return;
    }
    v8::Local<v8::Value> userVal(info[1]);
    if (!userVal->IsString())
    {
        Nan::ThrowTypeError("Argument 1 (user) must be a string.");
        return;
    }
    v8::Local<v8::Value> convCallbackVal(info[2]);
    if (!convCallbackVal->IsFunction())
    {
        Nan::ThrowTypeError("Argument 2 (convCallback) must be a function.");
        return;
    }

    v8::Isolate *isolate = v8::Isolate::GetCurrent();

    v8::String::Utf8Value serviceV8Str(isolate, serviceVal);
    std::string service(*serviceV8Str);

    v8::String::Utf8Value userV8Str(isolate, userVal);
    std::string user(*userV8Str);
}

NAN_MODULE_INIT(init)
{
    Nan::Set(target, Nan::New<v8::String>("pam_start").ToLocalChecked(), Nan::GetFunction(Nan::New<v8::FunctionTemplate>(PamStart)).ToLocalChecked());
}

NODE_MODULE(pam_faithful, init);