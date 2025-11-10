// Decides how to order the PSF Scans & Full Scans based upon the PSFs
//
// Created by Max Norfolk on 10/29/23.


#pragma once

#include <utility>
#include <functional>

#include "core/fishstore.h"

#include "plan/filter.h"
#include "plan/operators/abstract_start_op.h"

namespace fishstore::plan::op {
    struct ScanInfo {
        ScanInfo(PsfInfo psf_info, adapter::StringRef s_val) : psf_info(psf_info), s_val(std::move(s_val)) {}

        ScanInfo(PsfInfo psf_info, int32_t i_val) : psf_info(psf_info), i_val(i_val) {}

        PsfInfo psf_info;

        union {
            adapter::StringRef s_val;
            int32_t i_val;
        };

        [[nodiscard]] constexpr bool isInline() const { return psf_info.id.isInline(); }
        [[nodiscard]] constexpr bool isGeneral() const { return psf_info.id.isGeneral(); }
        [[nodiscard]] constexpr uint32_t id() const { return psf_info.id.id; }
        [[nodiscard]] constexpr PsfAddress start() const { return psf_info.start; }
        [[nodiscard]] constexpr PsfAddress end() const { return psf_info.end; }


        [[nodiscard]] constexpr bool covers(const PsfAddress& addr) const { return psf_info.covers(addr); }

        size_t psfLen(const core::psf_len_ht_t& ht) const {
            if (isInline())
                return core::psfLen(ht, id(), i_val);
            assert(s_val.Length() != 0);
            return core::psfLen(ht, id(), s_val);
        }
    };

    std::vector<ScanInfo>
    planScans(const core::psf_len_ht_t& len_ht, const core::rec_count_prefix_t& rec_count, const PsfMap& map,
              PsfAddress start, PsfAddress end, PsfAddress inmem_start_addr, PsfAddress inmem_end_addr,
              const Filter& filter);

    template<typename D, typename A>
    class TableScan : public AbstractStartOp {
    public:
        typedef fishstore::core::FishStore<D, A> store_t;

        // the constructor will plan the scan
        TableScan(store_t& store, const PsfMap& map, Schema schema = {}, Filter filter = {},
                  PsfAddress start = 0,
                  PsfAddress end = fishstore::core::Address::kMaxAddress,
                  bool no_plan = true,
                  std::vector<ScanInfo> scans = {})
            : store(store), schema(std::move(schema)), scan_filter(std::move(filter)), start(start),
              end(end) {
            if (no_plan)
                scan_plan = planScans(store.psf_len_ht, store.records_prefix_sum, map, start,
                                      (end == core::Address::kMaxAddress) ? store.GetTailAddress().control() : end,
                                      store.GetHeadAddress().control(), store.GetTailAddress().control(),
                                      scan_filter);
            else
                scan_plan = std::move(scans);
        }

        void StartScan() override;

        void Initialize(ThreadId id) override;

        void Touch(ThreadId id, record::Record* rec) override;

        void Finalize(ThreadId id) override;

        void FinalizeAll() override;

        [[nodiscard]] const Schema& GetSchema() const override {
            return schema;
        }

        [[nodiscard]] inline const Filter& GetFilter() const { return scan_filter; }

        void LinkParent(AbstractOp* op) override { parent = op; }

        void LinkChild(AbstractOp* op) override {
            assert(op == nullptr && "TableScan must be the lowest level");
        }

        void PrettyPrint(const uint32_t indent) override {
            printIndent(indent);
            printf("[TableScan]::\n");
            for (const auto& plan: scan_plan) {
                printIndent(indent);
                if (plan.psf_info.id == FS_ID)
                    printf("  [FullScan] from [%15lu, %15lu]\n", plan.start(), plan.end());
                else
                    printf("  [PSF: %3d] from [%15lu, %15lu]\n", plan.id(), plan.start(), plan.end());
            }
        }

        void PrettyPrint() override {
            this->PrettyPrint(0);
        }


        PsfAddress start;
        PsfAddress end;
        std::vector<ScanInfo> scan_plan;

    private:
        store_t& store;
        const Schema schema;
        const Filter scan_filter;

        AbstractOp* parent = nullptr;
    };

    using score_t = double;
    constexpr score_t MEM_FS_COST = 1000.793798;
    constexpr score_t DISK_FS_COST = 4030.423504;

