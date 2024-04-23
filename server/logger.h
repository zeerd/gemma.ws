#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <sstream>

using LogFunc = std::function<void(const char*)>;

class logger {
public:
    static const int TI = 1;
    static const int TO = 2;
    static const int E = 3;
    static const int W = 4;
    static const int I = 5;
    static const int D = 6;
    static const int V = 7;
public:
    logger(int f = I) : flag(f) {}
    ~logger() {
        if(trace_off && flag <= TO) {
        }
        else if(flag > threshold) {
        }
        else {
            std::stringstream ss;
            ss << "[" << n[flag] << "]";
            ss << os.str();
            if(flag == TI) { ss << " IN\n"; }
            if(flag == TO) { ss << " OUT\n"; }
            if(func == NULL) {
                std::cout << ss.str();
            } else {
                func(ss.str().c_str());
            }
        }
    }
private:
    static bool trace_off;
    static int threshold;
    const char* n[8] = {
        "", "Trace", "Trace", "Err", "War", "Inf", "dbg", "vbs"
    };
public:
    std::ostringstream os;
    static LogFunc func;
    int flag;
};

#endif // LOGGER_H
