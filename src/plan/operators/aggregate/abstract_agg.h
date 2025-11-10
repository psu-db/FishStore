// Abstract class for aggregates
//
// Created by Max Norfolk on 1/8/24.


#pragma once
namespace fishstore::plan::op::aggregate {
    class AbstractAggregate {
    public:
        // creates a new abstract aggregate that copies any important data about this one, but
        // has an empty value.
        virtual std::unique_ptr<AbstractAggregate> New() = 0;

        virtual ~AbstractAggregate() = default;

        virtual void Touch(record::Value*) = 0;

        // takes all changes from the passed aggregate, and apply it
        // for example two counts are merged by addition
        // instead of incrementing a single counter for a record
        virtual void Merge(AbstractAggregate*) = 0;

        virtual record::Value Finalize() = 0;

        virtual ezpsf::DataType InputType() = 0;

        virtual ezpsf::DataType OutputType() = 0;

        virtual std::string Name() = 0;
    };
}
