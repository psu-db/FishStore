/// This example is used to demonstrate a constant
///
/// Created by Max Norfolk on 10/17/2025

#include "experiment_setup.h"
#include "device/null_disk.h"

#include "tsl/hopscotch_map.h"

using ndisk_t = fishstore::device::NullDisk;


struct Setup {
    constexpr Setup(bool adaptive, bool reg)
        : adaptive(adaptive),
          reg(reg) {
    }

    bool adaptive;
    bool reg;

    constexpr bool operator==(const Setup &rhs) const {
        return adaptive == rhs.adaptive
               && reg == rhs.reg;
    }


    void print(int i) {
        if (reg) {
            if (adaptive)
                printf("Registered, Adaptive Batch[%lu]\n\n", i);
            else
                printf("Registered, Static Batch[%lu]\n\n", i);
        } else {
            printf("Deregistered, Batch[%lu]\n\n", i);
        }
    }
};

constexpr Setup extensive{false, true};


void doRegisters(const int hour, Setup setup, DataLoader<ndisk_t, adapter_t> &data) {
    if (hour == 0) {
        data.Register("(Str) created_at");
        data.Register("(Str) public");
        data.Register("(Str) type");
        data.Register("(Str) payload.forkee.owner.url");
        //
        // data.Register("(Str) payload.ref");
    }
    if (!setup.reg)
        return;


    const bool apply_all = !setup.adaptive && hour == 0;

    // only register at hour 0
    if (!setup.adaptive && hour != 0)
        return;

    const auto check_hour = [&](int arg_hr) { return apply_all || arg_hr == hour; };

    const auto dereg = [&](const std::string &reg) {
        if (setup.adaptive) {
            data.Deregister(reg);
        }
    };
    if (check_hour(0)) {
        data.Register("(Str) payload.pull_request.head.repo.language");
        data.Register("(Str) payload.pull_request.base.type");
    }
    if (check_hour(5)) {
        dereg("(Str) payload.pull_request.head.repo.language");
        dereg("(Str) payload.pull_request.base.type");
    }
    if (check_hour(7)) {
        if (!(setup == extensive))
            data.Register("(Str) payload.pull_request.head.repo.language");
    }
    if (check_hour(8)) {
        if (setup == extensive)
            data.Register("(Int) repo.id");
        else
            data.Register("(Int) repo.id == 678894831");

        dereg("(Str) payload.pull_request.head.repo.language");
    }
    if (check_hour(11)) {
        dereg("(Int) repo.id == 678894831");
    }
    if (check_hour(12)) {
        if (!(setup == extensive))
            data.Register("(Int)repo.id == 678894831");
    }
    if (check_hour(14)) {
        data.Register("(Str) payload.pull_request.base.url");
    }
    if (check_hour(16)) {
        dereg("(Int)repo.id == 678894831");
        if (setup == extensive)
            data.Register("(Int) actor.id");
        else
            data.Register("(Int) actor.id == 49699333");
    }
    if (check_hour(22)) {
        dereg("(Str) payload.pull_request.base.url");
    }

    if (check_hour(23)) {
        dereg("(Int) actor.id == 49699333");
    }
}


