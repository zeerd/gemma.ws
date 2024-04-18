#include <stdio.h>
#include <iostream>

#include "gemmathread.h"
#include "logger.h"

LogFunc logger::func = nullptr;

int main(int argc, char* argv[])
{
    if(argc < 2) {
        std::cout << std::endl << "Usage:" << std::endl
                  << "\t" << argv[0] << " ~/.config/Gemma.QT/Setting.conf"
                  << std::endl << std::endl;
    }
    else {
        std::shared_ptr<GemmaThread> m_gemma;
        m_gemma = std::make_shared<GemmaThread>(argv[1]);
        if(m_gemma->joinable()) {
            m_gemma->join();
        }
    }

    return 0;
}
