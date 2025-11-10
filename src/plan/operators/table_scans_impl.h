// Table Scan implementation -- seperated, so we can reference scan_contexts in table scan, but TableScan in scan_contexts
//
// Created by Max Norfolk on 1/2/24.


#pragma once

#include "table_scans.h"
#include "scan_contexts.h"

namespace fishstore::plan::op {
    template<typename D, typename A>
    void TableScan<D, A>::StartScan() {
        core::AsyncCallback callback = [](fishstore::core::IAsyncContext*, fishstore::core::Status status) {
            assert(status == fishstore::core::Status::Ok);
        };

        // execute each scan
        // TODO thread pool
        for (int i = 0; i < scan_plan.size(); ++i) {
            const auto& item = scan_plan[i];
            ThreadId thread_id = i;
            if (item.psf_info.id == FS_ID) {
                printf("FS  from %lu to %lu\n", item.start(), item.end());
                // FullScan
                contexts::PlanFullScanContext<D, A> fs_ctx(thread_id, *this);
                store.FullScan(fs_ctx, callback, 1, item.start(), item.end());
            } else {
                printf("PSF %c%d from %lu to %lu\n", item.psf_info.id.isInline() ? 'I' : 'G', item.psf_info.id.id,
                       item.start(), item.end());
                // regular scan
                if (item.isGeneral()) {
                    contexts::PlanScanContext<D, A, adapter::StringRef> scan_ctx(
                        thread_id, *this, item.psf_info.id, item.s_val);
                    store.Scan(scan_ctx, callback, 1, item.start(), item.end());
                } else {
                    contexts::PlanScanContext<D, A, int> scan_ctx(thread_id, *this, item.psf_info.id, item.i_val);
                    store.Scan(scan_ctx, callback, 1, item.start(), item.end());
                }
            }
            // the scan will call finalize when its thread is done
        }
        store.CompletePending(true);
        this->parent->FinalizeAll();
    }

    template<typename D, typename A>
    void TableScan<D, A>::Touch(ThreadId id, record::Record* rec) {
        this->parent->Touch(id, rec);
    }

    template<typename D, typename A>
    void TableScan<D, A>::Initialize(ThreadId thread_count) {
        this->parent->Initialize(thread_count);
    }

    template<typename D, typename A>
    void TableScan<D, A>::Finalize(ThreadId id) {
        this->parent->Finalize(id);
    }

    template<typename D, typename A>
    void TableScan<D, A>::FinalizeAll() {
        this->parent->FinalizeAll();
    }
}
