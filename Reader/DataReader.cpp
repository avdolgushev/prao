//
// Created by Dolgushev on 04.02.2019.
//

#include "DataReader.h"

DataReader::DataReader(string path, double starSeconds_timeChunk_dur, double star_time_start) {
    LOGGER(">> Creating reader of raw data (star time of chunk: %f\tfile: %s)", starSeconds_timeChunk_dur, path.c_str());
    in = ifstream(path, ios::binary | ios::in);
    if (!in.good()) {
        LOGGER(">> ERROR. %s file not found", path.c_str());
        throw logic_error(path + " file not found");
    }
    readHeader();
    //floats_offset = in.tellg();
    file_starTime_start_seconds = star_time_start * 60 * 60;
    //file_starTime_curr_seconds = star_time_start * 60 * 60;
    ideal_points = dataHeader.npoints;

    timeChunk_duration_star = starSeconds_timeChunk_dur;
    timeChunk_duration_sun = to_SunTime(starSeconds_timeChunk_dur);
    double points_per_chunk = timeChunk_duration_sun / dataHeader.tresolution;

    curr_on_k = new double[getPointSize()];
    curr_zr = new double[getPointSize()];
    LOGGER("<< Created reader of raw data (floats per chunk: %f)", points_per_chunk);
}

template<class T>
void deleter(T *& pointer){
    if (pointer != nullptr){
        delete[] pointer;
        pointer = nullptr;
    }
}


void DataReader::close() {
    deleter(buffer);

    if (buffer_second != nullptr){
        reading_thread->join();
        delete[] buffer_second;
        buffer_second = nullptr;
        delete reading_thread;
        reading_thread = nullptr;
    }
    in.close();

    deleter(calibration_on_k_left);
    deleter(calibration_on_k_right);
    deleter(calibration_zr_left);
    deleter(calibration_zr_right);
    deleter(curr_on_k);
    deleter(curr_zr);

    LOGGER("<< Raw data reader was closed at MJD: %f, star time: %f. Total time: reading - %f, calibrating - %f, copying - %f", get_MJD_current(), getCurrStarTimeSeconds(), time_reading / (float) CLOCKS_PER_SEC, time_calibrating / (float) CLOCKS_PER_SEC, time_copying / (float) CLOCKS_PER_SEC);
}


DataReader::~DataReader() {
    close();
}

bool DataReader::set_MJD_next(double next_MJD) {
    MJD_next_file = next_MJD;

    double MJD_duration = (next_MJD - get_MJD_begin()) * 24 * 60 * 60;
    ideal_points = round(MJD_duration / dataHeader.tresolution);
    if ((ideal_points - dataHeader.npoints) * dataHeader.tresolution > timeChunk_duration_sun){
        ideal_points = dataHeader.npoints;
        LOGGER(">> ERROR. while setting next MJD: gap > timeChunk_duration_sun (curr MDJ: %f, next_MJD: %f)", get_MJD_begin(), next_MJD);
        //throw logic_error("ERROR. while setting next MJD: gap > timeChunk_duration_sun");
        return true;
    }
    LOGGER("<< Current reader (MJD: %f) was set a next reader (MJD: %f). Points available in this file %d, ideal points is %f", get_MJD_begin(), MJD_next_file, dataHeader.npoints, ideal_points);
    return false;
}

double DataReader::getCurrStarTimeSeconds(){
    return file_starTime_start_seconds + count_read_points * to_starTime(dataHeader.tresolution);
}
double DataReader::getCurrStarTimeSecondsAligned(){
    double curr = file_starTime_start_seconds + count_read_points * to_starTime(dataHeader.tresolution);
    curr = curr + to_starTime(dataHeader.tresolution);
    curr = (int)(curr / timeChunk_duration_star);
    curr = curr * timeChunk_duration_star;

    return curr;
}

void DataReader::AlignByStarTimeChunk(){
    int points_to_skip_int = getCountPointsToNextAlignment(false);
    readNextPoints(nullptr, points_to_skip_int);
    LOGGER("<< Skipped %d points to align to star time (current star time: %f)", points_to_skip_int, getCurrStarTimeSeconds());
}

void DataReader::setCalibrationData(CalibrationDataStorage *calibrationData){
    calibration = calibrationData;
    LOGGER("<< Calibration data storage was attached to raw data reader");
    updateCalibrationData();
}

int DataReader::getCountPointsToNextAlignment(bool need_to_ceil){
    double star_seconds_to_read, base_time;
    base_time = getCurrStarTimeSeconds();

    star_seconds_to_read = timeChunk_duration_star - (base_time - int(base_time / timeChunk_duration_star) * timeChunk_duration_star);
    if (need_to_ceil && star_seconds_to_read < to_starTime(dataHeader.tresolution))
        star_seconds_to_read += timeChunk_duration_star;

    double sun_seconds_to_read = to_SunTime(star_seconds_to_read);
    double points_to_read = sun_seconds_to_read / dataHeader.tresolution;
    int points_to_read_int = round(points_to_read);

    return points_to_read_int;
}

int DataReader::readNextPoints(float *point) {
    int points_to_read_int = getCountPointsToNextAlignment(true);
    return readNextPointsInternal(point, points_to_read_int, 0, points_to_read_int);
}

int DataReader::readNextPoints(float *point, int count, int offset) {
    return readNextPointsInternal(point, count, offset, count);
}

int DataReader::readRemainder(float *point, int *remainder, int *from_this_file){
    int points_to_read_int = getCountPointsToNextAlignment(true);
    *from_this_file = int(ideal_points - count_read_points);
    *remainder = points_to_read_int - *from_this_file;

    int count = readNextPointsInternal(point, *from_this_file, 0, *from_this_file);
    LOGGER("<< Read remainder of the file (%d from this file (where %d interpolated), %d from the next file)", *from_this_file, *from_this_file - count, *remainder);
    close();
    return count;
}

