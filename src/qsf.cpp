// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf.h"
#include <mutex>
#include <vector>
#include <thread>
#include <typeinfo>
#include <unordered_set>
#include <unordered_map>
#include "core/scope_guard.h"
#include "core/strings.h"
#include "core/random.h"
#include "service/context.h"
#include "service/service.h"
#include "env.h"


using std::mutex;
using std::lock_guard;


static const char*  QSF_ROUTER = "inproc://qsf.router";
static const char*  DUMMY_NAME = "#S$ZD@B";

namespace {

// Global zmq context, do not need thread pool for I/O operations
static zmq::context_t  s_context(0);

// 0mq message router
static std::unique_ptr<zmq::socket_t>   s_router;

static std::unordered_map<std::string, ServicePtr> s_services;
static std::mutex  s_mutex;  // service guard

void checkLibraryVersion()
{
    int major, minor, patch;
    zmq::version(&major, &minor, &patch);
    CHECK(major == ZMQ_VERSION_MAJOR)
        << "expect zmq major ver" << ZMQ_VERSION_MAJOR
        << ", but get " << major;
}

void systemCommand(const std::string& command)
{
    std::string name = DUMMY_NAME;
    auto dealer = qsf::createDealer(name);
    assert(dealer);
    dealer->send(name.c_str(), name.size(), ZMQ_SNDMORE);
    dealer->send("sys", 3, ZMQ_SNDMORE);
    dealer->send(command.c_str(), command.size());
}

// Handle system command messages
bool onSysMessage(StringPiece from, StringPiece command)
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
        }
    }
    else
    {
        // force quit
        if (command == "shutdown")
        {
            return false;
        }
    }
    return true;
}

// Message routing between service objects
bool dispatchMessage()
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
        return onSysMessage(source, command);
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
void onServiceCleanup(const std::string& name)
{
    fprintf(stdout, "service [%s] exit.\n", name.c_str());
    Random::release();
    {
        lock_guard<mutex> guard(s_mutex);
        if (s_services.size() == 1) // this is the last service object
        {
            systemCommand("shutdown");
        }
        s_services.erase(name);
    }
}

// Callback function for service thread
void threadCallback(std::string type, std::string name, std::vector<std::string> args)
{
    try
    {
        assert(!type.empty() && !args.empty());
        Context ctx(name);
        auto service = createService(type, ctx);
        if (service)
        {
            {
                lock_guard<mutex> guard(s_mutex);
                s_services[name] = service;
            }
            service->run(args);
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

    onServiceCleanup(name);
}


bool initialize(const char* filename)
{
    checkLibraryVersion();

    if (!Env::initialize(filename))
    {
        return false;
    }

    // config easylogging++
    auto conf_text = Env::get("logconf");
    el::Configurations conf;
    conf.setToDefault();
    conf.parseFromText(conf_text);

    int mandatory = 1;
    int linger = 0;
    int64_t max_msg_size = MAX_MSG_SIZE;
    s_router.reset(new zmq::socket_t(s_context, ZMQ_ROUTER));
    //s_router->setsockopt(ZMQ_ROUTER_MANDATORY, &mandatory, sizeof(mandatory));
    s_router->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    s_router->setsockopt(ZMQ_MAXMSGSIZE, &max_msg_size, sizeof(max_msg_size));
    s_router->bind(QSF_ROUTER);

    return true;
}

void release()
{
    s_router.reset();
    s_services.clear();
    Env::release();
}

} // anonymouse namespace


//////////////////////////////////////////////////////////////////////////
namespace qsf {

std::unique_ptr<zmq::socket_t> createDealer(const std::string& identity)
{
    std::unique_ptr<zmq::socket_t> dealer(new zmq::socket_t(s_context, ZMQ_DEALER));
    int linger = 0;
    int64_t max_msg_size = MAX_MSG_SIZE;
    dealer->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    dealer->setsockopt(ZMQ_IDENTITY, identity.c_str(), identity.size());
    dealer->setsockopt(ZMQ_MAXMSGSIZE, &max_msg_size, sizeof(max_msg_size));
    dealer->connect(QSF_ROUTER);
    return std::move(dealer);
}

void stop()
{
    systemCommand("exit");
    while (true) // wait for any exist service
    {
        bool is_any_service = false;
        {
            lock_guard<mutex> guard(s_mutex);
            is_any_service = !s_services.empty();
        }
        if (is_any_service)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            continue;
        }
        break;
    }
}

bool createService(const std::string& type, const std::string& name, const std::string& str)
{
    // name of 'sys' is reserved
    if (type.empty() || name.empty() || name.size() > MAX_NAME_SIZE || name == "sys")
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
    std::thread thrd(std::bind(threadCallback, type, name, args));
    thrd.detach(); // allow independent execution

    return true;
}

int start(const char* filename)
{
    SCOPE_EXIT{ release(); };
    if (initialize(filename))
    {
        auto name = Env::get("start_name");
        auto args = Env::get("start_file");
        createService("luasandbox", name, args);

        while (dispatchMessage())
            ;
        return 0;
    }
    return 1;
}

} // namespace qsf
