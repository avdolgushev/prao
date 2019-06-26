//
// Created by work on 06.05.2019.
//

#include "Logger.h"

void Logger::writer(){
    while (!logger_stopperd || !storage.empty()){
        {
            lock_guard<mutex> locker(storage_mutex);
            ofstream file(Configuration.logsPath + _filename, fstream::out | fstream::app);
            bool t = file.good();
            for (string &s: storage)
                file << s << endl;
            file.close();
            storage.clear();
        }
        this_thread::sleep_for(chrono::milliseconds(100));
    }
}


Logger::Logger() {
    _filename = '/' + getCurrentDateTimeStr() + ".log";
}

Logger::~Logger() {
    stop_logging();
}

void Logger::LOG(string log_string, string file, string func, int line){

    string code_debug_info = "\t(file: " + file + "\tfunction: " + func + "\tline: " + to_string(line) + ")";
    if (false)
        log_string = getCurrentDateTimeStr() + code_debug_info + ": " + log_string;
    else
        log_string = getCurrentDateTimeStr() + ": " + log_string;

    {
        lock_guard<mutex> locker(storage_mutex);
        storage.push_back(log_string);
    }

    if (writer_thread == nullptr)
        writer_thread = new thread([this] { writer(); });
}

void Logger::stop_logging(){
    logger_stopperd = true;
    if (writer_thread != nullptr)
        writer_thread->join();
}

Logger Logger_obj;
void LOG(string file, string func, int line, ...) {
    char buffer[0x1000];

    va_list args;
    va_start (args, line);
    string format = va_arg(args, char*);
    vsnprintf(buffer, 0x1000, format.c_str(), args);
    va_end (args);
    Logger_obj.LOG(buffer, move(file), move(func), line);
}