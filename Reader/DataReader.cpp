//
// Created by Dolgushev on 04.02.2019.
//

#include "DataReader.h"

DataReader::DataReader(string path, double starSeconds_timeChunk_dur, double star_time_start, double MJD_duration) {
    LOGGER(">> Create reader of raw data (star time of chunk: %f\tfile: %s)", starSeconds_timeChunk_dur, path.c_str());
    in = ifstream(path, ios::binary | ios::in);
    if (!in.good()) {
        LOGGER(">> ERROR. %s file not found", path.c_str());
        throw logic_error(path + " file not found");
    }
    readHeader();
    curr_file_starTime_start = star_time_start;
    curr_file_time_duration_hours = MJD_duration * 24;
    ideal_points = curr_file_time_duration_hours * 60 * 60 / dataHeader.tresolution;



    timeChunk_duration_star = starSeconds_timeChunk_dur;
    timeChunk_duration_sun = to_SunTime(starSeconds_timeChunk_dur);
    points_per_chunk = float(timeChunk_duration_sun / dataHeader.tresolution);
    LOGGER("<< Created reader of raw data (floats per chunk: %f)", points_per_chunk);
}

DataReader::~DataReader() {
    if (buffer != nullptr) {
        delete(buffer);
        buffer = nullptr;
    }
    if (buffer_second != nullptr){
        delete(buffer_second);
        buffer_second = nullptr;
        reading_thread->join();
        delete reading_thread;
        reading_thread = nullptr;
    }
    in.close();
    LOGGER("<< Raw data reader was destroyed. Total time: reading - %f, calibrating - %f, copying - %f", time_reading / (float) CLOCKS_PER_SEC,
            time_calibrating / (float) CLOCKS_PER_SEC, time_copying / (float) CLOCKS_PER_SEC);
}


void DataReader::seekStarHour(double starHour){
    double t = to_starTime(curr_file_time_duration_hours);
    double points_ideal_star_hour = ideal_points / t;
    double proportion = starHour - curr_file_starTime_start;
    if (proportion < 0)
        proportion += 24;
    point_seek(proportion * points_ideal_star_hour);
}

void DataReader::setCalibrationData(CalibrationDataStorage *calibrationData){
    LOGGER(">> Calibration data storage was attached to raw data reader");
    calibration = calibrationData;
    updateCalibrationData();
}


int DataReader::readNextPoints(float *point) {
    int count = int(points_per_chunk + remainder + 0.5);
    remainder += points_per_chunk - count;
    readNextPointsInternal(point, count, 0, count);

    return count;
}

void DataReader::readNextPoints(float *point, int count) {
    readNextPointsInternal(point, count, 0, count);
}

void DataReader::point_seek(double point){
    int point_to_seek_int = int(point + 0.5);
    remainder = float(point_to_seek_int - point);


}

void DataReader::readNextPointsInternal(float *point, int full_count, int offset, int local_count){
    if (!is_header_parsed) // may be disabled for perf purposes
        readHeader();
    if (!reading_started)
        prepareReading();

    if (count_read_points + local_count > dataHeader.npoints) // may be disabled for perf purposes
        throw logic_error("count of points is exceed");


    if (points_before_switch_calibration < local_count){
        int saved = points_before_switch_calibration;
        readNextPointsInternal(point, full_count, offset, points_before_switch_calibration);
        updateCalibrationData();
        readNextPointsInternal(point, full_count, offset + saved, local_count - saved);
    }
    else if (buffer_pointer + local_count * size_per_point > BUFFER_SIZE){
        int points_available = (BUFFER_SIZE - buffer_pointer) / (size_per_point);
        if (points_available > 0)
            readNextPointsInternal(point, full_count, offset, points_available);

        swap_ready = true;
        while (swap_ready)
            this_thread::yield();

        int start = clock();
        calibrateArrayPoints((float *) buffer, min(points_before_switch_calibration, BUFFER_SIZE / (size_per_point)));
        time_calibrating += clock() - start;

        readNextPointsInternal(point, full_count, offset + points_available, local_count - points_available);
    } else {
        int start = clock();
        auto * buff = (float *)&buffer[buffer_pointer];
        auto * point_ = &point[offset];
        for (int i = 0; i < floats_per_point; ++i)
            for (int j = 0; j < local_count; ++j)
                point_[i * full_count + j] = buff[j * floats_per_point + i];

        buffer_pointer += size_per_point * local_count;

        points_before_switch_calibration -= local_count;
        count_read_points += local_count;
        time_copying += clock() - start;
    }
}


void DataReader::readHeader(){
    if (!is_header_parsed){
        LOGGER(">> Read raw data header");
        in >> dataHeader;
        is_header_parsed = true;
        floats_per_point = (dataHeader.nbands + 1) * 48;
        size_per_point = floats_per_point * sizeof(float);
        LOGGER("<< Raw data header was read (floats per point: %d)", floats_per_point);
    }
}