    constexpr score_t MEM_SCAN_COST_VERY_SEL_A = 49.849834;
    constexpr score_t MEM_SCAN_COST_VERY_SEL_B = -64.087562;
    constexpr score_t MEM_SCAN_COST_UNSEL_A = 207.423722;
    constexpr score_t MEM_SCAN_COST_UNSEL_B = 3.203503;
    constexpr score_t DISK_SCAN_COST_VERY_SEL_A = 180051.348537;
    constexpr score_t DISK_SCAN_COST_VERY_SEL_B = 5573.145236;
    constexpr score_t DISK_SCAN_COST_UNSEL_A = 5626.435472;
    constexpr score_t DISK_SCAN_COST_UNSEL_B = 3041.368685;
    constexpr score_t MEM_SKIP_COST_VERY_SEL_A = 20.544053;
    constexpr score_t MEM_SKIP_COST_VERY_SEL_B = 0.217128;
    constexpr score_t MEM_SKIP_COST_UNSEL_A = 192.601383;
    constexpr score_t MEM_SKIP_COST_UNSEL_B = 3.608314;
    constexpr score_t DISK_SKIP_COST_VERY_SEL_A = 184930.671383;
    constexpr score_t DISK_SKIP_COST_VERY_SEL_B = 13548.954082;
    constexpr score_t DISK_SKIP_COST_UNSEL_A = 1270.526991;
    constexpr score_t DISK_SKIP_COST_UNSEL_B = 3076.619208;

