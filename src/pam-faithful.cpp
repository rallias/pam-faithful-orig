#include <nan.h>
#include <security/pam_appl.h>

struct pam_faithful_transaction
{
    const char *pam_service_name;
    const char *pam_username;
    const pam_message **message;
    pam_response **response;
    uv_mutex_t *pam_working;
    uv_sem_t *js_callback_working;
    bool pam_waiting_data;
    bool pam_done;
    int pam_retval;
};

int pam_start_converser(int num_msg, const pam_message **msg, pam_response **resp, void *appdata_ptr)
{
    pam_faithful_transaction *data = static_cast<pam_faithful_transaction *>(appdata_ptr);
    data->message = msg;
    data->response = resp;
    data->pam_waiting_data = true;
    uv_mutex_unlock(data->pam_working);
    uv_sem_wait(data->js_callback_working);
    uv_mutex_lock(data->pam_working);
    return PAM_SUCCESS;
}

void pam_start_thread(uv_work_t *req)
{
    pam_faithful_transaction *data = static_cast<pam_faithful_transaction *>(req->data);
    uv_mutex_lock(data->pam_working);

    pam_conv *conversation;
    conversation->appdata_ptr = data;
    conversation->conv = pam_start_converser;
    pam_handle_t *handle;

    int retval = pam_start(data->pam_service_name, data->pam_username, conversation, &handle);
    data->pam_done = true;
    data->pam_retval = retval;
    uv_mutex_unlock(data->pam_working);
}

// pam_start = (service: string, user: string, convCallback: ({message_style: int, message: string}) => Promise<string>)|string => Promise<pam_handle>
NAN_METHOD(PamStart)
{
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

    uv_loop_t *default_loop = uv_default_loop();
    pam_faithful_transaction *transaction = (pam_faithful_transaction *)uv_malloc_func(sizeof(pam_faithful_transaction));

    // Course of action.
    // We create the shared data, a pam_faithful_transaction.
    // We create a new uv_work, pam_start_worker.
    // We create a new uv_check, callback_handle.
    // pam_auth_async locks pam_faithful_transaction.mutex.
    // pam_auth_async calls pam_start.
    // pam_start calls the C++ conv callback.
    // C++ conv callback sets pam_faithful_transaction.message and unlocks pam_faithful_transaction.mutex, and waits on pam_faithful_transaction.js_callback_working.
    // callback_handle queues JS callback on original thread.
    // JS callback resolves promise, which posts js_callback_working.
    // pam_auth_async continues PAM transaction.
    // If C++ conv callback called again, go back to that step, otherwise, mark pam_faithful_transaction.pam_done true.
    // Resolve the outer promise.
}

NAN_MODULE_INIT(init)
{
    // pam_start(3)
    Nan::Set(target, Nan::New<v8::String>("pam_start").ToLocalChecked(), Nan::GetFunction(Nan::New<v8::FunctionTemplate>(PamStart)).ToLocalChecked());

    // pam_conv(3)
    Nan::Set(target, Nan::New<v8::String>("PAM_PROMPT_ECHO_OFF").ToLocalChecked(), Nan::New(PAM_PROMPT_ECHO_OFF));
    Nan::Set(target, Nan::New<v8::String>("PAM_PROMPT_ECHO_ON").ToLocalChecked(), Nan::New(PAM_PROMPT_ECHO_ON));
    Nan::Set(target, Nan::New<v8::String>("PAM_ERROR_MSG").ToLocalChecked(), Nan::New(PAM_ERROR_MSG));
    Nan::Set(target, Nan::New<v8::String>("PAM_TEXT_INFO").ToLocalChecked(), Nan::New(PAM_TEXT_INFO));
}

NODE_MODULE(pam_faithful, init);