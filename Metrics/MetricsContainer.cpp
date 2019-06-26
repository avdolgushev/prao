//
// Created by work on 10.03.2019.
//

#include "MetricsContainer.h"


storageEntry * MetricsContainer::addNewFilesListItem(FilesListItem *filesListItem){
    if (filesListItem == nullptr)
        throw logic_error("MetricsContainer::addNewFilesListItem filesListItem == nullptr");
    storageEntry entry;
    entry.filesListItem = filesListItem;
    storage.push_back(entry);

    return &storage[storage.size() - 1];
}



void MetricsContainer::write_header(string file_path, storageEntry * entry, vector<metrics *> &found_metrics, metrics_with_time &found_start) {
    ofstream out(file_path, ios::binary);

    if (!out.good()){
        throw logic_error("error during writing header of output file: " + file_path);
    }

    DataHeader source_header = entry->filesListItem->getDataReader()->getDataHeader();

    Document doc;
    auto& allocator = doc.GetAllocator();
    doc.SetObject();

    Value source_file(kObjectType);
    source_file.AddMember("fcentral", source_header.fcentral, allocator);
    source_file.AddMember("wb_total", source_header.wb_total, allocator);

    {
        tm &dt = source_header.begin_datetime;
        tm_AddDefault(source_header.begin_datetime);
        string t1 = to_string(dt.tm_mday) + "." + to_string(dt.tm_mon) + "." + to_string(dt.tm_year) + " " +
                    to_string(dt.tm_hour) + ":" + to_string(dt.tm_min) + ":" + to_string(dt.tm_sec);
        tm_SubDefault(source_header.begin_datetime);
        Value t2;
        t2.SetString(t1.c_str(), t1.size(), allocator);
        source_file.AddMember("datetime", t2, allocator);
    }

    Value modulus(kArrayType);
    for (auto i: source_header.modulus)
        modulus.PushBack(i, allocator);

    source_file.AddMember("modulus", modulus, allocator);
    source_file.AddMember("tresolution", source_header.tresolution, allocator);
    source_file.AddMember("npoints", source_header.npoints, allocator);
    source_file.AddMember("nbands", source_header.nbands, allocator);

    Value wbands(kArrayType);
    for (auto i: source_header.wbands)
        wbands.PushBack(i, allocator);


    source_file.AddMember("wbands", wbands, allocator);

    Value fbands(kArrayType);
    for (auto i: source_header.fbands)
        fbands.PushBack(i, allocator);

    source_file.AddMember("fbands", fbands, allocator);

    Value filename;
    filename.SetString(entry->filesListItem->filename.c_str(), entry->filesListItem->filename.size(), allocator);
    source_file.AddMember("source_file_start", filename, allocator);
    source_file.AddMember("MJD_begin", entry->filesListItem->time_MJD, allocator);
    source_file.AddMember("star_begin", entry->filesListItem->star_time_start, allocator);


    doc.AddMember("source_file", source_file, allocator);

    doc.AddMember("star_start", found_start.starTime_, allocator);
    doc.AddMember("MJD_start", found_start.MJD_time_, allocator);
    doc.AddMember("npoints_zipped", found_metrics.size(), allocator);
    doc.AddMember("zipped_point_tresolution", Configuration.starSecondsZip, allocator);
    doc.AddMember("fileDuration_in_star_seconds", Configuration.starSecondsWrite, allocator);
    doc.AddMember("leftPercentile", Configuration.leftPercentile, allocator);
    doc.AddMember("rightPercentile", Configuration.rightPercentile, allocator);

    Value metrics(kArrayType);
    vector<string> metrics_strings = { "min", "max", "max_ind", "average", "median", "variance", "variance_bounded" };
    for (auto &i: metrics_strings) {
        Value t;
        t.SetString(i.c_str(), i.size(), allocator);
        metrics.PushBack(t, allocator);
    }
    doc.AddMember("metrics", metrics, allocator);

    {
        Value reserved;
        string reserved_s = "1111111111111111111111111111111111111111";

        reserved.SetString(reserved_s.c_str(), reserved_s.size(), allocator);
        doc.AddMember("reserved1", reserved, allocator);
        reserved.SetString(reserved_s.c_str(), reserved_s.size(), allocator);
        doc.AddMember("reserved2", reserved, allocator);
        reserved.SetString(reserved_s.c_str(), reserved_s.size(), allocator);
        doc.AddMember("reserved3", reserved, allocator);
        reserved.SetString(reserved_s.c_str(), reserved_s.size(), allocator);
        doc.AddMember("reserved4", reserved, allocator);
        reserved.SetString(reserved_s.c_str(), reserved_s.size(), allocator);
        doc.AddMember("reserved5", reserved, allocator);
    }

    StringBuffer buffer;
    PrettyWriter<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    const std::string& str = buffer.GetString();
    //std::cout << str << std::endl;

    int json_size = str.size();
    out.write(reinterpret_cast<const char *>(&json_size), sizeof(json_size));
    out << str;

    out.close();
}


