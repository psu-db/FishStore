//
//
// Created by Max Norfolk on 9/29/23.


#pragma once

#include "core/fishstore.h"

#include "psf_scan_group_by.h"
#include "table_scans.h"

namespace fishstore::plan::op::contexts {
    // full scans used by the query planner/executor
    template<typename D, typename A>
    class PlanFullScanContext : public core::IAsyncContext {
    public:
        PlanFullScanContext() = delete;

        explicit PlanFullScanContext(const ThreadId id, TableScan<D, A>& ts)
            : id(id),
              table_scan(ts),
              extract(ts.GetSchema()),
              current_record(ts.GetSchema().size()) {}

        inline void Touch(const fishstore::core::Record* rec) {
            table_scan.Touch(id, &current_record);
        }

        inline void Finalize() {
            table_scan.Finalize(id);
        }

        inline bool check(const fishstore::core::Record* rec) {
            auto&& [kpt_map,valid] = extract.apply(rec, current_record);
            if (!valid)
                return false;
            // printf("Got Record! [%s]\n", current_record.toString(extract.schema).c_str());
            return table_scan.GetFilter().checkKpts(current_record, kpt_map);
        }

    protected:
        core::Status DeepCopy_Internal(IAsyncContext*& context_copy) override {
            return IAsyncContext::DeepCopy_Internal(*this, context_copy);
        }

    private:
        const ThreadId id;
        TableScan<D, A>& table_scan;
        record::FieldExtract<A> extract;
        record::Record current_record;
    };

    template<typename D, typename A, typename V>
    class PlanScanContext : public core::IAsyncContext {
    public:
        PlanScanContext() = delete;


        explicit PlanScanContext(ThreadId id, TableScan<D, A>& ts, PsfId psf_id, V value)
            : id(id), table_scan(ts),
              extract(ts.GetSchema()),
              current_record(ts.GetSchema().size()),
              value(value), psf_id(psf_id.id) {
            assert(psf_id != NO_PSF);
            if constexpr (std::is_same_v<V, adapter::StringRef>)
                assert(psf_id.isGeneral() && "PSF should be general for PlanScanContext, with type as Strings");
            else
                assert(psf_id.isInline() && "PSF should be inline for PlanScanContext");
        }

        inline void Touch(const fishstore::core::Record* rec) {
            table_scan.Touch(id, &current_record);
        }

        inline void Finalize() {
            table_scan.Finalize(id);
        }

        inline bool check(const fishstore::core::KeyPointer* kpt) {
            if constexpr (std::is_same_v<V, adapter::StringRef>) {
                // string type
                adapter::StringRef rec_str{kpt->get_value(), kpt->value_size};

                // if all true,fall through
                if (!(kpt->mode == 0 && kpt->general_psf_id == psf_id
                      && rec_str.Length() == value.Length()
                      && strncmp(rec_str.Data(), value.Data(), value.Length()) == 0)) {
                    return false;
                }
            } else {
                // int type
                if (!(kpt->mode == 1 && kpt->inline_psf_id == psf_id && kpt->value == value))
                    return false;
            }
            // necessary but not sufficient


            auto fishstore_record = kpt->get_record();
            auto&& [kpt_map, valid] = extract.apply(fishstore_record, current_record);
            if (!valid)
                return false;
            auto filter = table_scan.GetFilter().checkKpts(PsfId{
                                                               psf_id,
                                                               std::is_same_v<V, adapter::StringRef>
                                                                   ? core::PsfType::GENERAL
                                                                   : core::PsfType::INLINE,
                                                           }, current_record, kpt_map);
            return filter;
        }

        [[nodiscard]] inline fishstore::core::KeyHash get_hash() const {
            if constexpr (std::is_same_v<V, adapter::StringRef>)
                return fishstore::core::KeyHash{
                    fishstore::core::Utility::HashBytesWithPSFID(psf_id, value.Data(), value.Length())
                };
            else
                return fishstore::core::KeyHash{fishstore::core::Utility::GetHashCode(psf_id, value)};
        }

    protected:
        core::Status DeepCopy_Internal(IAsyncContext*& context_copy) override {
            return IAsyncContext::DeepCopy_Internal(*this, context_copy);
        }

