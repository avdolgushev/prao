//
// Created by work on 06.05.2019.
//

#ifndef PRAO_COMPRESSER_LOGGER_H
#define PRAO_COMPRESSER_LOGGER_H

#include <string>
#include <vector>
#include <direct.h>
#include <thread>
#include <mutex>
#include <fstream>

#include "../Time/Time.h"


#define LOGGER(str) LOG(str, __FILE__, __FUNCTION__, __LINE__)

using namespace std;

class Logger {
    const string default_logging_directory = "loggs";
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

void LOG(string log_string, string file, string func, int line);

#endif //PRAO_COMPRESSER_LOGGER_H
