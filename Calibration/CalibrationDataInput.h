//
// Created by work on 04.02.2019.
//

#ifndef PRAO_COMPRESSER_CALIBRATIONDATAINPUT_H
#define PRAO_COMPRESSER_CALIBRATIONDATAINPUT_H

#include <ctime>
#include <string>
#include <regex>
#include <vector>
#include <sstream>

#include "../Time/Time.h"

#include "../Logger/Logger.h"

using namespace std;

/// to parse calibration data for 1 stand
struct CalibrationDataInput{
    tm datetime = {};
    //time_t time_internal;
    double MJD, temperature;
    int signal_type, is_data_good;
    vector<float> data;

    /// input - is a string read from file and that contains data related to 1 calibration array of signals with header
    explicit CalibrationDataInput(string &input);
};
#endif //PRAO_COMPRESSER_CALIBRATIONDATAINPUT_H
