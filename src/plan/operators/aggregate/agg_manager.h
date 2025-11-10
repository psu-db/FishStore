// Manages calling touch with the correct value for each aggregates operator
//
// Created by Max Norfolk on 1/9/24.


#pragma once

#include <utility>

#include "plan/operators/aggregate/abstract_agg.h"

namespace fishstore::plan::op::aggregate {
    class AggregateManager {
    public:
        AggregateManager() : aggs() {}

        // public constructor that initializes everything correctly
        explicit AggregateManager(
            const psudb::DynamicArray<std::pair<size_t, std::unique_ptr<AbstractAggregate>>> &from_aggs)
            : aggs(from_aggs.size()) {
            for (int i = 0; i < aggs.size(); ++i) {
                this->aggs[i] = {from_aggs[i].first, from_aggs[i].second->New()};
            }
        }

        // pair of index and pointer to the aggregate.
        explicit AggregateManager(psudb::DynamicArray<std::pair<size_t, AbstractAggregate *>> aggs)
            : aggs(aggs.size()) {
            for (int i = 0; i < aggs.size(); ++i) {
                this->aggs[i] = {aggs[i].first, aggs[i].second->New()};
            }
        }

        void Touch(record::Record *rec) {
            for (auto &pair: aggs) {
                const auto &index = pair.first;
                auto &agg = pair.second;

                auto &value = rec->at(index);
                agg->Touch(&value);
            }
        }

        // merges the other aggregate manager's with this one
        void Merge(const AggregateManager &other) {
            for (int i = 0; i < aggs.size(); ++i) {
                aggs[i].second->Merge(other.aggs[i].second.get());
            }
        }

        record::Record Finalize() {
            record::Record ret{aggs.size()};
            for (int i = 0; i < aggs.size(); ++i) {
                ret[i] = aggs[i].second->Finalize();
            }
            return ret;
        }

        // creates a new aggregate manager with the same information, but this one has abstract aggregates with
        // default values
        [[nodiscard]] AggregateManager New() const {
            return AggregateManager{aggs};
        }

        // generates the schema
        [[nodiscard]] Schema GetSchema() const {
            psudb::DynamicArray<Schema::FieldInfo> fields(aggs.size());
            for (int i = 0; i < aggs.size(); ++i) {
                const auto& ptr = aggs[i].second;
                fields[i] = {ptr->Name(), ptr->OutputType()};
            }
            return Schema{fields};
        }

    private:
        psudb::DynamicArray<std::pair<size_t, std::unique_ptr<AbstractAggregate>>> aggs;
    };
}
