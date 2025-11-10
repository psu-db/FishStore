// Projection operator
//
// Created by Max Norfolk on 12/13/23.


#pragma once

#include <string>
#include <utility>
#include <vector>
#include "plan/record_helper.h"

#include "plan/operators/abstract_op.h"

#include "adapters/simdjson_adapter.h"

namespace fishstore::plan::op {
    class Projection : public AbstractOp {
    public:
        // maps a record* to a new record
        // if you drop any string fields, you must manage the memory
        // by doing delete[] on the string ref pointer
        typedef std::function<record::Record(record::Record *)> ProjectionFunction;

        Projection() : proj_func(), proj_schema() {
        }

        Projection(Schema schema, ProjectionFunction func) : proj_func(std::move(func)),
                                                             proj_schema(std::move(schema)) {
        }

        void Touch(ThreadId id, record::Record *record) override {
            auto temp_rec = proj_func(record);
            if (parent != nullptr)
                parent->Touch(id, &temp_rec);
        }

        void Initialize(ThreadId thread_count) override {
            if (parent != nullptr)
                parent->Initialize(thread_count);
        }

        void Finalize(ThreadId id) override {
            if (parent != nullptr)
                parent->Finalize(id);
        }

        void FinalizeAll() override {
            if (parent != nullptr)
                parent->FinalizeAll();
        }

        [[nodiscard]] const Schema &GetSchema() const override {
            return proj_schema;
        }

        void LinkParent(AbstractOp *op) override { parent = op; }

        void LinkChild(AbstractOp *op) override {
        }

        void PrettyPrint() override { printf("[Projection]::%s", proj_schema.toString().c_str()); }

    private:
        AbstractOp *parent = nullptr;
        const ProjectionFunction proj_func;
        const Schema proj_schema;
    };
}
