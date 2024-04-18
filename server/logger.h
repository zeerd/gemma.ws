#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <iostream>
#include <sstream>

using LogFunc = std::function<void(const char*)>;

class logger {
public:
    static const int E = 1;
    static const int W = 2;
    static const int I = 3;
    static const int D = 4;
    static const int V = 5;
    static const int TI = 6;
    static const int TO = 7;
public:
    logger(int f = I) : flag(f) {}
    ~logger() {
        if(trace_off && flag >= level) {
        }
        else if(flag > level) {
        }
        else {
            std::stringstream ss;
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
    static const bool trace_off = true;
    static const bool level = I;
public:
    std::ostringstream os;
    static LogFunc func;
    int flag;
};

#endif // LOGGER_H
