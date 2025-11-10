/// This is a repeat of section 2 experiment, but this time, it does not care about
/// ingestion throughput, and instead just builds an index 1/2 way through
///
///
/// Created by max 7/14

#include <iostream>
#include <unistd.h>

#define ENABLE_TIMER

#include <psu-util/timer.h>

#include "adapters/simdjson_adapter.h"
#include "device/null_disk.h"
#include "plan/plan.h"

using namespace fishstore::plan::data_frame;
using namespace fishstore::plan::data_frame::aggregate;


// typedef fishstore::environment::QueueIoHandler handler_t;
// typedef fishstore::device::FileSystemDisk<handler_t, 1073741824L> disk_t;
using adapter_t = fishstore::adapter::SIMDJsonAdapter;
using disk_t = fishstore::device::NullDisk;

constexpr size_t TRIALS = 5;

// returns number of seconds
double time(const std::string &prefix, const std::function<void()> &func) {
    TIMER_INIT();
    TIMER_START();
    func();
    TIMER_STOP();

    printf("[%s]: %ld nanoseconds (%.2f seconds)\n\n", prefix.c_str(), TIMER_RESULT(), TIMER_RESULT() * 1e-9);
    return TIMER_RESULT() / 1e9;
}

inline double median(const std::string &prefix, const std::function<void()> &f, int trials = 3) {
    std::vector<double> timed_res;
    for (int i = 0; i < trials; ++i) {
        timed_res.emplace_back(time(prefix + std::string{" [Trial "} + std::to_string(i) + "]:", f));
    }
    std::sort(timed_res.begin(), timed_res.end());
    return timed_res.at(timed_res.size() / 2);
}

void plan(DataLoader<disk_t, adapter_t> &data, const std::vector<fishstore::plan::op::ScanInfo> &plan) {
    data.Query({"(Int) repo.id", "(Str) repo.url", "(Int) actor.id", "(Str) actor.display_login"})
            ->FilterPlan({"(Int) repo.id == 678894831"}, plan)
            ->GroupBy({"(Int) actor.id", "(Str) actor.display_login"})
            ->Aggregate({Count("")})
            ->Rename({"id", "name", "count"})
            ->SortBy({"(Int) count * -1"})
            ->Limit(3)
            ->Print();
}

void stitch(DataLoader<disk_t, adapter_t> &data, size_t from_end) {
    data.Query({"(Int) repo.id", "(Str) repo.url", "(Int) actor.id", "(Str) actor.display_login"})
            ->FromEnd(from_end)
            ->Filter({"(Int) repo.id == 678894831 "}, true, data.GetPsfId("(Int) repo.id"), 678894831)
            ->GroupBy({"(Int) actor.id", "(Str) actor.display_login"})
            ->Aggregate({Count("")})
            ->Rename({"id", "name", "count"})
            ->SortBy({"(Int) count * -1"})
            ->Limit(3)
            ->Print();
}


int main() {
    // const auto file = "/data/dataset/mnorfolk/Github/combined_data.json";
    // const auto file = "/scratch/mnorfolk/Data/Github/2024-01-01-0.json";
    const auto file = "/scratch/mnorfolk/Data/Github2025/combined_jan_first.json";
    const auto batches = DataLoader<disk_t, adapter_t>::GenerateBatches(file);

    size_t count = 0;
    for (auto &&batch: batches)
        count += batch.size();
    printf("[Loaded File: %ld bytes!]\n", count);


    struct Result {
        Result() = default;

        double raw;
        double traditional;
        double stitching;
    };

    DataLoader<disk_t, adapter_t> data;
    data.fishstore->StartSession();

    const auto midpoint = batches.begin() + batches.size() / 2;
    data.BatchInsert(batches.begin(), midpoint);
    data.Register("(Int) repo.id");

    const auto midpoint_addr = data.getTailAddress();

    data.BatchInsert(midpoint, batches.end());


    std::vector<Result> results;

    constexpr int NUM_POINTS = 16;
    uint64_t data_per = (data.getTailAddress() - data.getHeadAddress()) / NUM_POINTS;
    for (int i = 1; i <= NUM_POINTS; ++i) {
        auto &&res = results.emplace_back();

        res.raw = median("Raw", [&] {
            plan(data, {
                     fishstore::plan::op::ScanInfo{
                         fishstore::plan::PsfInfo{
                             fishstore::plan::FS_ID, data.getTailAddress() - i * data_per,
                             fishstore::core::Address::kMaxAddress
                         },
                         0
                     }
                 });
        });

        res.traditional = median("Traditional", [&] {
            const uint64_t scan_start = data.getTailAddress() - i * data_per;

            // if scan starts before midpoint, need to use FS, else use index
            auto psf_id = scan_start < midpoint_addr ? fishstore::plan::FS_ID : fishstore::plan::PsfId::Inline(0);

            plan(data, {
                     fishstore::plan::op::ScanInfo{
                         fishstore::plan::PsfInfo{
                             psf_id, scan_start,
                             fishstore::core::Address::kMaxAddress
                         },
                         0
                     }
                 });
        });


        res.stitching = median("Stitching", [&] {
            stitch(data, data_per * i);
        });
    }

    for (int i = 0; i < results.size(); ++i) {
        printf("%d %lf %lf %lf\n", i, results[i].raw, results[i].traditional, results[i].stitching);
    }
}
