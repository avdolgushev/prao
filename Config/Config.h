//
// Created by sorrow on 14.03.19.
//

#ifndef PRAO_COMPRESSER_CONFIG_H
#define PRAO_COMPRESSER_CONFIG_H


#include <string>
#include <iostream>

#include "../rapidjson/document.h"
#include "../rapidjson/writer.h"
#include "../rapidjson/stringbuffer.h"
#include "../rapidjson/filereadstream.h"


#define Configuration getObj()

using namespace rapidjson;

struct Config {
    void readFrom(char *fileName);

    std::string fileListPath;
    std::string calibrationListPath;
    size_t localWorkSize;
    double starSeconds;
    float leftPercentile;
    float rightPercentile;
    std::string outputPath;
    int algorithm;

};

extern Config Config_static;
Config & getObj();
#endif //PRAO_COMPRESSER_CONFIG_H
