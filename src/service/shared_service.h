#pragma once

#include "service.h"
#include "core/shared_library.h"


class SharedService : public Service
{
public:
    typedef int(*FnInit)(void*);
    typedef void(*FnRelease)();
    typedef void(*FnRun)(void*);

    explicit SharedService(Context& ctx);
    ~SharedService();

    virtual int run(const std::vector<std::string>& args);

private:
    void initialize(const std::string& path);
    void loadSymbols();

private:
    FnInit      on_init_ = nullptr;
    FnRelease   on_release_ = nullptr;
    FnRun       on_run_ = nullptr;

    std::unique_ptr<SharedLibrary>   this_lib_;
};