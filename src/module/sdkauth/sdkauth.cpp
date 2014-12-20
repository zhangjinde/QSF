#include <iostream>
#include <typeinfo>
#include <exception>
#include <zmq.hpp>
#include "core/platform.h"
#include "core/range.h"

using std::cout;
using std::endl;

static zmq::socket_t* self_socket = nullptr;

bool internal_poll()
{
    zmq::message_t from;
    zmq::message_t msg;
    self_socket->recv(&from);
    self_socket->recv(&msg);
    StringPiece name((const char*)from.data(), from.size());
    StringPiece data((const char*)msg.data(), msg.size());
    if (name == "sys" && data == "exit")
    {
        return false;
    }
    else
    {
        cout << name << ": " << data << endl;
        return true;
    }
}

extern "C"
QSF_EXPORT int sdkauth_run(zmq::socket_t* socket, const char* args)
{
    assert(socket);
    self_socket = socket;
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