// Updated version of data frames, to be more pandas-like
//
// Created by Max Norfolk 2/26/24.

#pragma once

#include <utility>

#include "plan/data_frame/data_frame_adapter.h"
#include "plan/data_frame/data_frame_utils.h"

namespace fishstore::plan::data_frame {
    template<typename D, typename A>
    class DataLoader;

    template<typename D, typename A>
    class DataFrame {
        template<typename D2, typename A2>
        friend
        class DataLoader;

    public:
        class TableScanHelper;

        class GroupByHelper;

        template<typename V>
        class PsfGroupByHelper;

    public:
        // Do not call this constructor, it will be called by the TableScanHelper class!
        explicit DataFrame(DataLoader<D, A>& data) : data(data), index_maker({}) {
            auto expected_jit = llvm::orc::LLJITBuilder().create();
            ASSERT(expected_jit && "JIT failed to be created!");
            jit = std::move(expected_jit.get());
            ezpsf::initJit<psudb::DynamicArray<DfValue>>(jit.get());
        }


        DataFrame* Project(const std::vector<std::string>& fields);

        std::unique_ptr<GroupByHelper> GroupBy(const std::vector<std::string>& group_by_fields);

        DataFrame* GroupBy(PsfId, DataLoader<D, A>&);

        DataFrame* Rename(const std::vector<std::string>& fields);

        DataFrame* SortBy(const std::vector<std::string>& fields);

        DataFrame* Limit(size_t limit);

        DataFrame* Filter(const std::vector<std::string>& fields);

        void Execute(op::RecordCallback::CallbackFunction callback);

        void Print();

        std::string ToString();

        DataFrame(const DataFrame&) = delete;

        virtual ~DataFrame() {
            for (const auto& item: all_ops)
                delete item;
            for (const auto& item: unlinked_op)
                delete item;
        }

        int Count() {
            int cnt = 0;
            this->Execute([&cnt](record::Record*) { ++cnt; });
            return cnt;
        }

    private:
        DataLoader<D, A>& data;

        // Jit Stuff
        std::unique_ptr<llvm::orc::LLJIT> jit;

    public:
        ezpsf::index_gen::MapIndexGenerator<utils::Map> index_maker;
        op::AbstractStartOp* start_op;

        DataFrame* PrintPlan() {
            uint32_t indent = 0;
            for (auto it = all_ops.rbegin(); it != all_ops.rend(); ++it) {
                (*it)->PrettyPrint(indent++);
            }
            fflush(stdout);
            return this;
        }

    private:
        // Operators
        std::vector<op::AbstractOp *> all_ops;
        std::vector<op::AbstractOp *> unlinked_op;

        void linkOperators() {
            for (int i = 0; i < all_ops.size() - 1; ++i) {
                all_ops[i]->LinkParent(all_ops[i + 1]);
                all_ops[i + 1]->LinkChild(all_ops[i]);
            }
        }

        void SetStartScan(op::AbstractStartOp* op);

        // creates a function that maps the record to another record
        std::pair<Schema, std::function<record::Record(record::Record*)>>
        createMapFunction(const std::vector<std::string>& fields);

        plan::Filter createFilterFunction(const std::vector<std::string>& fields);
    };

    template<typename D, typename A>
    std::ostream& operator<<(std::ostream& out, DataFrame<D, A>* df) {
        out << df->ToString();
        return out;
    }
};
