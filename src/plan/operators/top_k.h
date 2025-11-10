//
// Created by mxn5437 on 9/28/25.
//

#pragma once
#include "abstract_op.h"
#include "sort.h"

namespace fishstore::plan::op {
    class TopK : public AbstractOp {
    public:
        explicit TopK(InMemorySort::CompareFunction comp, size_t k)
            : k(k), queue(TopKComp{this->comp}), comp(std::move(comp)) {
        }

        void Initialize(ThreadId thread_count) override {
        }

        void Touch(ThreadId id, record::Record *rec) override {
            assert(rec != nullptr);

            if (queue.size() < k) {
                queue.push(*rec);
            } else if (record::Record top = queue.top(); comp(top, *rec)) {
                // top < rec
                queue.pop();
                queue.push(*rec);
            }
        }

        // sort subquery results
        void Finalize(ThreadId id) override {
        }

        // TODO should be k-way merge
        void FinalizeAll() override {
            if (parent != nullptr) {
                ThreadId thread_id = 0;
                // TODO make multithreading
                parent->Initialize(thread_id);
                while (!queue.empty()){
                    record::Record top = queue.top();
                    parent->Touch(thread_id, &top);
                    queue.pop();
                }
                parent->Finalize(thread_id);
                parent->FinalizeAll();
            }
        }

        [[nodiscard]] const Schema &GetSchema() const override {
            return child->GetSchema();
        }

        void LinkParent(AbstractOp *op) override {
            parent = op;
        }

        void LinkChild(AbstractOp *op) override {
            child = op;
        }

        void PrettyPrint() override { printf("[Sort]\n"); }

    private:
        AbstractOp *child = nullptr;
        AbstractOp *parent = nullptr;

        struct TopKComp {
            explicit TopKComp(const InMemorySort::CompareFunction &comp) : comp(comp) {
            }

            const InMemorySort::CompareFunction &comp;

            bool operator()( record::Record &r1, record::Record &r2) const {
                return !comp(r1, r2);
            }
        };

        size_t k;

        std::priority_queue<record::Record, std::vector<record::Record>, TopKComp> queue;

        InMemorySort::CompareFunction comp;
    };
}
