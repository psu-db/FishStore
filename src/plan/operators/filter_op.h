// A basic filter operator that verifies all records satisfies the set of CNF clauses similar to
// the format for the TableScan's filter
//
// Created by Max Norfolk on 3/26/24.

#pragma once

#include "abstract_op.h"
#include "plan/filter.h"

namespace fishstore::plan::op {
    class FilterOp : public AbstractOp {
    public:
        explicit FilterOp(Filter filter) : filter(std::move(filter)) {}

        inline void Initialize(ThreadId thread_count) override { parent->Initialize(thread_count); }

        inline void Touch(ThreadId id, record::Record *rec) override {
            if (filter.check(*rec)) {
                parent->Touch(id, rec);
            }
        }

        inline void Finalize(ThreadId id) override { parent->Finalize(id); }

        inline void FinalizeAll() override { return parent->FinalizeAll(); }

        [[nodiscard]] inline const Schema &GetSchema() const override { return child->GetSchema(); }

        inline void LinkParent(AbstractOp *op) override { parent = op; }

        inline void LinkChild(AbstractOp *op) override { child = op; }

        inline void PrettyPrint() override {
            printf("[FilterOp]::\n");
        }

    private:
        AbstractOp *parent = nullptr;
        AbstractOp *child = nullptr;
        Filter filter;
    };
}
