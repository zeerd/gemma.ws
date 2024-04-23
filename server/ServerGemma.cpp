#include <stdio.h>
#include <signal.h>
#include <iostream>

#include "gemmathread.h"
#include "logger.h"

LogFunc logger::func = nullptr;
bool logger::trace_off = true;
int logger::threshold = logger::I;

static std::shared_ptr<GemmaThread> m_gemma = nullptr;

static void handle_signal(int _signal)
{
    if (_signal == SIGINT) {
        if(m_gemma->running()) {
            m_gemma->stop();
        }
        else {
            exit(0);
        }
        std::cout << std::endl;
    }
}

static void usage(int argc, char* argv[])
{
    (void)argc;
    std::cout << std::endl;
    std::cout << basename(argv[0]) << " Version " << COMMIT_NAME << std::endl;
    std::cout << std::endl;
    std::cout << "Usage:" << std::endl
              << "\t" << argv[0] << " ~/.config/Gemma.QT/Setting.conf"
              << std::endl;
    std::cout << std::endl;
}

int main(int argc, char* argv[])
{
    signal(SIGINT, handle_signal);

    if(argc < 2) {
        usage(argc, argv);
    }
    else {
        m_gemma = std::make_shared<GemmaThread>(argv[1]);
        if(m_gemma->joinable()) {
            logger().os << "Hello!" << std::endl;
            m_gemma->join();
        }
        logger().os << "Bye!" << std::endl;
    }

    return 0;
}
