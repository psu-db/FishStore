// Limits the number of records that will be passed up to the parent
// for example, if there 100 records in the child of the limit operator
// but limit is 3, then the parent will see exactly 3 records
//
// Created by Max Norfolk on 2/27/24.


#pragma once

#include "plan/operators/abstract_op.h"

namespace fishstore::plan::op {
    class Limit : public AbstractOp {
    public:
        explicit Limit(size_t limit) : limit(limit) {
        }

        void Initialize(ThreadId thread_count) override {
        }

        void Touch(ThreadId id, record::Record *rec) override {
            if (seen < limit) {
                parent->Touch(id, rec);
                ++seen;
            }
        }

        void Finalize(ThreadId id) override { parent->Finalize(id); }

        void FinalizeAll() override { parent->FinalizeAll(); }

        [[nodiscard]] const Schema &GetSchema() const override {
            return child->GetSchema();
        }

        void LinkParent(AbstractOp *op) override { parent = op; }

        void LinkChild(AbstractOp *op) override { child = op; }

        void PrettyPrint() override { printf("[Limit]::%lu\n", limit); }

    private:
        AbstractOp *child = nullptr;
        AbstractOp *parent = nullptr;
        uint64_t seen = 0;
        uint64_t limit;
    };
}
