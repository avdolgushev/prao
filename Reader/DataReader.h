//
// Created by Dolgushev on 04.02.2019.
//

#ifndef PRAO_COMPRESSER_DATEREADER_H
#define PRAO_COMPRESSER_DATEREADER_H

#include "DataHeader.h"
#include "../Calibration/CalibrationDataStorage.h"

#include "../Logger/Logger.h"

#include <cstring>
#include <cmath>
#include <thread>

class DataReader{
    bool is_header_parsed = false; // true if header has been parsed
    int count_read_points = 0, points_before_switch_calibration = 0, calibration_file_duration_in_points = 0; // how many points are already read from file; how many points remains with current calibration data
    int floats_per_point = 0, size_per_point = 0; // how many floats during time resolution; the size of that floats
    //streampos floats_offset = 0;
    DataHeader dataHeader = {}; // header of the file


    CalibrationDataStorage *calibration; // the storage that stores all related calibration data objects
    double *calibration_on_k_left = nullptr, *calibration_on_k_right = nullptr, *calibration_zr_left = nullptr, *calibration_zr_right = nullptr, *curr_on_k = nullptr, *curr_zr = nullptr; // data used during calibration

    ifstream in; // the stream from which reading is performed
    static constexpr int BUFFER_SIZE = 0x1000 * 33 * 7 * 3 * 4; // disk sector size + count of big channels + count of small channels + 48 modules + coef ~11MB
    int buffer_pointer = BUFFER_SIZE; // pointer for buffered read
    char *buffer = nullptr; // buffer for read for processor thread
    char *buffer_second = nullptr; // buffer for read for reader thread
    bool reading_started = false; // if the reading was started (if the reading thread was started)
    bool swap_ready = false; // if the 2 buffers are ready for swapping
    thread *reading_thread = nullptr;

    double timeChunk_duration_star = 0, timeChunk_duration_sun = 0; // seconds of chunk to be read in star time and in sun time
    double file_starTime_start_seconds = 0, curr_file_time_duration_hours = 0;
    //double file_starTime_curr_seconds = 0;
    double ideal_points = 0;
    double MJD_next_file = 0;
    //DataReader * nextDataReader = nullptr;

    void readHeader();
    void prepareReading();
    void readingThread();
    void realloc(double *& base, double const * from);

    /// \breif sets appropriate calibration data regard to count_read_points
    void updateCalibrationData();
    void calibrateArrayPoints(float *point, int count);

    inline void calibrateArrayPointsDetailed(float *point, int size) {
        for (int i = 0; i < size; ++i)
            for (int j = 0; j < floats_per_point; ++j)
                point[i * floats_per_point + j] = (point[i * floats_per_point + j] - curr_zr[j]) / curr_on_k[j];
    }

    /// \breif read points recursive implementation
    /// \param point destination
    /// \param full_count count of points to read
    /// \param offset offset in destination to which read local_count points
    /// \param local_count count of points to read during current recursive call
    int readNextPointsInternal(float *point, int full_count, int offset, int local_count);
public:
    int time_reading = 0;
    int time_calibrating = 0;
    int time_copying = 0;

    /// \param filepath path to file from which read
    /// \param starSeconds_timeChunk_dur duration in star seconds of how many points will be read by call to readNextPoints
    explicit DataReader(string filepath, double starSeconds_timeChunk_dur, double star_time_start);
    ~DataReader();
    void close();

    bool set_MJD_next(double next_MJD);
    double getCurrStarTimeSeconds();
    double getCurrStarTimeSecondsAligned();
    int getCountPointsToNextAlignment(bool need_to_ceil);
    void AlignByStarTimeChunk(/*DataReader *nextDataReader*/);
    /// \breif set calibration signals
    void setCalibrationData(CalibrationDataStorage *calibrationData);

    inline double get_starSeconds_timeChunk_dur(){
        return timeChunk_duration_star;
    }
    /// \breif get size of array that should be passed to read points for the specified in ctor time
    inline int getNeedBufferSize(){
        return floats_per_point * int(timeChunk_duration_sun / dataHeader.tresolution + 1) * 2;
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
    inline double get_MJD_begin(){
        return dataHeader.MJD_begin;
    }

    /// \breif get date start in MJD format
    inline double get_MJD_current(){
        return dataHeader.MJD_begin + (count_read_points * dataHeader.tresolution) / 86400;
    }

    /// \brief check that available at least one chunk of point regard to chunk time
    inline bool eof() {
        return count_read_points + getCountPointsToNextAlignment(true) > ideal_points;
    }

    /// \brief check that available at least count of points
    inline bool eof(int count) {
        return count_read_points + count > ideal_points;
    }

    /// \brief read points regard to time chunk specified in ctor
    /// \param to destination
    /// \return count of points were read
    int readNextPoints(float *to);
    int readNextPoints(float *to, int count, int offset = 0);

    int readRemainder(float *to, int *remainder, int *from_this_file);
};

#endif //PRAO_COMPRESSER_DATEREADER_H