int main(int argc, char **argv) {
    // const auto file = "/data/dataset/mnorfolk/Github/combined_data.json";
    // const auto file = "/scratch/mnorfolk/Data/Github/combined_data.json";

    std::vector<std::vector<std::string> > all_batches;
    std::string base = "/scratch/mnorfolk/Data/Github2025/";

    // Day 1: 2025-01-01-07.json to 2025-01-01-23.json
    for (int hour = 7; hour < 24; ++hour) {
        std::ostringstream oss;
        oss << base << "2025-01-01-" << std::setw(2) << std::setfill('0') << hour << ".json";
        std::string filename = oss.str();

        std::cout << "Processing file: " << filename << std::endl;

        all_batches.emplace_back(DataLoader<ndisk_t, adapter_t>::GenerateBatches(filename));
        printf("Batch[%d] is %lu batches\n", hour, all_batches.back().size());
    }

    // Day 2: 2025-01-02-00.json to 2025-01-02-06.json
    for (int hour = 0; hour <= 6; ++hour) {
        std::ostringstream oss;
        oss << base << "2025-01-02-" << std::setw(1) << std::setfill('0') << hour << ".json";
        std::string filename = oss.str();

        std::cout << "Processing file: " << filename << std::endl;

        all_batches.emplace_back(DataLoader<ndisk_t, adapter_t>::GenerateBatches(filename));
        printf("Batch[%d] is %lu batches\n", hour, all_batches.back().size());
    }


    using namespace fishstore::plan;
    using StringRef = fishstore::adapter::StringRef;


    /// Maps Page => per page stats

    std::vector<std::vector<double> > ingest_s;
    std::vector<std::vector<double> > query_s;


    std::vector<Setup> setups{Setup{false, false}, Setup{true, true}, extensive};


    for (auto &&setup: setups) {
        constexpr int TRIALS = 9;
        std::vector<std::vector<double> > runs(TRIALS);

        for (int r = 0; r < runs.size(); ++r) {
            printf("=== Ingestion Run %d ===\n", r + 1);

            DataLoader<ndisk_t, adapter_t> data{8192, 12LL << 30};
            data.fishstore->StartSession();


            runs[r].reserve(all_batches.size());
            for (size_t i = 0; i < all_batches.size(); ++i) {
                doRegisters(i, setup, data);
                setup.print(i);
                data.PrintRegistrations();

                size_t nanos = median("BatchInsert", [&]() {
                    data.BatchInsert(all_batches[i]);
                }, 1);

                runs[r].push_back(nanos * 1e-9f);
                printf("Run %d Batch %lu: %.3fs\n", r + 1, i, nanos * 1e-9f);
            }
        }

        // median
        ingest_s.emplace_back();
        for (size_t i = 0; i < all_batches.size(); ++i) {
            std::vector<double> t;
            t.reserve(runs.size());
            for (int r = 0; r < runs.size(); ++r) {
                t.push_back(runs[r][i]);
            }
            std::sort(t.begin(), t.end());
            double med = t[runs.size() / 2]; // works for both even and odd num_runs
            ingest_s.back().push_back(med);
            printf("Batch %lu Median: %.3fs\n", i, med);
        }
    }

    for (auto &&setup: setups) {
        query_s.emplace_back();

        DataLoader<ndisk_t, adapter_t> data{8192, 12LL << 30};
        data.fishstore->StartSession();

        for (size_t i = 0; i < all_batches.size(); ++i) {
            // for (size_t i = all_batches.size() - 1; i > 0; --i) {
            const auto &batch = all_batches[i];

            doRegisters(i, setup, data);
            if (i == 0) {
                data.BatchInsert(batch);
                data.BatchInsert(batch);
                data.BatchInsert(batch);
            }

            setup.print(i);
            data.PrintRegistrations();
            data.BatchInsert(batch);


            const auto nanos_q = median("[Query]", [&]() {
                if (i < 8) {
                    data.Query({"(Str) type", "(Int) repo.id", "(Str) repo.url"})
                            ->FromEnd(1ULL << 30)
                            ->FilterString({"(Str) type == \"PullRequestEvent\""}, data.GetPsfId("(Str) type"),
                                           "PullRequestEvent")
                            ->GroupBy({"(Int) repo.id", "(Str) repo.url"})
                            ->Aggregate({Count("")})
                            ->Rename({"id", "url", "count"})
                            ->SortBy({"(Int) count * -1"})
                            ->Limit(3)
                            ->Print();
                } else if (i < 16) {
                    auto tmp = data.Query({
                        "(Int) repo.id", "(Str) repo.url", "(Int) actor.id", "(Str) actor.display_login"
                    });
                    tmp->FromEnd(1ULL << 30);
                    auto tmp2 = setup == extensive
                                    ? tmp->Filter({"(Int) repo.id == 678894831"},
                                                  true,
                                                  data.GetPsfId("(Int) repo.id"),
                                                  678894831)
                                    : tmp->FilterUsePsf({"(Int) repo.id == 678894831", "TRUE"},
                                                        {
                                                            FilterIntInfo{"(Int)repo.id == 678894831", 1, 1}
                                                        });

                    tmp2
                            ->GroupBy({"(Int) actor.id", "(Str) actor.display_login"})
                            ->Aggregate({Count("")})
                            ->Rename({"id", "name", "count"})
                            ->SortBy({"(Int) count * -1"})
                            ->Limit(3)
                            ->Print();
                } else {
                    auto tmp = data.Query({
                        "(Int) repo.id", "(Str) repo.url", "(Str) type", "(Int) actor.id",
                        "(Str) actor.display_login"
                    });
                    tmp->FromEnd(1ULL << 30);

                    auto tmp2 = setup == extensive
                                    ? tmp->Filter({"(Int) actor.id == 49699333"},
                                                  true,
                                                  data.GetPsfId("(Int) actor.id"),
                                                  49699333)
                                    : tmp->Filter({"(Int) actor.id == 49699333"});

                    tmp2
                            ->GroupBy({
                                "(Int) repo.id", "(Str) repo.url", "(Int) actor.id", "(Str) actor.display_login"
                            })
                            ->Aggregate({Count("")})
                            ->Rename({"id", "url", "actor", "login", "count"})
                            ->SortBy({"(Int) count * -1"})
                            ->Limit(3)
                            ->Print();
                }
            }, 5);

            query_s.back().emplace_back(nanos_q * 1e-9f);
            printf("End Batch %lu\n", i);
        }
    }

    printf("Ingest\t\t\tLatency\n");
    for (int i = 0; i < query_s[0].size(); ++i) {
        printf("%d\t%lf\t%lf\t%lf\t%lf\t%lf\t%lf\n", i,
               ingest_s[0][i], ingest_s[1][i], ingest_s[2][i],
               query_s[0][i], query_s[1][i], query_s[2][i]);
    }

    return 0;
}
