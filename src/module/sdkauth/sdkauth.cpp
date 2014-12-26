#include <iostream>
#include <typeinfo>
#include <exception>
#include <zmq.hpp>
#include "core/platform.h"
#include "core/range.h"
#include "service/context.h"

using std::cout;
using std::endl;

static Context* self_ctx = nullptr;

bool internal_poll()
{
    bool res = true;
    self_ctx->recv([&](StringPiece name, StringPiece data)
    {
        if (name == "sys" && data == "exit")
        {
            self_ctx->send("sys", "OK");
            res = false;
        }
        else
        {
            cout << name << ": " << data << endl;
        }
    });
    return res;
}

extern "C"
QSF_EXPORT int sdkauth_service_run(Context* ctx, const char* args)
{
    assert(ctx);
    self_ctx = ctx;
    try
    {
        while (internal_poll())
            ;
    }
    catch (std::exception& ex)
    {
        cout << typeid(ex).name() << ": " << ex.what() << endl;
        return 1;
    }
    return 0;
}