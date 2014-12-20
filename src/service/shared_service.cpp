#include "shared_service.h"
#include "core/conv.h"
#include "core/strings.h"

using std::string;

SharedService::SharedService(Context& ctx)
    : Service("SharedService", ctx)
{
}

SharedService::~SharedService()
{
}

void SharedService::initialize(const std::string& path)
{
    this_lib_.reset(new SharedLibrary(path));
    string name = this_lib_->path() + "_run";
    on_run_ = (decltype(on_run_))this_lib_->getSymbol(name);
    if (on_run_ == nullptr)
    {
        throw std::runtime_error(to<string>("symbole [", name, "] not found"));
    }
}

int SharedService::run(const std::vector<std::string>& args)
{
    if (args.empty())
    {
        return 1;
    }
    std::string argument = join(" ", args);
    auto& ctx = this->context();
    auto& socket = ctx.socket();
    initialize(args[0]);
    int r = 0;
    try
    {
        r = on_run_(&socket, argument.c_str());
    }
    catch (std::exception& ex)
    {
        r = 1;
    }
    return r;
}