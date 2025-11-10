//
// Created by mxn5437 on 2/27/24.
//

#pragma once

#include <utility>
#include <string>

#include "plan/operators/aggregate/agg_types.h"

namespace fishstore::plan::data_frame::aggregate {
    using AggregatePair = std::pair<op::aggregate::AggregateType, std::string>;

    [[nodiscard]] inline AggregatePair Sum(const std::string &arg) { return {op::aggregate::AggregateType::SUM, arg}; }

    [[nodiscard]] inline AggregatePair Count(const std::string &arg) {
        return {op::aggregate::AggregateType::COUNT, arg};
    }

    template<typename D, typename A>
    op::aggregate::AggregateManager makeAggregateManager(DataFrame<D, A> &df, const std::vector<AggregatePair> &vec) {
        typedef std::pair<size_t, std::unique_ptr<op::aggregate::AbstractAggregate> > pair_t;

        psudb::DynamicArray<pair_t> manager_args(vec.size());

        for (int i = 0; i < vec.size(); ++i) {
            const auto &pair = vec[i];
            switch (pair.first) {
                case op::aggregate::AggregateType::SUM: {
                    size_t index = df.index_maker.mapFieldId(pair.second);
                    manager_args[i] = std::move(
                        std::make_pair(index, std::make_unique<op::aggregate::SumAggregate>())
                    );
                    break;
                }
                case op::aggregate::AggregateType::COUNT:

                    // for count, doesn't matter what they write, just count the number of records
                    manager_args[i] = std::move(
                        std::make_pair(0, std::make_unique<op::aggregate::CountAggregate>())
                    );
                    break;
                case op::aggregate::AggregateType::AVG:
                case op::aggregate::AggregateType::MIN:
                case op::aggregate::AggregateType::MAX:
                case op::aggregate::AggregateType::ERROR_AGG:
                    assert(false && "Invalid aggregate type");
            }
        }
        return op::aggregate::AggregateManager{manager_args};
    }
}
