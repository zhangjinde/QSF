#include <cstdio>
#include <csignal>
#include "qsf.h"


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
        qsf::Stop();
        exit(1);
    });

    return qsf::Start(file);
}