int DataReader::readNextPointsInternal(float *point, int full_count, int offset, int local_count){
    if (!is_header_parsed) // may be disabled for perf purposes
        readHeader();
    if (!reading_started)
        prepareReading();

    int out_count = 0;

    if (local_count <= 0)
        return 0;

    if (count_read_points + local_count > ideal_points)
        throw logic_error("count of points is exceed");

    if (count_read_points + local_count > dataHeader.npoints){
        int saved = dataHeader.npoints - count_read_points;
        if (saved < 0)
            saved = 0;
        out_count += readNextPointsInternal(point, saved, offset, saved);
        points_before_switch_calibration -= local_count - saved;
        count_read_points += local_count - saved;
    }
    else if (points_before_switch_calibration < local_count){
        int saved = points_before_switch_calibration;
        out_count += readNextPointsInternal(point, full_count, offset, points_before_switch_calibration);
        updateCalibrationData();
        out_count += readNextPointsInternal(point, full_count, offset + saved, local_count - saved);
    }
    else if (buffer_pointer + local_count * size_per_point > BUFFER_SIZE){
        int points_available = (BUFFER_SIZE - buffer_pointer) / (size_per_point);
        if (points_available > 0)
            out_count += readNextPointsInternal(point, full_count, offset, points_available);

        if (reading_thread != nullptr) {
            swap_ready = true;
            while (swap_ready)
                this_thread::yield();
        }
//        else {
//            int start = clock();
//            in.read(buffer, BUFFER_SIZE);
//            buffer_pointer = 0;
//            time_reading += clock() - start;
//        }

        out_count += readNextPointsInternal(point, full_count, offset + points_available, local_count - points_available);
    } else {
        int start = clock();
        calibrateArrayPoints((float *) &buffer[buffer_pointer], local_count);
        time_calibrating += clock() - start;


        start = clock();
        if (point != nullptr) {
            auto *buff = (float *) &buffer[buffer_pointer];
            auto *point_ = &point[offset];
            for (int i = 0; i < floats_per_point; ++i)
                for (int j = 0; j < local_count; ++j)
                    point_[i * full_count + j] = buff[j * floats_per_point + i];
        }
        buffer_pointer += size_per_point * local_count;

        points_before_switch_calibration -= local_count;
        count_read_points += local_count;
        time_copying += clock() - start;
        out_count += local_count;
    }
    return out_count;
}


void DataReader::readHeader(){
    if (!is_header_parsed){
        LOGGER(">> Reading raw data header");
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

        //LOGGER(">> Reading thread: swap reading and processing buffers.");

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
    base = new double[getPointSize()];
    memcpy(base, from, sizeof(double) * getPointSize());
}

void DataReader::updateCalibrationData(){
    double currentPointMJD = get_MJD_current();
    CalibrationData * left = calibration->getCalibrationData_left_by_time(currentPointMJD);
    CalibrationData * right = calibration->getCalibrationData_right_by_time(currentPointMJD);

    LOGGER(">> Raw data reader: switching actual calibration file (current MJD: %f, left MJD: %f, right MJD: %f)", currentPointMJD, left->get_MJD(), right->get_MJD());

    points_before_switch_calibration = ((right->get_MJD() - currentPointMJD) * 24 * 60 * 60 + dataHeader.tresolution - 0.000000001) / dataHeader.tresolution;

    realloc(calibration_on_k_left, left->get_one_kelvin());
    realloc(calibration_on_k_right, right->get_one_kelvin());
    realloc(calibration_zr_left, left->get_zero_level());
    realloc(calibration_zr_right, right->get_zero_level());

    calibration_file_duration_in_points = ((right->get_MJD() - left->get_MJD()) * 24 * 60 * 60 + dataHeader.tresolution - 0.000000001) / dataHeader.tresolution;

    LOGGER(">> Switched actual calibration file (points before next switching: %d)", points_before_switch_calibration);
}


void DataReader::calibrateArrayPoints(float *point, int count) {
    static const int calibrationChunkSize = round(timeChunk_duration_sun / 10 / dataHeader.tresolution);

//    LOGGER(">> Calibration of %d points was started. Calibration points step is %d points", count, calibrationChunkSize);

    int curr_calibrated = 0, arr_size = getPointSize();
    double coef;
    for (int t = 0; t < count / calibrationChunkSize; ++t){
        coef = double(points_before_switch_calibration - curr_calibrated) / calibration_file_duration_in_points;

        for (int i = 0; i < arr_size; ++i) {
            curr_on_k[i] = coef * calibration_on_k_left[i] + (1 - coef) * calibration_on_k_right[i];
            curr_zr[i] = coef * calibration_zr_left[i] + (1 - coef) * calibration_zr_right[i];
        }

        calibrateArrayPointsDetailed(&point[t * calibrationChunkSize * floats_per_point], calibrationChunkSize);
        curr_calibrated += calibrationChunkSize;
    }


    coef = double(points_before_switch_calibration - curr_calibrated) / calibration_file_duration_in_points;

    for (int i = 0; i < arr_size; ++i) {
        curr_on_k[i] = coef * calibration_on_k_left[i] + (1 - coef) * calibration_on_k_right[i];
        curr_zr[i] = coef * calibration_zr_left[i] + (1 - coef) * calibration_zr_right[i];
    }

    if (count % calibrationChunkSize != 0)
        calibrateArrayPointsDetailed(&point[(count - count % calibrationChunkSize) * floats_per_point], count % calibrationChunkSize);

//    LOGGER("<< Calibration of %d points was finished", count);
}