float *MetricsContainer::prepare_buffer_time_ray_band_metric(storageEntry *entry, vector<metrics *> &found_metrics, int *out_array_size, metrics_with_time &found_start) {
    int point_size = entry->filesListItem->getDataReader()->getPointSize();
    int points_count = found_metrics.size();
    *out_array_size = metrics::metric_count * point_size * points_count;
    auto buffer = new float[*out_array_size];

    metrics ** data = found_metrics.data();

    for (int time = 0; time < points_count; ++time)
        memcpy(&buffer[time * point_size * metrics::metric_count], data[time], point_size * metrics::metric_count * 4);

    return buffer;
}


float *MetricsContainer::prepare_metric_unk(storageEntry *entry, vector<metrics *> &found_metrics, int *out_array_size,
                                            metrics_with_time &found_start) {
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
    for (auto it = storage.begin(); it != storage.end(); ++it) {
        for (auto it2 = it->storage.begin(); it2 != it->storage.end(); ++it2)
            delete[] it2->metrics_;
        delete it->filesListItem;
    }
}



void MetricsContainer::flush() {

    storageEntry * found = nullptr;

    metrics_with_time found_start;

    vector<metrics *> found_metrics;
    vector<int> found_metrics_counts;
    double time_last_found_metric;

    vector<pair<vector<metrics_with_time>*, vector<metrics_with_time>::iterator> > iterators_to_erase;

    for (auto it = storage.begin(); it != storage.end(); ++it) {
        storageEntry &curr_storageEntry = *it;

        if (curr_storageEntry.storage.empty())
            continue;

        if (found != nullptr) // 5. end found in second vector
            iterators_to_erase.emplace_back(make_pair(&curr_storageEntry.storage, curr_storageEntry.storage.begin()));

        for (auto it2 = curr_storageEntry.storage.begin(); it2 != curr_storageEntry.storage.end();) {
            double starTime = it2->starTime_;

            double tmp;

            if (found != nullptr && modf(starTime / Configuration.starSecondsWrite, &tmp) > EPS) { // 2. adding metrics
                double diff = starTime - time_last_found_metric;
                if (diff < 0)
                    diff += 86400;
                if (abs(diff - Configuration.starSecondsZip) > EPS)
                    throw logic_error("a gap is more than starSecondsZip from config");

                found_metrics.push_back(it2->metrics_);
                found_metrics_counts.push_back(found_metrics_counts[found_metrics_counts.size() - 1] + it2->count_read_points);

                time_last_found_metric = starTime;
            }
            else if (found != nullptr && modf(starTime / Configuration.starSecondsWrite, &tmp) < EPS) { // 3. found end
                int n = curr_storageEntry.filesListItem->getDataReader()->getPointSize();
                for (int i = 1; i < found_metrics.size(); ++i){
                    metrics * curr = found_metrics[i];
                    for (int j = 0; j < n; ++j)
                        curr[j].max_ind += found_metrics_counts[i - 1];
                }

                saveFound(found, found_metrics, found_start);
                found_metrics.clear();
                found = nullptr;

                iterators_to_erase.emplace_back(make_pair(&curr_storageEntry.storage, it2));

                while(!iterators_to_erase.empty()){ // removing from iterators
                    auto start = iterators_to_erase[0].second;
                    auto end = iterators_to_erase[1].second;

                    for (auto curr = start; curr != end; ++curr)
                        delete[] curr->metrics_;

                    iterators_to_erase[0].first->erase(start, end);

                    iterators_to_erase.erase(iterators_to_erase.begin(), iterators_to_erase.begin() + 2);
                }
                it2 = curr_storageEntry.storage.begin();
                continue;
            }
            if (found == nullptr && modf(starTime / Configuration.starSecondsWrite, &tmp) < EPS) { // 1. found start
                found = it.base();
                found_start = *it2;
                time_last_found_metric = starTime;
                found_metrics.push_back(it2->metrics_);
                found_metrics_counts.push_back(it2->count_read_points);
                iterators_to_erase.emplace_back(make_pair(&curr_storageEntry.storage, it2));
            }

            ++it2;
        }

        if (found != nullptr) // 4. end not found but curr vector is exceed
            iterators_to_erase.emplace_back(make_pair(&curr_storageEntry.storage, curr_storageEntry.storage.end()));
    }
}


void MetricsContainer::saveFound(storageEntry * entry, vector<metrics *> &found_metrics, metrics_with_time &found_start) {
    string path = Configuration.outputPath + "\\" + entry->filesListItem->filename + "_" + to_string(found_start.starTime_ / 3600) + ".processed";
    write_header(path, entry, found_metrics, found_start);

    int buffer_size;
    float * buffer_to_write = prepare_buffer_time_ray_band_metric(entry, found_metrics, &buffer_size, found_start);

    FILE *f = fopen(path.c_str(), "ab");
    fwrite(buffer_to_write, 4, buffer_size, f);

    fclose(f);
    delete[] buffer_to_write;

    LOGGER("<< Saved output file to %s. MJD start: %f, star start: %f", path.c_str(), found_start.MJD_time_, found_start.starTime_);
}
