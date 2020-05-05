//
// Created by work on 06.05.2019.
//

#ifndef PRAO_COMPRESSER_LOGGER_H
#define PRAO_COMPRESSER_LOGGER_H

#include <string>
#include <vector>
# if defined(_WIN32)
#include <direct.h> // only for windows
#else
#include <sys/stat.h> // only for unix
# endif
#include <cstdarg>
#include <thread>
#include <mutex>
#include <fstream>

#include "../Time/Time.h"
#include "../Config/Config.h"


#define LOGGER(...) LOG(__FILE__, __FUNCTION__, __LINE__, __VA_ARGS__)

using namespace std;

class Logger {
    string _filename;

    vector<string> storage;
    mutex storage_mutex;
    thread *writer_thread = nullptr;

    bool logger_stopperd = false;

    void writer();

public:
    Logger();
    explicit Logger(string filename) : _filename(move(filename)) { }
    ~Logger();

    void LOG(string log_string, string file, string func, int line);
    void stop_logging();
};

extern Logger Logger_obj;

void LOG(string file, string func, int line, ...);

#endif //PRAO_COMPRESSER_LOGGER_H
