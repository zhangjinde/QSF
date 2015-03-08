// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "SharedService.h"
#include "core/Conv.h"
#include "core/Strings.h"
#include "core/Logging.h"

using std::string;

#define SYMBOL_POSTFIX  "_run"

SharedService::SharedService(Context& ctx)
    : Service("SharedService", ctx)
{
}

SharedService::~SharedService()
{
}

void SharedService::Initialize(const std::string& path)
{
    this_lib_.reset(new SharedLibrary(path));
    string name = this_lib_->path() + SYMBOL_POSTFIX;
    on_run_ = (decltype(on_run_))this_lib_->getSymbol(name);
    if (on_run_ == nullptr)
    {
        throw std::runtime_error(to<string>("symbole [", name, "] not found"));
    }
}

int SharedService::Run(const std::vector<std::string>& args)
{
    if (args.empty())
    {
        return 1;
    }
    std::string argument = join(" ", args);
    auto& ctx = this->GetCtx();
    Initialize(args[0]);
    int r = 0;
    try
    {
        r = on_run_(&ctx, argument.c_str());
    }
    catch (std::exception& ex)
    {
        r = 1;
        LOG(ERROR) << typeid(ex).name() << ": " << ex.what();
    }
    return r;
}