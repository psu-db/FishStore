//
//
// Created by Max Norfolk on 1/8/24.


#pragma once

#include <utility>

#include "plan/operators/abstract_op.h"
#include "plan/operators/aggregate/agg_manager.h"

namespace fishstore::plan::op {
    class InMemoryGroupBy : public AbstractOp {
    public:
        // maps a record to a new record that is used to define the groups.
        // if 2 records map to the same record, they are considered in the
        // same group
        typedef std::function<record::Record(record::Record *)> GroupByFunction;

        constexpr static size_t DEFAULT_SIZE = 65536;

        InMemoryGroupBy() : map(DEFAULT_SIZE,
                                record::HashRecord{&group_by_schema},
                                record::RecordEqual{(&group_by_schema)}) {
        }

        InMemoryGroupBy(Schema group_by_schema_, GroupByFunction func, aggregate::AggregateManager &&manager)
            : group_by_schema(std::move(group_by_schema_)),
              group_by(std::move(func)),
              default_manager(std::move(manager)),
              record_schema(Schema::merge(group_by_schema, default_manager.GetSchema())),
              map(DEFAULT_SIZE,
                  record::HashRecord{(&group_by_schema)},
                  record::RecordEqual{(&group_by_schema)}
              ) {
        }

        void Initialize(ThreadId thread_count) override {
            // TODO setup thread safe stuff
        }

        void Touch(ThreadId id, record::Record *record) override {
            record::Record group_rec = group_by(record);

            // if there exists a manager already, don't make a new one
            // if there doesn't, make a new one
            auto it = map.find(group_rec);
            if (it == map.end()) {
                map.emplace(group_rec, default_manager.New());
            }
            map[group_rec].Touch(record);
            #ifdef SAVE_RECORDS
            all_records[it].emplace_back(*record); // copy to save ptrs
            #endif
        }

        void Finalize(ThreadId id) override {
            // TODO make it thread safe
        }

        void FinalizeAll() override {
            parent->Initialize(1);
            for (auto &pair: map) {
                auto rec = record::Record::merge(pair.first, pair.second.Finalize());
                parent->Touch(0, &rec);
            }
            parent->FinalizeAll();
        }

        [[nodiscard]] const Schema &GetSchema() const override {
            return record_schema;
        }

        void LinkParent(AbstractOp *op) override { parent = op; }

        void LinkChild(AbstractOp *op) override {
        }

        void PrettyPrint() override { printf("[In Memory Group By]::%s\n", record_schema.toString().c_str()); }

    private:
        const Schema group_by_schema; // schema for how group is defined
        const GroupByFunction group_by;

        const aggregate::AggregateManager default_manager;

        const Schema record_schema; // schema for records passed up

        AbstractOp *parent = nullptr;


        std::unordered_map<record::Record, aggregate::AggregateManager, record::HashRecord, record::RecordEqual> map;

        #ifdef SAVE_RECORDS
        std::unordered_map<record::Record, std::vector<record::Record>, record::HashRecord, record::RecordEqual> all_records;
        #endif
    };
}
