//
//
// Created by Max Norfolk on 1/9/24.


#pragma once

#include "plan/operators/aggregate/abstract_agg.h"

namespace fishstore::plan::op::aggregate {
    class SumAggregate : public AbstractAggregate {
    public:
        std::unique_ptr<AbstractAggregate> New() override {
            return std::unique_ptr<AbstractAggregate>{new SumAggregate()};
        }

        void Touch(record::Value *value) override {
            sum += value->int32;
        }

        void Merge(AbstractAggregate *agg) override {
            sum += dynamic_cast<SumAggregate *>(agg)->sum;
        }

        record::Value Finalize() override {
            return record::Value{sum};
        }

        ezpsf::DataType InputType() override {
            return ezpsf::DataType::INT32_T; // any
        }

        ezpsf::DataType OutputType() override {
            return ezpsf::DataType::INT32_T;
        }

        std::string Name() override {
            return "SUM";
        }

    private:
        int32_t sum = 0;
    };
}
