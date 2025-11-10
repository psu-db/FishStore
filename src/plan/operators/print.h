// Prints to the console based upon the schema
//
// Created by Max Norfolk on 2/28/24.


#pragma once

#include "plan/operators/abstract_op.h"

namespace fishstore::plan::op {
    class Print : public AbstractOp {
    public:
        inline void Initialize(ThreadId thread_count) override {
            if (parent != nullptr)
                parent->Initialize(thread_count);
        }

        inline void Touch(ThreadId id, record::Record *rec) override {
            const auto &schema = GetSchema();
            if (first_rec) {
                std::cout << schema.toString() << std::endl;
                first_rec = false;
            }
            std::cout << rec->toString(schema) << std::endl;
        }

        inline void Finalize(ThreadId id) override {
            if (parent != nullptr) {
                parent->Finalize(id);
            }
        }

        inline void FinalizeAll() override {
            if (parent != nullptr)
                parent->FinalizeAll();
        }

        [[nodiscard]] inline const Schema &GetSchema() const override {
            return child->GetSchema();
        }

        inline void LinkParent(AbstractOp *op) override {
            parent = op;
        }

        inline void LinkChild(AbstractOp *op) override {
            child = op;
        }

        void PrettyPrint() override { printf("[Print Operator]\n"); }

    private:
        AbstractOp *child = nullptr;
        AbstractOp *parent = nullptr;
        bool first_rec = true;
    };
}
