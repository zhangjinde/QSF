#include <cstdio>
#include <csignal>
#include <exception>
#include <typeinfo>
#include "QSF.h"
#include "core/Logging.h"

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
        qsf::Stop();
        exit(1);
    });

    try
    {
        return qsf::Start(default_file);
    }
    catch (std::exception& ex)
    {
        LOG(ERROR) << typeid(ex).name() << ": " << ex.what();
        return EXIT_FAILURE;
    }
}
