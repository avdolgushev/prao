//
// Created by sorrow on 14.03.19.
//

#include "Config.h"

int Config::readFrom(const char *fileName) {
    std::ifstream inp(fileName);
    if (!inp.good()){
        std::cout << "config file not found at " << fileName <<  std::endl;
        return 1;
    }
    std::stringstream buffer;
    buffer << inp.rdbuf();

    std::cout << "parsing config from " << fileName <<  std::endl;
    Document d;
    d.Parse(buffer.str().c_str());

    assert(d.HasMember("fileListPath"));
    assert(d.HasMember("calibrationListPath"));
    assert(d.HasMember("localWorkSize"));
    assert(d.HasMember("starSecondsZip"));
    assert(d.HasMember("starSecondsWrite"));
    assert(d.HasMember("leftPercentile"));
    assert(d.HasMember("rightPercentile"));
    assert(d.HasMember("outputPath"));
    assert(d.HasMember("logsPath"));
    assert(d.HasMember("kernelPath"));

    this->fileListPath = d["fileListPath"].GetString();
    this->calibrationListPath = d["calibrationListPath"].GetString();
    this->localWorkSize = static_cast<size_t>(d["localWorkSize"].GetInt());
    this->starSecondsZip = d["starSecondsZip"].GetDouble();
    this->starSecondsWrite = d["starSecondsWrite"].GetDouble();
    this->leftPercentile = d["leftPercentile"].GetFloat();
    this->rightPercentile = d["rightPercentile"].GetFloat();
    this->outputPath = d["outputPath"].GetString();
    this->logsPath = d["logsPath"].GetString();
    this->kernelPath = d["kernelPath"].GetString();

    double tmp;
    if (modf(starSecondsWrite / starSecondsZip, &tmp) > EPS)
        throw std::logic_error("starSecondsWrite % starSecondsZip != 0");
    if (leftPercentile > rightPercentile)
        throw std::logic_error("leftPercentile > rightPercentile");
    if (starSecondsWrite > 86400)
        throw std::logic_error("starSecondsWrite > 86400");
    if (localWorkSize < 0 || localWorkSize > 1024)
        throw std::logic_error("localWorkSize < 0 || localWorkSize > 1024");

    _mkdir(outputPath.c_str());
    _mkdir(logsPath.c_str());

    return 0;
}

Config Config_static;
Config & getObj() {
    return Config_static;
}