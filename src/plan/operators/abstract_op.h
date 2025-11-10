// An abstract class for operators.
//
// Created by Max Norfolk on 12/29/23.

#pragma once

#include "plan/record_helper.h"

namespace fishstore::plan::op {
    typedef uint32_t ThreadId;

    inline void printIndent(const uint32_t indent) {
        for (int i = 0; i < indent; ++i) {
            printf("  ");
        }
    }

    class AbstractOp {
    public:
        virtual ~AbstractOp() = default;

        // does any initialization code necessary for to setup handling thread_count number of threads
        inline virtual void Initialize(ThreadId thread_count) = 0;

        inline virtual void Touch(ThreadId, record::Record *) = 0;

        // finalizes a specific thread
        // after finalize is called, it is undefined behavior to call touch with the same
        // thread id
        inline virtual void Finalize(ThreadId) = 0;

        // finalizes all thread
        inline virtual void FinalizeAll() = 0;

        [[nodiscard]] inline virtual const Schema &GetSchema() const = 0;

        // sets the parent operator for this operator. may be nullptr if this the uppermost operator
        inline virtual void LinkParent(AbstractOp *) = 0;

        // sets the child operator for this operator. may be nullptr
        // if this is the lowest operator in the tree
        inline virtual void LinkChild(AbstractOp *) = 0;

        inline virtual void PrettyPrint() = 0;

        inline virtual void PrettyPrint(const uint32_t indent) {
            printIndent(indent);
            this->PrettyPrint();
        };
    };
}
