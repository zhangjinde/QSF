#include <cstdio>
#include <csignal>
#include <exception>
#include <typeinfo>
#include "qsf.h"
#include "core/logging.h"



using namespace std;


int main(int argc, char* argv[])
{
    const char* default_file = "config";
    if (argc >= 2)
    {
        default_file = argv[1];
    }
    
    // Ctrl + C interrupt signal
    signal(SIGINT, [](int sig)
    {
        qsf::stop();
        exit(1);
    });

    int r = EXIT_SUCCESS;
    try
    {
        r = qsf::start(default_file);
    }
    catch (std::exception& ex)
    {
        LOG(ERROR) << typeid(ex).name() << ": " << ex.what();
        r = EXIT_FAILURE;
    }
    return r;
}
