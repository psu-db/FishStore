// An operator that will call a user passed function for each record.
//
// Created by Max Norfolk on 1/22/24.


#pragma once

#include <utility>

#include "plan/operators/abstract_op.h"

namespace fishstore::plan::op {
    class RecordCallback : public AbstractOp {
    public:
        // maps a record* to a new record
        // if you drop any string fields, you must manage the memory
        // by doing delete[] on the string ref pointer
        typedef std::function<void(record::Record *)> CallbackFunction;

        RecordCallback() : callback() {
        }

        explicit RecordCallback(CallbackFunction func) : callback(std::move(func)) {
        }

        void Touch(ThreadId id, record::Record *record) override {
            callback(record);
            if (parent != nullptr)
                parent->Touch(id, record);
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
            return child->GetSchema();
        }

        void LinkParent(AbstractOp *op) override { parent = op; }

        void LinkChild(AbstractOp *op) override { child = op; }

        void PrettyPrint() override { printf("[RecordCallback]"); }

    private:
        AbstractOp *parent = nullptr;
        AbstractOp *child = nullptr;
        const CallbackFunction callback;
    };
}
