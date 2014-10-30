// Copyright (C) 2014 ichenq@gmail.com. All rights reserved.
// Distributed under the terms and conditions of the Apache License.
// See accompanying files LICENSE.

#include "qsf.h"
#include <thread>
#include <typeinfo>
#include "core/scope_guard.h"
#include "env.h"

using std::mutex;
using std::lock_guard;
using std::string;
using std::vector;
using std::unique_ptr;


static const char*  QSF_QUEUE = "inproc://router.queue";
static const char*  DUMMY_NAME = "#S$ZD@B";

namespace {

// global zmq context, do not need thread pool for I/O operations
static zmq::context_t  s_context(0);

// zmq message router
static std::unique_ptr<zmq::socket_t>   s_router;

static std::unordered_map<std::string, ServicePtr> s_services;
static std::mutex  s_mutex;  // mutex to guard services


void SystemCommand(const std::string& command)
{
    string id = DUMMY_NAME;
    auto dealer = qsf::CreateDealer(id);
    assert(dealer);
    dealer->send(id.c_str(), id.size(), ZMQ_SNDMORE);
    dealer->send("sys", 3, ZMQ_SNDMORE);
    dealer->send(command.c_str(), command.size());
}

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
        }
    }
    else
    {
        // force quit
        if (command == "quit")
        {
            return false;
        }
    }
    return true;
}

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

void ServiceCleanup(const string& id)
{
    fprintf(stdout, "service [%s] exit.\n", id.c_str());
    {
        lock_guard<mutex> guard(s_mutex);
        if (s_services.size() == 1) // this is the last service object
        {
            SystemCommand("quit");
        }
        s_services.erase(id);
    }
}


void ThreadCallback(string type, string id, vector<string> args)
{
    try
    {
        assert(!type.empty() && !args.empty());
        Context ctx(id);
        auto service = CreateService(type, ctx);
        if (service)
        {
            {
                lock_guard<mutex> guard(s_mutex);
                s_services[id] = service;
            }
            service->Run(args);
        }
    }
    catch (std::exception& ex)
    {
        LOG(ERROR) << typeid(ex).name() << ": " << ex.what();
    }
    catch (...)
    {
        LOG(ERROR) << id << ": unknown exception occurs!";
    }

    ServiceCleanup(id);
}


bool Initialize(const char* filename)
{
    if (!Env::Initialize(filename))
    {
        return false;
    }

    // config easylogging++
    auto conf_text = Env::Get("logconf");
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
    s_router->bind(QSF_QUEUE);

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

unique_ptr<zmq::socket_t> CreateDealer(const string& identity)
{
    unique_ptr<zmq::socket_t> dealer(new zmq::socket_t(s_context, ZMQ_DEALER));
    int linger = 0;
    int64_t max_msg_size = MAX_MSG_SIZE;
    dealer->setsockopt(ZMQ_LINGER, &linger, sizeof(linger));
    dealer->setsockopt(ZMQ_IDENTITY, identity.c_str(), identity.size());
    dealer->setsockopt(ZMQ_MAXMSGSIZE, &max_msg_size, sizeof(max_msg_size));
    dealer->connect(QSF_QUEUE);
    return std::move(dealer);
}

void Stop()
{
    SystemCommand("exit");
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

// create an service
bool CreateService(const string& type, const string& id, const string& str)
{
    // name of 'sys' is reserved
    if (type.empty() || id.empty() || id.size() > MAX_NAME_SIZE || id == "sys")
    {
        return false;
    }
    if (str.empty())
    {
        return false;
    }
    vector<string> args;
    split(" ", str, args);
    {
        lock_guard<mutex> guard(s_mutex);
        if (s_services.count(id)) // name registered already
        {
            return false;
        }
    }
    std::thread thrd(std::bind(ThreadCallback, type, id, args));
    thrd.detach(); // allow independent execution

    return true;
}

int Start(const char* filename)
{
    try
    {
        SCOPE_EXIT{ Release(); };
        if (Initialize(filename))
        {
            const string& name = Env::Get("start_name");
            const string& args = Env::Get("start_file");
            CreateService("luasandbox", name, args);

            while (DispatchMessage())
                ;
        }
    }
    catch (std::exception& ex)
    {
        LOG(ERROR) << typeid(ex).name() << ": " << ex.what();
        return 1;
    }
    return 0;
}

} // namespace qsf
