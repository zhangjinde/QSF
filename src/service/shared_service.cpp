#include "shared_service.h"
#include "core/conv.h"

using std::string;

SharedService::SharedService(Context& ctx)
    : Service("SharedService", ctx)
{
}

SharedService::~SharedService()
{
    if (on_release_)
    {
        on_release_();
    }
}

void SharedService::loadSymbols()
{
    string name = this_lib_->path();
    name.erase(name.find('.'));
    string symbol = name + "_init";
    on_init_ = (FnInit)this_lib_->getSymbol(symbol);
    if (on_init_ == nullptr)
    {
        throw std::runtime_error(to<string>("symbole [", symbol, "] not found"));
    }
    symbol = name + "_run";
    on_run_ = (FnRun)this_lib_->getSymbol(name + "run");
    if (on_run_ == nullptr)
    {
        throw std::runtime_error(to<string>("symbole [", symbol, "] not found"));
    }
    symbol = name + "_release";
    on_release_ = (FnRelease)this_lib_->getSymbol(symbol);
}

int SharedService::run(const std::vector<std::string>& args)
{
    if (args.size() < 1)
    {
        return 1;
    }
    initialize(args[0]);
    return 0;
}

void SharedService::initialize(const std::string& path)
{
    this_lib_.reset(new SharedLibrary(path));
    loadSymbols();
    CHECK(on_init_ && on_run_);
    auto& ctx = this->context();
    void* socket = (void*)ctx.socket();
    on_init_(socket);
}