//
// Created by sorrow on 14.03.19.
//

#ifndef PRAO_COMPRESSER_CONFIG_H
#define PRAO_COMPRESSER_CONFIG_H


#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>

# if defined(_WIN32)
#include <direct.h> // only for windows
#else
#include <sys/stat.h> // only for unix
# endif

#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/prettywriter.h"
#include "../rapidjson/stringbuffer.h"
#include "../rapidjson/filereadstream.h"

#include "../Logger/Logger.h"

#define EPS 1e-9

#define Configuration getObj()

using namespace rapidjson;

struct Config {
    int readFrom(const char *fileName);

    std::string fileListPath;
    std::string calibrationListPath;
    size_t localWorkSize;
    double starSecondsZip;
    double starSecondsWrite;
    float leftPercentile;
    float rightPercentile;
    std::string outputPath;
    std::string logsPath;
    std::string kernelPath;

};

extern Config Config_static;
Config & getObj();
#endif //PRAO_COMPRESSER_CONFIG_H