    private:
        const ThreadId id;
        TableScan<D, A>& table_scan;
        record::FieldExtract<A> extract;
        record::Record current_record;
        const V value;
        const uint32_t psf_id;
    };

    template<typename D, typename A>
    class PlanInlineGroupByScanContext : public core::IAsyncContext {
    public:
        PlanInlineGroupByScanContext() = delete;

        explicit PlanInlineGroupByScanContext(uint mapped_value, PsfScanGroupBy<D, A, int>& group_by, PsfId psf_id,
                                              int value)
            : mapped_value(mapped_value), group_by(group_by),
              extract(group_by.GetAllFieldsSchema()),
              current_record(group_by.GetAllFieldsSchema().size()),
              value(value), psf_id(psf_id.id) {
            assert(psf_id != NO_PSF);
            assert(psf_id.isInline() && "This ScanContext requires inline PSFs");
        }

        inline void Touch(const fishstore::core::Record* rec) {
            group_by.Touch(thread_id, &current_record, mapped_value);
        }

        inline void Finalize() {
            group_by.Finalize(thread_id);
        }

        inline bool check(const fishstore::core::KeyPointer* kpt) {
            if (!(kpt->mode == 1 && kpt->inline_psf_id == psf_id && kpt->value == value))
                return false;
            // necessary but not sufficient
            auto fishstore_record = kpt->get_record();

            extract.apply(fishstore_record, current_record);
            return group_by.GetFilter().check(current_record);
        }

        [[nodiscard]] inline fishstore::core::KeyHash get_hash() const {
            return fishstore::core::KeyHash{fishstore::core::Utility::GetHashCode(psf_id, value)};
        }

    protected:
        core::Status DeepCopy_Internal(IAsyncContext*& context_copy) override {
            return IAsyncContext::DeepCopy_Internal(*this, context_copy);
        }

    private:
        ThreadId thread_id = 0;
        const uint mapped_value;
        PsfScanGroupBy<D, A, int>& group_by;
        record::FieldExtract<A> extract;
        record::Record current_record;
        const uint32_t value;
        const uint32_t psf_id;
    };

    template<typename D, typename A>
    class PlanGeneralGroupByScanContext : public core::IAsyncContext {
    public:
        PlanGeneralGroupByScanContext() = delete;

        explicit PlanGeneralGroupByScanContext(uint mapped_value, PsfScanGroupBy<D, A, adapter::StringRef>& group_by,
                                               PsfId psf_id,
                                               const adapter::StringRef& value)
            : mapped_value(mapped_value), group_by(group_by),
              extract(group_by.GetAllFieldsSchema()),
              current_record(group_by.GetAllFieldsSchema().size()),
              value(value), psf_id(psf_id.id) {
            assert(psf_id != NO_PSF);
            assert(psf_id.isGeneral() && "This ScanContext requires general PSF");
        }

        inline void Touch(const fishstore::core::Record* rec) {
            group_by.Touch(thread_id, &current_record, mapped_value);
        }

        inline void Finalize() {
            group_by.Finalize(thread_id);
        }

        inline bool check(const fishstore::core::KeyPointer* kpt) {
            adapter::StringRef rec_str{kpt->get_value(), kpt->value_size};
            if (!(kpt->mode == 0 && kpt->general_psf_id == psf_id && ezpsf::str_info::FUNC_STR_CMP(rec_str, value) ==
                  0))
                return false;
            // necessary but not sufficient
            auto fishstore_record = kpt->get_record();

            extract.apply(fishstore_record, current_record);
            return group_by.GetFilter().check(current_record);
        }

        [[nodiscard]] inline fishstore::core::KeyHash get_hash() const {
            return core::KeyHash(core::Utility::HashBytesWithPSFID(psf_id, value.Data(), value.Length()));
        }

    protected:
        core::Status DeepCopy_Internal(IAsyncContext*& context_copy) override {
            return IAsyncContext::DeepCopy_Internal(*this, context_copy);
        }

    private:
        ThreadId thread_id = 0;
        const uint mapped_value;
        PsfScanGroupBy<D, A, adapter::StringRef>& group_by;
        record::FieldExtract<A> extract;
        record::Record current_record;
        const adapter::StringRef value;
        const uint32_t psf_id;
    };
}
