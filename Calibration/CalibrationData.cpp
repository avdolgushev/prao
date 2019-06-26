//
// Created by work on 04.02.2019.
//

#include "CalibrationData.h"


void CalibrationData::calculateCoefs() {
    //LOGGER(">> Calculating calibration coefs (one kelvin & zero level)");
    int size = small->data.size();
    float n, Tpr;
    float *sm = small->data.data(), *bg = big->data.data();
    for (int i = 0; i < size; ++i) {
        n = bg[i] / sm[i];
        Tpr = (Tgs - n * Teq) / (n - 1);
        one_kelvin[i] = (bg[i] - sm[i]) / (Tgs - Teq);
        zero_level[i] = sm[i] - one_kelvin[i] * (Tpr + Teq);
    }

    small->data.clear();
    big->data.clear();
    LOGGER("<< Calculating calibration coefs (one kelvin & zero level)");
}

CalibrationData::CalibrationData(double time_MJD) : MJD(time_MJD) { }

CalibrationData::CalibrationData(string &data1, string &data2) {
    LOGGER(">> Parsing big & small calibration signals");
    small = new CalibrationDataInput(data1);
    big = new CalibrationDataInput(data2);

    if (small->temperature == Tgs && big->temperature == Teq)
        swap(small, big);

    if (small->MJD != big->MJD) {
        LOGGER("<< ERROR. small signal's MJD != big signal's MJD");
        throw logic_error("small->MJD != big->MJD");
    }

    MJD = small->MJD;

    unsigned int size = small->data.size();
    one_kelvin = new double[size];
    zero_level = new double[size];

    calculateCoefs();
    LOGGER("<< Parsing big & small calibration signals, MJD: %f", small->MJD);
}

CalibrationData::~CalibrationData() {
    if (small != nullptr)
        delete small;
    if (big != nullptr)
        delete big;
    if (one_kelvin != nullptr)
        delete[] one_kelvin;
    if (zero_level != nullptr)
        delete[] zero_level;
    small = big = nullptr;
    one_kelvin = zero_level = nullptr;
}

double const * CalibrationData::get_one_kelvin(){
    return one_kelvin;
}

double const * CalibrationData::get_zero_level(){
    return zero_level;
}

double CalibrationData::get_MJD() {
    return MJD;
}

void CalibrationData::print_date() {
    tm_AddDefault(small->datetime);
    cout.precision(8);
    cout << "UTC: " << small->datetime.tm_year << "\t" << small->datetime.tm_mon << "\t" << small->datetime.tm_mday << "\t" << small->datetime.tm_hour << "\t" << small->datetime.tm_min << "\t";
    cout << "MJD: " << fixed << small->MJD << endl;
    tm_SubDefault(small->datetime);
}

bool CalibrationData::operator<(const CalibrationData &c) const {
    return MJD < c.MJD;
}