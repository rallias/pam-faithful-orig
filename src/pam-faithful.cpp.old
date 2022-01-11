#include <security/pam_appl.h>
#include <napi.h>
#include <thread>
#include <mutex>
#include <functional>

struct pam_faithful_transaction
{
    std::string service;
    std::string username;
    Napi::ThreadSafeFunction convCallback;
    std::mutex baton;
    Napi::ThreadSafeFunction promise;
    pam_handle_t *handle;
    int pam_status;
    unsigned int num_msg;
    const pam_message **msg;
    pam_response **resp;
    bool pam_fail;
};

class PamHandleWrapper : public Napi::ObjectWrap<PamHandleWrapper>
{
public:
    PamHandleWrapper(const Napi::CallbackInfo &info) : Napi::ObjectWrap<PamHandleWrapper>(info) {}

    static void Initialize(Napi::Env env, Napi::Object exports)
    {
    }

    void *pam_handle;
};

Napi::Value
pam_start_and_they_say_it_is_a_promise_fail(const Napi::CallbackInfo &info)
{
    pam_faithful_transaction *transaction = static_cast<pam_faithful_transaction *>(info.Data());
    transaction->pam_fail = true;
    transaction->baton.unlock();
    return info.Env().Undefined();
}

pam_response *pam_start_and_im_processing_it_line_by_line(Napi::String response)
{
    // ... and I swear it was in self defense.
    pam_response *retdata = new pam_response;
    retdata->resp = new char[response.Utf8Value().length() + 1];
    size_t len = response.Utf8Value().copy(retdata->resp, response.Utf8Value().length());
    retdata->resp[len] = '\0';
    return retdata;
}

Napi::Value pam_start_ive_got_the_data(const Napi::CallbackInfo &info)
{
    // When I named this function, I had the song "I Shot The Sheriff" stuck in my mind.
    pam_faithful_transaction *transaction = static_cast<pam_faithful_transaction *>(info.Data());
    if (info.Length() < 1)
    {
        transaction->pam_fail = true;
        transaction->baton.unlock();
        throw Napi::TypeError::New(info.Env(), "Insufficient arguments.");
    }
    else
    {
        if (info[0].IsArray())
        {
            if (info[0].As<Napi::Array>().Length() < transaction->num_msg)
            {
                transaction->pam_fail = true;
                transaction->baton.unlock();
                throw Napi::Error::New(info.Env(), "Response count does not match message count.");
            }
            else
            {
                pam_response **responses = (new pam_response *[transaction->num_msg]);
                transaction->resp = responses;
                for (unsigned int i = 0; i < transaction->num_msg; i++)
                {
                    responses[i] = pam_start_and_im_processing_it_line_by_line(info[0].As<Napi::Array>().Get(i).As<Napi::String>());
                }
                transaction->baton.unlock();
                return info.Env().Undefined();
            }
        }
        else if (info[0].IsString())
        {
            if (transaction->num_msg > 1)
            {
                transaction->pam_fail = true;
                transaction->baton.unlock();
                throw Napi::Error::New(info.Env(), "Response count does not match message count.");
            }
            else
            {
                pam_response *response = pam_start_and_im_processing_it_line_by_line(info[0].As<Napi::String>().As<Napi::String>());
                transaction->resp = &response;
                transaction->baton.unlock();
                return info.Env().Undefined();
            }
        }
        else
        {
            transaction->pam_fail = true;
            transaction->baton.unlock();
            throw Napi::Error::New(info.Env(), "What did you give me?!?");
        }
    }
}

