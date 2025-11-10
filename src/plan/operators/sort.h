// A simple sorting operator that will just put everything into a vector and call std::sort.
//
// Created by Max Norfolk on 12/29/23.


#pragma once

#include <utility>

#include "abstract_op.h"

namespace fishstore::plan::op {
    class InMemorySort : public AbstractOp {
    public:
        // comparison function that states if a < b.
        typedef std::function<bool(record::Record &a, record::Record &b)> CompareFunction;

        InMemorySort() : comp() {}

        explicit InMemorySort(CompareFunction comp) : comp(std::move(comp)), records(), final_results() {}

        void Initialize(ThreadId thread_count) override {
            records = psudb::DynamicArray<std::vector<record::Record>>{thread_count};
        }

        void Touch(ThreadId id, record::Record *rec) override {
            if (rec == nullptr)
                assert(false);
            final_results.emplace_back(*rec);
//            records[id].emplace_back(*rec);
        }

        // sort subquery results
        void Finalize(ThreadId id) override {
//            std::sort(records[id].begin(), records[id].end(), comp);
        }

        // TODO should be k-way merge
        void FinalizeAll() override {
            // todo multi way merge
            std::sort(final_results.begin(), final_results.end(), comp);
            if (parent != nullptr) {
                ThreadId thread_id = 0;
                // TODO make multithreading
                parent->Initialize(thread_id);
                for (auto &record: final_results) {
                    parent->Touch(thread_id, &record);
                }
                parent->Finalize(thread_id);
                parent->FinalizeAll();
            }
        }

        [[nodiscard]] const Schema &GetSchema() const override {
            return child->GetSchema();
        }

        std::vector<record::Record> &GetRecords() { return final_results; }

        void LinkParent(AbstractOp *op) override {
            parent = op;
        }

        void LinkChild(AbstractOp *op) override {
            child = op;
        }

        void PrettyPrint() override{printf("[Sort]\n");}

    private:
        AbstractOp *child = nullptr;
        AbstractOp *parent = nullptr;
        psudb::DynamicArray<std::vector<record::Record>> records;
        std::vector<record::Record> final_results;
        CompareFunction comp;
    };
}