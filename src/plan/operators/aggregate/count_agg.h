// COUNT(*) will count the number of records
//
// Created by Max Norfolk on 1/9/24.


#pragma once

#include "plan/operators/aggregate/abstract_agg.h"
#include "plan/record_helper.h"

namespace fishstore::plan::op::aggregate {
    class CountAggregate : public AbstractAggregate {
    public:
        std::unique_ptr<AbstractAggregate> New() override {
            return std::unique_ptr<AbstractAggregate>{new CountAggregate()};
        }

        void Touch(record::Value* value) override {
            count++;
        }

        void Merge(AbstractAggregate* agg) override {
            count += dynamic_cast<CountAggregate *>(agg)->count;
        }

        record::Value Finalize() override {
            return record::Value{count};
        }

        ezpsf::DataType InputType() override {
            return ezpsf::DataType::INT32_T; // can be int32_t doesn't matter
        }

        ezpsf::DataType OutputType() override {
            return ezpsf::DataType::INT32_T;
        }

        std::string Name() override {
            return "COUNT";
        }

    private:
        int32_t count = 0;
    };
}