void DataReader::prepareReading() {
    LOGGER(">> First reading of the raw data. BUFFER size is %f MB", BUFFER_SIZE / 1024.0 / 1024.0);

    reading_started = true;
    buffer = new char[BUFFER_SIZE];
    int start = clock();
    in.read(buffer, BUFFER_SIZE); // initial read of first data chunk
    time_reading += clock() - start;
    buffer_pointer = 0;
    if (!in.eof()) { // TODO: think about EOF
        buffer_second = new char[BUFFER_SIZE];
        reading_thread = new thread( [this] { readingThread(); } );
        LOGGER("<< First reading of the raw data. Additional thread was created.");
    } else
        LOGGER("<< First reading of the raw data. Additional thread wasn't created.");
}


void DataReader::readingThread() {
    LOGGER(">> Reading thread start.");

    int start = clock();
    in.read(buffer_second, BUFFER_SIZE); // reading of next data
    time_reading += clock() - start;

    while (true){
        while (!swap_ready)
            this_thread::yield();

        LOGGER(">> Reading thread: swap reading and processing buffers.");

        swap(buffer, buffer_second);
        buffer_pointer = 0;
        swap_ready = false;
        if (in.eof()) // TODO: think about EOF
            break;
        start = clock();
        in.read(buffer_second, BUFFER_SIZE); // reading of next data
        time_reading += clock() - start;
        //this_thread::sleep_for(chrono::milliseconds(250));
    }

    LOGGER("<< Reading thread exit.");
}


void DataReader::realloc(double *& base, double const * from){
    if (base != nullptr)
        delete base;
    base = new double[getPointSize()];
    memcpy(base, from, sizeof(double) * getPointSize());
}

void DataReader::updateCalibrationData(){
    double currentPointMJD = dataHeader.MJD_begin + (count_read_points * dataHeader.tresolution) / 86400;
    CalibrationData * left = calibration->getCalibrationData_left_by_time(currentPointMJD);
    CalibrationData * right = calibration->getCalibrationData_right_by_time(currentPointMJD);

    LOGGER(">> Raw data reader: switch actual calibration file (current MJD: %f, left MJD: %f, right MJD: %f)", currentPointMJD, left->get_MJD(), right->get_MJD());

    points_before_switch_calibration = ((right->get_MJD() - currentPointMJD) * 24 * 60 * 60 + dataHeader.tresolution - 0.000000001) / dataHeader.tresolution;

    realloc(calibration_on_k, left->get_one_kelvin());
    realloc(on_k_step, right->get_one_kelvin());
    realloc(calibration_zr, left->get_zero_level());
    realloc(zr_step, right->get_zero_level());

    int i = getPointSize();
    int n = ((right->get_MJD() - left->get_MJD()) * 24 * 60 * 60 + dataHeader.tresolution - 0.000000001) / dataHeader.tresolution;
    while(--i >= 0){
        on_k_step[i] = (on_k_step[i] - calibration_on_k[i]) / n;
        zr_step[i] = (zr_step[i] - calibration_zr[i]) / n;
    }


    LOGGER(">> Switched actual calibration file (points before next switching: %d)", points_before_switch_calibration);
}


void DataReader::calibrateArrayPoints(float *point, int count) {
    static const int calibrationChunkSize = 100;

    LOGGER(">> Calibration of %d points was started. Calibration points step is %d points", count, calibrationChunkSize);

    for (int t = 0; t < count / calibrationChunkSize; ++t)
        calibrateArrayPointsDetailed(&point[t * calibrationChunkSize * floats_per_point], calibrationChunkSize);

    if (count % calibrationChunkSize != 0)
        calibrateArrayPointsDetailed(&point[(count - count % calibrationChunkSize) * floats_per_point],
                                     count % calibrationChunkSize);

    LOGGER("<< Calibration of %d points was finished", count);
    /*
    for (int i = 0; i < count; ++i) {
        for (int j = 0; j < floats_per_point; ++j) {
            point[i * floats_per_point + j] = (point[i * floats_per_point + j] - calibration_zr[j]) / calibration_on_k[j];
            //calibration_on_k[j] += on_k_step[j];
            //calibration_zr[j] += zr_step[j];
        }
    }

    for(int i = 0; i < floats_per_point; ++i) {
        calibration_on_k[i] += on_k_step[i] * count;
        calibration_zr[i] += zr_step[i] * count;
    }*/
}

void DataReader::calibrateArrayPointsDetailed(float *point, int size) {
    for (int i = 0; i < size; ++i)
        for (int j = 0; j < floats_per_point; ++j)
            point[i * floats_per_point + j] = (point[i * floats_per_point + j] - calibration_zr[j]) / calibration_on_k[j];

    for(int i = 0; i < floats_per_point; ++i) {
        calibration_on_k[i] += on_k_step[i] * size;
        calibration_zr[i] += zr_step[i] * size;
    }
}





/*
void DataReader::calibratePoint(float *point) { // may be manually inlined for perf purposes
    float const *on_k = calibration->get_one_kelvin(), *zr = calibration->get_zero_level();

    for (int i = 0; i < floats_per_point; ++i)
        point[i] = (point[i] - zr[i]) / on_k[i];
}
*/