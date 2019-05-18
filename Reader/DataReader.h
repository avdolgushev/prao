//
// Created by Dolgushev on 04.02.2019.
//

#ifndef PRAO_COMPRESSER_DATEREADER_H
#define PRAO_COMPRESSER_DATEREADER_H

#include "DataHeader.h"
#include "../Point/Point.h"
#include "../Calibration/CalibrationDataStorage.h"

#include "../Logger/Logger.h"

#include <cstring>
#include <cmath>
#include <thread>

class DataReader{
    bool is_header_parsed = false; // true if header has been parsed
    int count_read_points = 0, points_before_switch_calibration = 0; // how many points are already read from file; how many points remains with current calibration data
    int floats_per_point = 0, size_per_point = 0; // how many floats during time resolution; the size of that floats
    float points_per_chunk = 0, remainder = 0; // how many points the time chunk consists of; the remainder while reading integer of points_per_chunk
    DataHeader dataHeader = {}; // header of the file

    CalibrationDataStorage *calibration; // the storage that stores all related calibration data objects
    double *calibration_on_k = nullptr, *on_k_step = nullptr, *calibration_zr = nullptr, *zr_step = nullptr; // data used during calibration

    ifstream in; // the stream from which reading is performed
    static constexpr int BUFFER_SIZE = 0x1000 * 33 * 7 * 3 * 4; // disk sector size + count of big channels + count of small channels + 48 modules + coef ~11MB
    int buffer_pointer = BUFFER_SIZE; // pointer for buffered read
    char *buffer = nullptr; // buffer for read for processor thread
    char *buffer_second = nullptr; // buffer for read for reader thread
    bool reading_started = false; // if the reading was started (if the reading thread was started)
    bool swap_ready = false; // if the 2 buffers are ready for swapping
    thread *reading_thread = nullptr;

    double timeChunk_duration_star = 0, timeChunk_duration_sun = 0; // seconds of chunk to be read in star time and in sun time


    void readHeader();
    void prepareReading();
    void readingThread();
    void realloc(double *& base, double const * from);

    /// \breif sets appropriate calibration data regard to count_read_points
    void updateCalibrationData();
    void calibrateArrayPoints(float *point, int count);
    void calibrateArrayPointsDetailed(float *point, int size);

    /// \breif read points recursive implementation
    /// \param point destination
    /// \param full_count count of points to read
    /// \param offset offset in destination to which read local_count points
    /// \param local_count count of points to read during current recursive call
    void readNextPointsInternal(float *point, int full_count, int offset, int local_count);
public:
    int time_reading = 0;
    int time_calibrating = 0;
    int time_copying = 0;

    /// \param filepath path to file from which read
    /// \param starSeconds_timeChunk_dur duration in star seconds of how many points will be read by call to readNextPoints
    explicit DataReader(string filepath, double starSeconds_timeChunk_dur);
    ~DataReader();

    /// \breif set calibration signals
    void setCalibrationData(CalibrationDataStorage *calibrationData);

    /// \breif get size of array that should be passed to read points for the specified in ctor time
    inline int getNeedBufferSize(){
        return floats_per_point * int(points_per_chunk + 1);
    }

    /// \breif get count of floats in point regard to file time resolution
    inline int getPointSize() {
        return floats_per_point;
    }

    /// \breif get DataHeader
    inline DataHeader getDataHeader() {
        return dataHeader;
    }

    /// \breif get date start in MJD format
    inline float get_MJD_begin(){
        return dataHeader.MJD_begin;
    }

    /// \brief check that available at least one chunk of point regard to chunk time
    inline bool eof() {
        return ceil(count_read_points + points_per_chunk + remainder) >= dataHeader.npoints;
    }

    /// \brief check that available at least count of points
    inline bool eof(int count) {
        return count_read_points + count >= dataHeader.npoints;
    }

    /// \brief read points regard to time chunk specified in ctor
    /// \param to destination
    /// \return count of points were read
    int readNextPoints(float *to);
    void readNextPoints(float *to, int count);
};

#endif //PRAO_COMPRESSER_DATEREADER_H
