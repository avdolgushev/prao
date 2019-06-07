//
// Created by sorrow on 14.03.19.
//

#include "Config.h"

void Config::readFrom(char *fileName) {
    std::cout << "parsing config..." << std::endl;
    FILE *fp = fopen(fileName, "r"); // non-Windows use "r"
    char readBuffer[2048];
    FileReadStream is(fp, readBuffer, sizeof(readBuffer));
    Document d;
    d.ParseStream(is);
    fclose(fp);

    assert(d.HasMember("fileListPath"));
    assert(d.HasMember("calibrationListPath"));
    assert(d.HasMember("localWorkSize"));
    assert(d.HasMember("starSecondsZip"));
    assert(d.HasMember("starSecondsWrite"));
    assert(d.HasMember("leftPercentile"));
    assert(d.HasMember("rightPercentile"));
    assert(d.HasMember("outputPath"));
    assert(d.HasMember("logsPath"));
    assert(d.HasMember("algorithm"));

    this->fileListPath = d["fileListPath"].GetString();
    this->calibrationListPath = d["calibrationListPath"].GetString();
    this->localWorkSize = static_cast<size_t>(d["localWorkSize"].GetInt());
    this->starSecondsZip = d["starSecondsZip"].GetDouble();
    this->starSecondsWrite = d["starSecondsWrite"].GetDouble();
    this->leftPercentile = d["leftPercentile"].GetFloat();
    this->rightPercentile = d["rightPercentile"].GetFloat();
    this->outputPath = d["outputPath"].GetString();
    this->logsPath = d["logsPath"].GetString();
    this->algorithm = d["algorithm"].GetInt();

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
}

Config Config_static;
Config & getObj() {
    return Config_static;
}