// Joins multiple streams into one
//
// Created by Max Norfolk on 3/26/24.

#pragma once
#include "plan/operators/abstract_start_op.h"

namespace fishstore::plan::op {
    class UnionStart : public AbstractStartOp {
    public:
        inline void Initialize(ThreadId thread_count) override { parent->Initialize(thread_count); }

        inline void Touch(ThreadId id, record::Record *rec) override { parent->Touch(id, rec); }

        inline void Finalize(ThreadId id) override { parent->Finalize(id); }

        inline void FinalizeAll() override {
            child_finalize_count++;
            if (child_finalize_count == children.size())
                parent->FinalizeAll();
        }

        [[nodiscard]] inline const Schema &GetSchema() const override {
            assert(!children.empty() && "Link a child to get the schema!");
            return children[0]->GetSchema();
        }

        inline void LinkParent(AbstractOp *op) override { parent = op; }

        inline void LinkChild(AbstractOp *) override { assert(false && "Use LinkChild AbstractStartOp"); }

        inline void LinkChild(AbstractStartOp *op) { children.emplace_back(op); }

        inline void PrettyPrint() override {
            printf("[Union]::[%ld]", children.size());
        }

        void StartScan() override {
            for (const auto &child: children)
                child->StartScan();
        }

    private:
        std::vector<AbstractStartOp *> children;
        AbstractOp *parent = nullptr;
        size_t child_finalize_count = 0;
    };
}
