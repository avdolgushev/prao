//
// Created by work on 10.03.2019.
//

#include "MetricsContainer.h"


void MetricsContainer::write_header(storageEntry * entry, vector<metrics *> &found_metrics) {
    string file_path = entry->filesListItem->filepath + ".processed";

    ofstream out(file_path);

    out << "numpar\t" << 4 << endl;
    out << "npoints\t" << found_metrics.size() << endl;
    out << "MJD_begin\t" << setprecision(16) << entry->filesListItem->time_JD << endl;
    out << "nbands\t" << entry->filesListItem->nbands << endl;

    out.close();
}


float *MetricsContainer::prepare_buffer(storageEntry * entry, vector<metrics *> &found_metrics, int *out_array_size) {
    int point_size = entry->filesListItem->getDataReader()->getPointSize();
    int storage_size = found_metrics.size();
    *out_array_size = metrics::metric_count * point_size * storage_size;
    auto buffer = new float[*out_array_size];

    metrics ** data = found_metrics.data();

    for (int k = 0; k < metrics::metric_count; ++k){ // looping metrics
        float * buff_curr = &buffer[k * point_size * storage_size];
        for (int i = 0; i < storage_size; ++i){
            for (int j = 0; j < point_size; ++j){
                buff_curr[i + j * storage_size] = ((float*)(&data[i][j]))[k];
            }
        }
    }

    return buffer;
}


MetricsContainer::~MetricsContainer() {
    for (auto it = storage.begin(); it != storage.end(); ++it)
        for (auto it2 = it->storage.begin(); it2 != it->storage.end(); ++it2)
            delete[] it2->second;
}

storageEntry * MetricsContainer::addNewFilesListItem(FilesListItem *filesListItem){
    if (filesListItem == nullptr)
        throw logic_error("MetricsContainer::addNewFilesListItem filesListItem == nullptr");
    storageEntry entry;
    entry.filesListItem = filesListItem;
    storage.push_back(entry);

    return &storage[storage.size() - 1];
}


void MetricsContainer::flush() {

    storageEntry * found = nullptr;
    vector<metrics *> found_metrics;
    double time_last_found_metric;

    vector<pair<vector<pair<double, metrics *> >*, vector<pair<double, metrics *> >::iterator> > iterators_to_erase;

    for (auto it = storage.begin(); it != storage.end(); ++it) {
        storageEntry &curr_storageEntry = *it;

        double starSeconds = curr_storageEntry.filesListItem->getDataReader()->get_starSeconds_timeChunk_dur();

        if (curr_storageEntry.storage.empty())
            continue;

        if (found != nullptr) // 5. end found in second vector
            iterators_to_erase.emplace_back(make_pair(&curr_storageEntry.storage, curr_storageEntry.storage.begin()));

        for (auto it2 = curr_storageEntry.storage.begin(); it2 != curr_storageEntry.storage.end();) {
            double time = it2->first;

            double tmp;

            if (found != nullptr && modf(time / 3600, &tmp) > EPS) { // 2. adding metrics
                double diff = it2->first - time_last_found_metric;
                if (diff < 0)
                    diff += 86400;
                if (abs(diff - starSeconds) > EPS)
                    throw logic_error("a gap is more than starSeconds from config");

                found_metrics.push_back(it2->second);
                time_last_found_metric = it2->first;
            }
            else if (found != nullptr && modf(time / 3600, &tmp) < EPS) { // 3. found end
                saveFound(found, found_metrics);
                found_metrics.clear();
                found = nullptr;

                iterators_to_erase.emplace_back(make_pair(&curr_storageEntry.storage, it2));

                while(!iterators_to_erase.empty()){ // removing from iterators
                    auto start = iterators_to_erase[0].second;
                    auto end = iterators_to_erase[1].second;
                    iterators_to_erase[0].first->erase(start, end);

                    iterators_to_erase.erase(iterators_to_erase.begin(), iterators_to_erase.begin() + 2);
                }
                it2 = curr_storageEntry.storage.begin();
                continue;
            }
            if (found == nullptr && modf(time / 3600, &tmp) < EPS) { // 1. found start
                found = it.base();
                time_last_found_metric = it2->first;
                found_metrics.push_back(it2->second);
                iterators_to_erase.emplace_back(make_pair(&curr_storageEntry.storage, it2));
            }

            ++it2;
        }

        if (found != nullptr) // 4. end not found but curr vector is exceed
            iterators_to_erase.emplace_back(make_pair(&curr_storageEntry.storage, curr_storageEntry.storage.end()));
    }
}


void MetricsContainer::saveFound(storageEntry * entry, vector<metrics *> &found_metrics) {
    string path = entry->filesListItem->filepath + ".processed";
    write_header(entry, found_metrics);

    int buffer_size;
    float * buffer_to_write = prepare_buffer(entry, found_metrics, &buffer_size);

    FILE *f = fopen(path.c_str(), "ab");
    fwrite(buffer_to_write, 4, buffer_size, f);

    fclose(f);
    delete[] buffer_to_write;
}
