//
//
// Created by Max Norfolk on 1/30/24.


#pragma once

#include "plan/operators/scan_contexts.h"

namespace fishstore::plan::op {
    template<typename D, typename A, typename V>
    void PsfScanGroupBy<D, A, V>::StartScan() {
        const auto begin = possible_values.begin();
        const auto end = possible_values.end();
        Initialize(static_cast<ThreadId>(end - begin));

        core::AsyncCallback callback = [](fishstore::core::IAsyncContext *, fishstore::core::Status status) {
            assert(status == fishstore::core::Status::Ok);
        };
        uint map_value = 0;
        for (auto it = begin; it != end; ++it) {
            if constexpr (std::is_same_v<V, int>) {
                contexts::PlanInlineGroupByScanContext<D, A> scan_ctx{map_value, *this, psf_id, *it};
                store.Scan(scan_ctx, callback, 1, lower_bound, upper_bound);
            } else if constexpr (std::is_same_v<V, adapter::StringRef>) {
                contexts::PlanGeneralGroupByScanContext<D, A> scan_ctx{map_value, *this, psf_id, *it};
                store.Scan(scan_ctx, callback, 1, lower_bound, upper_bound);
            }

            // handle secondary group by

            map_value++;
        }
        store.CompletePending(true);
        FinalizeAll();
    }

    template<typename D, typename A, typename V>
    void PsfScanGroupBy<D, A, V>::Initialize(ThreadId thread_count) {
        parent->Initialize(thread_count);
    }

    template<typename D, typename A, typename V>
    void PsfScanGroupBy<D, A, V>::Touch(ThreadId thread_id, record::Record *rec, uint mapped_value) {
        // TODO make it work with secondary group by
        /*auto &second_group_by = map.at(value);

        const auto group_record = group_by_func(rec);


        const auto it = second_group_by.find(group_record);
        if (it == second_group_by.end()) {
            second_group_by.emplace(group_record, default_manager.New());
        }
         auto &manager = second_group_by[group_record];
        manager.Touch(rec);*/
        map.at(mapped_value).Touch(rec);
    }

    template<typename D, typename A, typename V>
    void PsfScanGroupBy<D, A, V>::Finalize(ThreadId thread_id) {
        parent->Finalize(thread_id);
    }

    template<typename D, typename A, typename V>
    void PsfScanGroupBy<D, A, V>::FinalizeAll() {
        parent->Initialize(1);
        for (int i = 0; i < map.size(); ++i) {
            auto rec = record::Record::merge(record::Record{record::Value(possible_values[i])}, map[i].Finalize());
            parent->Touch(0, &rec);
        }
        parent->FinalizeAll();
    }
}
