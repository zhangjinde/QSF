#include <cstdio>
#include <csignal>
#include <exception>
#include <typeinfo>
#include "qsf.h"
#include "core/logging.h"



using namespace std;


int main(int argc, char* argv[])
{
    const char* file = "config";
    if (argc > 2)
    {
        file = argv[1];
    }
    
    // Ctrl + C interrupt signal
    signal(SIGINT, [](int sig)
    {
        qsf::stop();
        exit(1);
    });

    int r = 0;
    try
    {
        r = qsf::start(file);
    }
    catch (std::exception& ex)
    {
        LOG(ERROR) << typeid(ex).name() << ": " << ex.what();
    }
    return 0;
}