    inline std::vector<ScanInfo>
    planScans(const core::psf_len_ht_t& len_ht, const core::rec_count_prefix_t& rec_count, const PsfMap& map,
              const PsfAddress start, const PsfAddress end, const PsfAddress inmem_start_addr,
              const PsfAddress inmem_end_addr,
              const Filter& filter) {
        assert(start < end && "Illegal scan");
        printf("\n\n[Starting Planning from %lu to %lu]\n", start, end);
        printf("Memory: [%lu %lu]\n", inmem_start_addr, inmem_end_addr);

        auto psf_set = filter.usedIds();

        // if no psfs to use, then don't bother doing any planning
        if (psf_set.empty()) {
            return {ScanInfo{PsfInfo{FS_ID, start, end}, 0}};
        }


        std::vector<ScanInfo> psfs;
        psfs.emplace_back(PsfInfo{FS_ID, start, end}, 0);


        std::vector<PsfAddress> discrete_times;
        discrete_times.emplace_back(start);
        discrete_times.emplace_back(end);
        if (start < inmem_start_addr && inmem_start_addr < end)
            discrete_times.emplace_back(inmem_start_addr);

        #define bounded_emplace(V) if ((V) >= start && (V) <= end) discrete_times.emplace_back(V)
        for (auto&& clause: filter) {
            if (clause.id == NO_PSF)
                continue;
            auto info = map.at(clause.id);
            info.end = std::min(inmem_end_addr, info.end);

            // extract timestamps
            bounded_emplace(info.start);
            bounded_emplace(info.end);

            // extract scan info
            if (clause.id.isInline())
                psfs.emplace_back(info, clause.i_val);
            else
                psfs.emplace_back(info, clause.s_val);
        }

        #undef bounded_emplace

        // sort the timestamps by decreasing order as we want to go from high address to low address
        std::sort(discrete_times.begin(), discrete_times.end(), std::greater<>());
        discrete_times.erase(std::unique(discrete_times.begin(), discrete_times.end()), discrete_times.end());


        const size_t time_count = discrete_times.size();
        const size_t psf_count = psfs.size();

        struct DpEntry {
            DpEntry() = default;

            DpEntry(const score_t score, const int prev_index, bool continuing_scan)
                : score(score), prev_index(prev_index), continuing_scan(continuing_scan) {}

            score_t score;
            int prev_index: 31;
            bool continuing_scan: 1;

            void print() {
                printf("[C: %s, PrevIndex: %2d, %4.3e]\t", continuing_scan ? "Con" : "New", prev_index, score);
            }
        };

        // stores the score of each plan
        // opt.at(index(i, j)) means the score of using PSF psfs[j] for timestamp[i] to timestamp[i+1]-1
        psudb::DynamicArray<DpEntry> opt((time_count - 1) * psf_count);

        const score_t TINY_INF = 1e38; // impossible number to reach, but won't have overflow


        const auto index = [time_count, psf_count](const int time_index, const int psf_index) constexpr {
            // assert(time_index < time_count- 1);
            const auto ret = psf_count * time_index + psf_index;
            return ret;
        };

        const auto segInMemory = [&](const int ts) {
            return discrete_times[ts + 1] >= inmem_start_addr;
        };


        const auto costFullScan = [&](const int ts) -> score_t {
            const score_t scan_len = core::recordCount(rec_count, discrete_times[ts + 1], discrete_times[ts]);
            const score_t mul = segInMemory(ts) ? op::MEM_FS_COST : op::DISK_FS_COST;
            return scan_len * mul;
        };

        // cost of just the scan component (because we are continuing a scan
        const auto costJustScan = [&](const int ts, const int psf_index) -> score_t {
            const auto psf = psfs[psf_index];
            assert(psf.psf_info.id != FS_ID);

            const score_t chain_len = psf.psf_info.end - psf.psf_info.start;
            assert(chain_len > 0 && "PSF has 0 length!");
            assert(discrete_times.size() > ts + 1);
            const score_t scan_len = discrete_times[ts] - discrete_times[ts + 1];
            assert(scan_len > 0);
            const score_t percent_of_chain_scanned = scan_len / chain_len;
            assert(percent_of_chain_scanned <= 1.0);

            const score_t num_recs = percent_of_chain_scanned * static_cast<score_t>(psf.psfLen(len_ht));
            const score_t total_recs = core::recordCount(rec_count, discrete_times[ts + 1], discrete_times[ts]);
            const score_t selectivity = num_recs / total_recs;


            if (selectivity < 0.05) {
                const score_t x = std::log(selectivity);
                const score_t mul = segInMemory(ts)
                                        ? op::MEM_SCAN_COST_VERY_SEL_A + op::MEM_SCAN_COST_VERY_SEL_B * x
                                        : op::DISK_SCAN_COST_VERY_SEL_A + op::DISK_SCAN_COST_VERY_SEL_B * x;
                return num_recs * mul;
            }

            const score_t x = 1 / selectivity;
            const score_t mul = segInMemory(ts)
                                    ? (op::MEM_SCAN_COST_UNSEL_A + op::MEM_SCAN_COST_UNSEL_B * x)
                                    : (op::DISK_SCAN_COST_UNSEL_A + op::DISK_SCAN_COST_UNSEL_B * x);
            return num_recs * mul;
        };


        const auto costTraversalAndScan = [&](const int ts, const int psf_index) -> score_t {
            const auto psf = psfs[psf_index];
            assert(psf.psf_info.id != FS_ID);


            if (discrete_times[ts] == psf.end()) {
                return costJustScan(ts, psf_index);
            }
            assert(discrete_times.size() > ts + 1);
            const score_t psf_len = psf.psf_info.end - psf.psf_info.start;
            assert(psf_len > 0);

            score_t mem_len = 0;
            score_t disk_len = 0;

            // if crosses memory
            if (psf.psf_info.end >= inmem_start_addr && discrete_times[ts] < inmem_start_addr) {
                mem_len = psf.psf_info.end - static_cast<score_t>(inmem_start_addr);
                disk_len = inmem_start_addr - static_cast<score_t>(discrete_times[ts]);
            } else {
                // scan is only in memory
                if (discrete_times[ts] >= inmem_start_addr) {
                    mem_len = psf.psf_info.end - static_cast<score_t>(discrete_times[ts]);
                } else {
                    assert(psf.psf_info.end < inmem_start_addr && "Illegal state, should be no in memory components!");
                    disk_len = psf.psf_info.end - static_cast<score_t>(discrete_times[ts]);
                }
            }


            assert(mem_len >= 0 && disk_len >= 0);
            const score_t mem_perc_traversed = mem_len / psf_len;
            const score_t disk_perc_traversed = disk_len / psf_len;
            assert(mem_perc_traversed <= 1.0 && disk_perc_traversed <= 1.0);

            const score_t psf_recs = static_cast<score_t>(psf.psfLen(len_ht));
            const score_t rec_mem = mem_perc_traversed * psf_recs;
            const score_t rec_disk = disk_perc_traversed * psf_recs;

            const score_t selectivity = psf_recs / core::recordCount(rec_count, discrete_times[ts], psf.psf_info.end);
            if (selectivity < 0.05) {
                const score_t x = std::log(selectivity);
                const score_t mem_mul = op::MEM_SKIP_COST_VERY_SEL_A + op::MEM_SKIP_COST_VERY_SEL_B * x;
                const score_t disk_mul = op::DISK_SKIP_COST_VERY_SEL_A + op::DISK_SKIP_COST_VERY_SEL_B * x;
                return costJustScan(ts, psf_index)
                       + mem_mul * rec_mem
                       + disk_mul * rec_disk;
            }

            const score_t x = 1 / selectivity;
            const score_t mem_mul = op::MEM_SKIP_COST_UNSEL_A + op::MEM_SKIP_COST_UNSEL_B * x;
            const score_t disk_mul = op::DISK_SKIP_COST_UNSEL_A + op::DISK_SKIP_COST_UNSEL_B * x;
            return costJustScan(ts, psf_index)
                   + mem_mul * rec_mem
                   + disk_mul * rec_disk;
        };

        int best_prev_idx = 0;
        score_t best_prev_score = costFullScan(0);
        opt.at(index(0, 0)) = {best_prev_score, -1, false};

        for (int psf_idx = 1; psf_idx < psfs.size(); ++psf_idx) {
            const auto score = psfs[psf_idx].covers(discrete_times[0])
                                   ? costTraversalAndScan(0, psf_idx)
                                   : TINY_INF;
            opt.at(index(0, psf_idx)) = {score, -1, false};
            if (score < best_prev_score) {
                best_prev_idx = psf_idx;
                best_prev_score = score;
            }
        }

        for (int ts = 1; ts < time_count - 1; ++ts) {
            const auto timestamp = discrete_times[ts];
            const auto fullscan_cost = costFullScan(ts) + best_prev_score;
            opt.at(index(ts, 0)) = {fullscan_cost, best_prev_idx, best_prev_idx == 0};
            int best_curr_idx = 0;
            score_t best_curr_score = fullscan_cost;

            for (int psf_idx = 1; psf_idx < psfs.size(); ++psf_idx) {
                const auto& psf = psfs[psf_idx];

                // If PSF doesn't cover the timestamp, can't use this
                if (!psf.covers(timestamp - 1)) {
                    opt.at(index(ts, psf_idx)) = {TINY_INF, -1, false};
                    continue;
                }

                // try to continue a scan
                DpEntry min = {
                    costJustScan(ts, psf_idx) + opt.at(index(ts - 1, psf_idx)).score,
                    psf_idx,
                    true
                };

                const score_t skip_cost = costTraversalAndScan(ts, psf_idx) + best_prev_score;
                if (skip_cost < min.score)
                    min = DpEntry{skip_cost, best_prev_idx, false};

                opt.at(index(ts, psf_idx)) = min;

                if (min.score < best_curr_score) {
                    best_curr_idx = psf_idx;
                    best_curr_score = min.score;
                }
            }

            best_prev_idx = best_curr_idx;
            best_prev_score = best_curr_score;
        }


        printf("%35s ", "");
        for (auto&& psf: psfs) {
            if (psf.psf_info.id == FS_ID) {
                printf("FS:  %29s\t", "");
            } else
                printf("PSF: %-29d\t", psf.id());
        }
        printf("\n");
        for (int i = 0; i < time_count - 1; ++i) {
            printf("[%12lu %12lu %c]: ", discrete_times[i + 1], discrete_times[i], segInMemory(i) ? 'm' : 'd');
            for (int j = 0; j < psfs.size(); ++j) {
                opt.at(index(i, j)).print();
            }
            printf("\n");
        }

        // starts initially as smallest of the last psfs
        int next_psf_idx = 0;
        for (int i = 1; i < psfs.size(); ++i) {
            if (opt.at(index(time_count - 2, i)).score < opt.at(index(time_count - 2, next_psf_idx)).score) {
                next_psf_idx = i;
            }
        }
        std::vector<ScanInfo> ret;
        bool prev_was_fs = false;
        bool continuing_scan = false;
        for (int ts = time_count - 2; ts >= 0; --ts) {
            assert(next_psf_idx >= 0 && next_psf_idx < psfs.size());
            const auto& psf = psfs[next_psf_idx];
            auto beg_ts = discrete_times[ts + 1];
            if (prev_was_fs) {
                --ret.back().psf_info.end;
            } else {
                ++beg_ts;
            }
            // const auto end_ts = discrete_times[ts] +;
            if (!continuing_scan) {
                ret.emplace_back(
                    PsfInfo{psf.psf_info.id, beg_ts, discrete_times[ts]},
                    psf.s_val // use s_val, as if we copy s_val, we will copy i_val as well for free
                );
            } else {
                ret.back().psf_info.end = discrete_times[ts];
            }

            prev_was_fs = next_psf_idx == 0;

            continuing_scan = opt.at(index(ts, next_psf_idx)).continuing_scan;
            next_psf_idx = opt.at(index(ts, next_psf_idx)).prev_index;
        }
        assert(!continuing_scan && "At end, should not be contiuing a scan");
        ret[0].psf_info.start = start; // fix off by 1 as bounds are inclusive

        printf("\nFinal Plan:\n");
        for (const auto& psf_info: ret) {
            if (psf_info.psf_info.id != FS_ID)
                printf("[%c%2d: %lu %lu]\n", psf_info.psf_info.id.isInline() ? 'I' : 'G', psf_info.id(),
                       psf_info.psf_info.start, psf_info.psf_info.end);
            else
                printf("[FS:  %lu %lu]\n", psf_info.psf_info.start, psf_info.psf_info.end);
        }


        printf("[Ending Planning]\n\n");

        return ret;
        /*
                printf("\nPsf:                 :[");
                for (int i = 0; i < psf_count; ++i) {
                    printf("Psf%4d,   ", psfs[i].id());
                }
                printf("\n");
                for (int i = 0; i < time_count - 1; ++i) {
                    printf("Timestamp: %10ld:", discrete_times[i]);
                    for (int j = 0; j < psf_count; ++j) {
                        printf("[%3.2f %3d], ", opt.at(index(i, j)).score, opt.at(index(i, j)).prev_index);
                    }
                    printf("\n");
                }*/

        /*
                for (int i = 0; i < time_count; ++i) {
                    const auto timestamp = discrete_times[i];

                    bool any_covered = false;

                    uint32_t best_round_score = TINY_INF;
                    int best_index = -1;

                    for (int j = 0; j < psfs.size(); ++j) {
                        const auto& psf = psfs[j];

                        // only consider PSFs that cover the timestamp
                        if (!psf.covers(timestamp)) {
                            opt.at(index(i, j)) = {TINY_INF, prev_best_index};
                            continue;
                        }


                        // PSF covers that timestamp
                        any_covered = true;
                        if (!previous_any_covered) {
                            // we can't continue off an old psf scan
                            opt.at(index(i, j)) = {1, -1, true};
                        } else {
                            // we may continue off an old psf scan.
                            uint32_t min = opt.at(index(i - 1, j)).score;
                            int prev_id = j;

                            for (int k = 0; k < psfs.size(); ++k) {
                                const uint32_t new_min = opt.at(index(i - 1, k)).score + 1;
                                if (min > new_min) {
                                    min = new_min;
                                    prev_id = k;
                                }
                            }

                            opt.at(index(i, j)) = {min, prev_id, true};
                        }

                        if (opt.at(index(i, j)).score < best_round_score) {
                            best_round_score = opt.at(index(i, j)).score;
                            best_index = j;
                        }
                    }

                    previous_any_covered = any_covered;
                    prev_best_index = best_index;
                }

                // makes inline PSF if index is not -1, else makes FS_ID
                auto makePsfId = [](const int my_id) {
                    if (my_id == -1)
                        return FS_ID;
                    return PsfId::Inline(my_id);
                };
                auto best_psf = makePsfId(prev_best_index);

                std::vector<PsfInfo> ret;
                PsfId current_id = (best_psf == NO_PSF) ? FS_ID : psfs[best_psf.id].id;
                PsfAddress end_addr = end;

                for (int i = time_count - 1; i >= 0; --i) {
                    if (best_psf == NO_PSF || !opt.at(index(i, best_psf.id)).valid) {
                        if (current_id != FS_ID) {
                            ret.emplace_back(current_id, discrete_times[i + 1], end_addr);

                            end_addr = discrete_times[i];
                            current_id = FS_ID;
                        }
                        if (i >= 1)
                            best_psf = makePsfId(opt.at(index(i - 1, 0)).prev_index);
                    } else {
                        PsfId psf_id = psfs.at(best_psf.id).id;
                        if (current_id != psf_id) {
                            ret.emplace_back(current_id, discrete_times[i + 1], end_addr);

                            end_addr = discrete_times[i];
                            current_id = psf_id;
                        }
                        if (i >= 1)
                            best_psf = makePsfId(opt.at(index(i, best_psf.id)).prev_index);
                    }
                }
                ret.emplace_back(current_id, start, end_addr); // add the last one

                return ret;*/
    }
}
