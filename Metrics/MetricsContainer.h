//
// Created by work on 10.03.2019.
//

#ifndef PRAO_COMPRESSER_METRICSCONTAINER_H
#define PRAO_COMPRESSER_METRICSCONTAINER_H

#include <string>
#include <vector>
#include <fstream>
#include <iomanip>

#include "MetricsType.h"
#include "../Reader/DataReader.h"
#include "../Reader/FilesListItem.h"

struct storageEntry{
    FilesListItem * filesListItem = nullptr;
    vector<pair<double, metrics *> > storage = vector<pair<double, metrics *> >();

    void addNewMetrics(double starTime, metrics *metrics_) {
        storage.emplace_back(make_pair(starTime, metrics_));
    }
};

class MetricsContainer {
    vector<storageEntry> storage = vector<storageEntry>();

    float * prepare_buffer(storageEntry * entry, vector<metrics *> &found_metrics, int *out_array_size);
    void write_header(storageEntry * entry, vector<metrics *> &found_metrics);

public:
    ~MetricsContainer();
    storageEntry * addNewFilesListItem(FilesListItem *filesListItem);
    void flush();
    void saveFound(storageEntry * entry, vector<metrics *> &found_metrics);
};


#endif //PRAO_COMPRESSER_METRICSCONTAINER_H
