//
// Created by work on 10.02.2019.
//

#include "FilesListItem.h"

DataReader* FilesListItem::getDataReader(double starSeconds_timeChunk_dur){
    if (reader == nullptr) {
        reader = new DataReader(filepath, starSeconds_timeChunk_dur, star_time_start);
    }
    return reader;
}


FilesListItem::~FilesListItem(){
    if (reader != nullptr) {
        delete reader;
        reader = nullptr;
    }
}

bool FilesListItem::good() {
    return nbands != 0;
}


istream &operator>>(istream & in, FilesListItem& dt){
    LOGGER(">> Read file description from files list");

    string tmp;
    getline(in, tmp, ';');
    sscanf(tmp.c_str(), "%d-%d-%d %d:%d:%d", &dt.time_UTC.tm_year, &dt.time_UTC.tm_mon, &dt.time_UTC.tm_mday, &dt.time_UTC.tm_hour, &dt.time_UTC.tm_min, &dt.time_UTC.tm_sec);
    tm_toUTC(dt.time_UTC);

    getline(in, dt.filename, ';');
    getline(in, dt.filepath, ';');
    dt.filepath = regex_replace(dt.filepath, regex("(\\\\){2}"), "/");

    getline(in, tmp, ';');
    dt.nbands = atoi(tmp.c_str());

    getline(in, tmp, ';');
    dt.npoints = atoi(tmp.c_str());

    getline(in, tmp, ';');
    dt.tresolution = atof(tmp.c_str()) / 1000;

    getline(in, tmp, ';');
    dt.star_time_start = atof(tmp.c_str());

    getline(in, tmp, ';');
    dt.star_time_end = atof(tmp.c_str());

    getline(in, tmp, '\n');
    dt.time_MJD = atof(tmp.c_str());

    dt.reader = nullptr;

    LOGGER("<< File description from files list was read (file: %s\tMJD: %f)", dt.filepath.c_str(), dt.time_MJD);
    return in;
}
