#pragma once

#include "service.h"
#include "core/shared_library.h"


class SharedService : public Service
{
public:
    explicit SharedService(Context& ctx);
    ~SharedService();

    virtual int run(const std::vector<std::string>& args);

private:
    void initialize(const std::string& path);

private:
    int (*on_run_)(void*, const char*) = nullptr;
    std::unique_ptr<SharedLibrary>  this_lib_;
};