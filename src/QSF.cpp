// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "QSF.h"
#include <mutex>
#include <atomic>
#include <vector>
#include <thread>
#include <typeinfo>
#include <unordered_set>
#include <unordered_map>
#include "core/ScopeGuard.h"
#include "core/Strings.h"
#include "core/Random.h"
#include "service/Context.h"
#include "service/Service.h"
#include "utils/Initializer.h"
#include "Env.h"

using std::mutex;
using std::lock_guard;

static const char*  QSF_ROUTER = "inproc://qsf.router";
static const char*  DUMMY_NAME = "#S$ZD@B";

enum
{
    QSF_OK,
    QSF_CLOSING,
    QSF_CLOSED,
};

namespace {

// IPC zmq context, do not need thread pool for I/O operations
static zmq::context_t  s_context(0);

// zmq message router
static std::unique_ptr<zmq::socket_t>   s_router;

static std::unordered_map<std::string, ServicePtr> s_services;
static std::mutex  s_mutex;  // service guard
static std::atomic<int> s_close_flag;


void CheckLibraryVersion()
{
    int major, minor, patch;
    zmq::version(&major, &minor, &patch);
    CHECK(major == ZMQ_VERSION_MAJOR)
        << "expect zmq major ver" << ZMQ_VERSION_MAJOR
        << ", but get " << major;
}

void SystemCommand(const std::string& command)
{
    std::string name = DUMMY_NAME;
    auto dealer = qsf::CreateDealer(name);
    assert(dealer);
    dealer->send("sys", 3, ZMQ_SNDMORE);
    dealer->send(command.c_str(), command.size());
}

// Handle system command messages
bool OnSysMessage(StringPiece from, StringPiece command)
{
    if (command == "exit")
    {
        // send `exit` signal to every service
        lock_guard<mutex> guard(s_mutex);
        for (auto& item : s_services)
        {
            const auto& name = item.first;
            s_router->send(name.c_str(), name.size(), ZMQ_SNDMORE);
            s_router->send("sys", 3, ZMQ_SNDMORE);
            s_router->send("exit", 4);

            zmq::message_t response;
            s_router->recv(&response);
        }
        return false;
    }
    return true;
}

// Message routing between service objects
bool DispatchMessage()
{
    zmq::message_t from;    // where this message came from
    zmq::message_t to;      // where is this message going
    zmq::message_t msg;     // message itself

    s_router->recv(&from);
    s_router->recv(&to);
    s_router->recv(&msg);

    StringPiece source((const char*)from.data(), from.size());
    StringPiece destination((const char*)to.data(), to.size());
    if (destination == "sys")
    {
        StringPiece command((const char*)msg.data(), msg.size());
        return OnSysMessage(source, command);
    }
    else
    {
        s_router->send(to, ZMQ_SNDMORE);
        s_router->send(from, ZMQ_SNDMORE);
        s_router->send(msg);
    }
    return true;
}

// Terminate application when no services exist
void OnServiceCleanup(const std::string& name)
{
    fprintf(stdout, "service [%s] exit.\n", name.c_str());
    Random::release();
    lock_guard<mutex> guard(s_mutex);
    s_services.erase(name);
}

// Callback function for service thread
void ServiceThreadCallback(std::string type, 
                           std::string name, 
                           std::vector<std::string> args)
{
    try
    {
        assert(!type.empty() && !args.empty());
        Context ctx(std::move(qsf::CreateDealer(name)), name);
        auto service = CreateService(type, ctx);
        if (service)
        {
            {
                lock_guard<mutex> guard(s_mutex);
                s_services[name] = service;
            }
            Initializer init;
            service->Run(args);
        }
    }
    catch (std::exception& ex)
    {
        LOG(ERROR) << typeid(ex).name() << ": " << ex.what();
    }
    catch (...)
    {
        LOG(ERROR) << name << ": unknown exception occurs!";
    }

    OnServiceCleanup(name);
}


bool Initialize(const char* filename)
{
    CheckLibraryVersion();

    if (!Env::Initialize(filename))
    {
        return false;
    }

    s_close_flag = QSF_OK;

    s_router.reset(new zmq::socket_t(s_context, ZMQ_ROUTER));
    int linger = 0;
    s_router->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    int mandatory = 1;
    s_router->setsockopt(ZMQ_ROUTER_MANDATORY, &mandatory, sizeof(mandatory));
    int64_t max_msg_size = Env::GetInt("max_ipc_msg_size");
    CHECK(max_msg_size > 0);
    s_router->setsockopt(ZMQ_MAXMSGSIZE, &max_msg_size, sizeof(max_msg_size));
    s_router->bind(QSF_ROUTER);

    return true;
}

void Release()
{
    s_router.reset();
    s_services.clear();
    Env::Release();
}

} // anonymouse namespace


//////////////////////////////////////////////////////////////////////////
namespace qsf {

std::unique_ptr<zmq::socket_t> CreateDealer(const std::string& identity)
{
    std::unique_ptr<zmq::socket_t> dealer(new zmq::socket_t(s_context, ZMQ_DEALER));
    int linger = 0;
    int64_t max_msg_size = Env::GetInt("max_ipc_msg_size");
    dealer->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    dealer->setsockopt(ZMQ_IDENTITY, identity.c_str(), identity.size());
    dealer->setsockopt(ZMQ_MAXMSGSIZE, &max_msg_size, sizeof(max_msg_size));
    dealer->connect(QSF_ROUTER);
    return std::move(dealer);
}

void Stop()
{
    s_close_flag = QSF_CLOSING;
    SystemCommand("exit");
    while (s_close_flag != QSF_CLOSED) // wait for any exist service
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

bool CreateService(const std::string& type, 
                   const std::string& name, 
                   const std::string& str)
{
    // name of 'sys' is reserved
    if (type.empty() || name.empty() 
        || name.size() > MAX_NAME_SIZE 
        || name == "sys")
    {
        return false;
    }
    if (str.empty())
    {
        return false;
    }
    std::vector<std::string> args;
    split(" ", str, args);
    {
        lock_guard<mutex> guard(s_mutex);
        if (s_services.count(name)) // name registered already
        {
            return false;
        }
    }
    std::thread thrd(std::bind(ServiceThreadCallback, type, name, args));
    thrd.detach(); // allow independent execution

    return true;
}

int Start(const char* filename)
{
    if (!Initialize(filename))
    {
        return 1;
    }

    Initializer init;
    auto type = Env::Get("start_type");
    auto name = Env::Get("start_name");
    auto args = Env::Get("start_file");
    CHECK(CreateService(type, name, args));

    while (true)
    {
        try
        {
            if (!DispatchMessage())
            {
                break;
            }
        }
        catch (zmq::error_t& ex)
        {
            LOG(ERROR) << "zmq error[" << ex.num() << "]: " << ex.what();
        }
    }

    s_close_flag = QSF_CLOSED; // finished

    return 0;
}

} // namespace qsf