int pam_start_converser(int num_msg, const pam_message **msg, pam_response **resp, void *appdata_ptr)
{
    pam_faithful_transaction *transaction = static_cast<pam_faithful_transaction *>(appdata_ptr);
    auto callback = [](Napi::Env env, Napi::Function jsCallback, pam_faithful_transaction *transaction)
    {
        transaction->baton.lock();
        Napi::Array value = Napi::Array::New(env, transaction->num_msg);
        for (unsigned int i = 0; i < transaction->num_msg; i++)
        {
            Napi::Object object = Napi::Object::New(env);
            const pam_message *thisMessage = *(transaction->msg + i);
            object.Set(Napi::String::New(env, "msg_style"), Napi::Number::New(env, thisMessage->msg_style));
            object.Set(Napi::String::New(env, "message"), Napi::String::New(env, thisMessage->msg));
            value.Set(i, object);
        }
        Napi::Value retData = jsCallback.Call({value});

        Napi::Function iveGotTheDataFunc = Napi::Function::New(env, pam_start_ive_got_the_data, NULL, (void *)transaction);
        Napi::Function itsDeadJimFunc = Napi::Function::New(env, pam_start_and_they_say_it_is_a_promise_fail, NULL, (void *)transaction);
        if (retData.IsPromise())
        {
            retData.As<Napi::Promise>().Get("then").As<Napi::Function>().Call({(Napi::Value)iveGotTheDataFunc});
            retData.As<Napi::Promise>().Get("catch").As<Napi::Function>().Call({(Napi::Value)itsDeadJimFunc});
        }
        else if (retData.IsArray() || retData.IsString())
        {
            iveGotTheDataFunc.Call({retData});
        }
        else if (retData.IsObject())
        {
            if (retData.As<Napi::Object>().Has("then") && retData.As<Napi::Object>().Get("then").IsFunction())
            {
                // It's a "promise". I suspect, although god-willing never plan on testing out, this is where Bluebird ends up.
                retData.As<Napi::Object>().Get("then").As<Napi::Function>().Call({(Napi::Value)iveGotTheDataFunc});
                if (retData.As<Napi::Object>().Has("catch") && retData.As<Napi::Object>().Get("catch").IsFunction())
                {
                    // And has the ability to fail.
                    retData.As<Napi::Object>().Get("catch").As<Napi::Function>().Call({(Napi::Value)itsDeadJimFunc});
                }
            }
            else
            {
                // C'Mon... at least pretend to be a promise?
                transaction->pam_fail = true;
                transaction->baton.unlock();
            }
        }
        else
        {
            // WHAT ARE YOU DOING TO ME?!?!?
            transaction->pam_fail = true;
            transaction->baton.unlock();
        }
    };
    transaction->baton.unlock();
    transaction->num_msg = num_msg;
    transaction->msg = msg;
    transaction->convCallback.BlockingCall(transaction, callback);
    transaction->baton.lock();
    if (!transaction->pam_fail)
    {
        *resp = *transaction->resp;
        return PAM_SUCCESS;
    }
    else
    {
        return PAM_CONV_ERR;
    }
}

// pam_start = (service: string, user: string, convCallback: ({msg_style: int, msg: string}[]) => Promise<string[]>)|string[] => Promise<pam_handle>
Napi::Value PamStart(const Napi::CallbackInfo &info)
{
    pam_faithful_transaction *transaction = new pam_faithful_transaction;

    if (info.Length() < 3)
    {
        throw Napi::TypeError::New(info.Env(), "Wrong number of arguments.");
    }

    if (!info[0].IsString())
    {
        throw Napi::TypeError::New(info.Env(), "Argument 0 (service) must be a string.");
    }

    if (!info[1].IsString())
    {
        throw Napi::TypeError::New(info.Env(), "Argument 1 (username) must be a string.");
    }

    if (!info[2].IsFunction())
    {
        throw Napi::TypeError::New(info.Env(), "Argument 2 (convCallback) must be a function.");
    }

    transaction->service = info[0].ToString().Utf8Value();
    transaction->username = info[1].ToString().Utf8Value();
    transaction->baton.lock();
    Napi::Promise::Deferred deferred = Napi::Promise::Deferred::New(info.Env());
    std::thread thread = std::thread(
        [transaction]
        {
            transaction->baton.lock();
            pam_conv *conversation = new pam_conv;
            conversation->conv = pam_start_converser;
            conversation->appdata_ptr = transaction;
            transaction->pam_status = pam_start(transaction->service.c_str(), transaction->username.c_str(), conversation, &transaction->handle);
            transaction->promise.Release();
            transaction->convCallback.Release();
        });
    transaction->convCallback = Napi::ThreadSafeFunction::New(
        info.Env(),
        info[2].As<Napi::Function>(),
        "pam_start thread",
        0,
        1,
        [&thread](Napi::Env)
        { thread.join(); });
    transaction->promise = Napi::ThreadSafeFunction::New(
        info.Env(),
        Napi::Function::New(info.Env(), [](const Napi::CallbackInfo &info) {}),
        "pam_start promise resolution",
        0,
        1,
        [transaction, deferred](Napi::Env env)
        {
            if (transaction->pam_status == PAM_SUCCESS)
            {
                deferred.Resolve(Napi::External<pam_handle_t>::New(env, transaction->handle));
            }
            else
            {
                deferred.Reject(Napi::Number::New(env, transaction->pam_status));
            }
        });
    transaction->baton.unlock();
    return deferred.Promise();
}

Napi::Object init(Napi::Env env, Napi::Object exports)
{
    // pam_start(3)
    exports.Set(Napi::String::New(env, "pam_start"), Napi::Function::New(env, PamStart));

    // pam_conv(3)
    exports.Set(Napi::String::New(env, "PAM_PROMPT_ECHO_OFF"), Napi::Number::New(env, PAM_PROMPT_ECHO_OFF));
    exports.Set(Napi::String::New(env, "PAM_PROMPT_ECHO_ON"), Napi::Number::New(env, PAM_PROMPT_ECHO_ON));
    exports.Set(Napi::String::New(env, "PAM_ERROR_MSG"), Napi::Number::New(env, PAM_ERROR_MSG));
    exports.Set(Napi::String::New(env, "PAM_TEXT_INFO"), Napi::Number::New(env, PAM_TEXT_INFO));

    return exports;
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, init);