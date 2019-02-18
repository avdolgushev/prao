//
// Created by Dolgushev on 04.02.2019.
//

#ifndef PRAO_COMPRESSER_DATEREADER_H
#define PRAO_COMPRESSER_DATEREADER_H

#include "DataHeader.h"
#include "../Point/Point.h"
#include "../Calibration/CalibrationDataStorage.h"

#include <cstring>


class DataReader{
    bool is_header_parsed = false;
    int count_read_points = 0, points_before_switch_calibration = 0;
    int floats_per_point = 0, size_per_point = 0;
    DataHeader dataHeader = {};

    CalibrationDataStorage *calibration;
    double *calibration_on_k = nullptr, *on_k_step = nullptr, *calibration_zr = nullptr, *zr_step = nullptr;

    ifstream in;
    static constexpr int BUFFER_SIZE = 0x1000 * 33 * 7 * 3 * 4; // disk sector size + count of big channels + count of small channels + 48 modules + coef ~11MB
    int buffer_pointer = BUFFER_SIZE;
    char *buffer = nullptr;

    double timeChunk_duration_star = 0, timeChunk_duration_sun = 0;


    void readHeader();
    //void calibratePoint(float * point);
    void realloc(double *& base, double const * from);
    void updateCalibrationData();
    void calibrateArrayPoints(float *point, int count);
    void calibrateArrayPointsDetailed(float *point, int size);
public:

    explicit DataReader(string filepath, double starSeconds_timeChunk_dur);
    ~DataReader();
    void setCalibrationData(CalibrationDataStorage *calibrationData);

    /// used only for testing
    inline int getPointSize() {
        return floats_per_point;
    }

    inline float get_MJD_begin(){
        return dataHeader.MJD_begin;
    }

    inline bool eof() {
        return count_read_points >= dataHeader.npoints;
    }
/*
    /// ATTENTION: function depends on offset of data array in PointBase class. data array should be placed at 0 offset (or offset 4 if class has virtual methods == is_polymorphic<PointBase>::value)
    inline void readNextPoint(PointBase *to) {
        readNextPoints((float *) to);
    }

    inline void readNextPoint(PointSmall &to) {
        readNextPoints((float *) to.data);
    }

    inline void readNextPoint(PointBig &to) {
        readNextPoints((float *) to.data);
    }
    */
    void readNextPoints(float *to, int count = 1);
};

#endif //PRAO_COMPRESSER_DATEREADER_H