// A group by that uses builtin FishStore scans, but requires that the domain of the group by function be iteratable
//
// Created by Max Norfolk on 12/29/23.

#pragma once

#include <utility>

#include "plan/operators/abstract_op.h"
#include "aggregate/agg_manager.h"

namespace fishstore::plan::op {
    template<typename D, typename A, typename V>
    class PsfScanGroupBy : public AbstractStartOp {
    public:
        typedef fishstore::core::FishStore<D, A> store_t;
        static_assert(std::is_same_v<V, int> || std::is_same_v<V, adapter::StringRef>,
                      "V must be int or adapter::StringRef");

        // TODO give this use
        typedef std::function<record::Record(record::Record *)> GroupByFunction;

        PsfScanGroupBy(store_t &store, PsfId psf, Schema all_fields_, aggregate::AggregateManager &&manager,
                       psudb::DynamicArray<V> possible_values,
                       Filter where = {},
                       PsfAddress lower = 0, PsfAddress upper = core::Address::kMaxAddress)
            : store(store), psf_id(psf), default_manager(std::move(manager)),
              possible_values(std::move(possible_values)),
              map(possible_values.size()), where_filter(std::move(where)),
              all_fields(std::move(all_fields_)),
              lower_bound(lower), upper_bound(upper) {
            if constexpr (std::is_same_v<V, int>) {
                const auto temp = Schema{{Schema::FieldInfo::IntField("group", psf)}};
                final_schema = Schema::merge(temp, default_manager.GetSchema());
            } else if constexpr (std::is_same_v<V, adapter::StringRef>) {
                const auto temp = Schema{{Schema::FieldInfo::StringField("group", psf)}};
                final_schema = Schema::merge(temp, default_manager.GetSchema());
            }

            assert(!possible_values.empty() && "There should be at least one possible group!");
            for (int i = 0; i < possible_values.size(); ++i) {
                map.at(i) = default_manager.New();
            }
        }

        ~PsfScanGroupBy() override = default;

        /**
         * filter for the where clause in a SQL query. This filter is applied before the aggregations happen
         * use filter_op.h to apply a having clause
         *
         * for example:
         * SELECT COUNT(*)
         * FROM ...
         * WHERE salary > 10
         * GROUP BY salary
         * HAVING COUNT(*) > 5
         * in this case, it is the salary > 10
         */
        [[nodiscard]] inline const Filter &GetFilter() const { return where_filter; }

        void StartScan() override;

        inline void Initialize(ThreadId thread_count) override;

        inline void Touch(ThreadId, record::Record *) override {
            assert("false && Use Touch(ThreadId, record::Record*, V value) instead!");
        }

        // Value is the PSF's value
        inline void Touch(ThreadId, record::Record *, uint mapped_value);

        inline void Finalize(ThreadId) override;

        inline void FinalizeAll() override;

        // final schema
        [[nodiscard]] inline const Schema &GetSchema() const override { return final_schema; }

        // all fields that need to be extracted
        [[nodiscard]] inline const Schema &GetAllFieldsSchema() const { return all_fields; }

        inline void LinkParent(AbstractOp *op) override {
            parent = op;
        }

        inline void LinkChild(AbstractOp *op) override {
            assert(op == nullptr && "TableScan must be the lowest level");
        }

        void PrettyPrint() override { printf("TODO PSF scan group by\n"); }

    private:
        // FishStore
        store_t &store;
        PsfId psf_id = NO_PSF;

        // Aggregate Stuff
        // same index in possible_values will map to the same index in map array
        aggregate::AggregateManager default_manager;
        psudb::DynamicArray<V> possible_values;
        psudb::DynamicArray<aggregate::AggregateManager> map;


        Filter where_filter;


        // schema of the aggregates & group combined
        Schema all_fields;
        Schema final_schema;

        // scan stuff
        PsfAddress lower_bound;
        PsfAddress upper_bound;

        AbstractOp *parent = nullptr;

        // TODO secondary hash for extra group by
        // GroupByFunction group_by_func;
        /* std::unordered_map<V,
         std::unordered_map<record::Record, aggregate::AggregateManager, record::HashRecord, record::RecordEqual>>
                 map;*/
    };
}
