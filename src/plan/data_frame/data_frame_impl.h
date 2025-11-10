// Definition of Data Frame methods
//
// Created by Max Norfolk on 2/26/24.


#pragma once

#include <utility>

#include "plan/data_frame/data_frame.h"
#include "plan/data_frame/data_loader.h"
#include "plan/data_frame/data_frame_helpers.h"

namespace fishstore::plan::data_frame {
    ///////////////////////////////////////////
    // Data Frame
    ///////////////////////////////////////////

    template<typename D, typename A>
    void DataFrame<D, A>::SetStartScan(op::AbstractStartOp* op) {
        all_ops.emplace_back(op);
        start_op = op;
    }

    template<typename D, typename A>
    std::unique_ptr<typename DataFrame<D, A>::GroupByHelper>
    DataFrame<D, A>::GroupBy(const std::vector<std::string>& group_by_fields) {
        return std::make_unique<GroupByHelper>(*this, group_by_fields);
    }

    template<typename D, typename A>
    DataFrame<D, A>* DataFrame<D, A>::SortBy(const std::vector<std::string>& fields) {
        psudb::DynamicArray<ezpsf::EzPsf> sort_funcs{fields.size()};
        psudb::DynamicArray<ezpsf::DataType> ret_types{fields.size()};
        for (int i = 0; i < fields.size(); ++i) {
            const auto& field = fields[i];
            auto psf = ezpsf::getPsf(field, jit.get(), index_maker);
            sort_funcs[i] = psf.psf;
            ret_types[i] = psf.type;
        }

        op::InMemorySort::CompareFunction comp = [sort_funcs, ret_types](record::Record& a, record::Record& b) -> bool {
            for (int i = 0; i < sort_funcs.size(); ++i) {
                switch (ret_types[i]) {
                    case ezpsf::DataType::INT32_T: {
                        int res_a = 0;
                        int res_b = 0;
                        const bool has_value_a = sort_funcs[i](&a, &res_a);
                        const bool has_value_b = sort_funcs[i](&b, &res_b);
                        assert(has_value_a && has_value_b && "Missing Value for Int");
                        if (res_a == res_b)
                            continue;
                        return res_a < res_b;
                    }
                    case ezpsf::DataType::STR_T: {
                        adapter::StringRef value_a{};
                        adapter::StringRef value_b{};
                        const bool has_value_a = sort_funcs[i](&a, &value_a);
                        const bool has_value_b = sort_funcs[i](&b, &value_b);
                        assert(has_value_a && has_value_b && "Missing Value for Str");
                        size_t num_to_cmp = std::min(value_a.Length(), value_b.Length());
                        auto result = std::strncmp(value_a.Data(), value_b.Data(), num_to_cmp);
                        if (result == 0)
                            continue;
                        return result < 0; // a < b
                    }
                    default:
                        assert(false && "Unsupported datatype");
                }
            }
            return false; // a == b
        };
        all_ops.emplace_back(new op::InMemorySort{comp});
        return this;
    }

    template<typename D, typename A>
    void DataFrame<D, A>::Execute(op::RecordCallback::CallbackFunction callback) {
        all_ops.emplace_back(new op::RecordCallback{std::move(callback)});
        linkOperators();
        start_op->StartScan();
    }

    template<typename D, typename A>
    void DataFrame<D, A>::Print() {
        all_ops.emplace_back(new op::Print{});
        linkOperators();

        uint32_t indent = 0;
        for (auto it = all_ops.rbegin(); it != all_ops.rend(); ++it) {
            (*it)->PrettyPrint(indent++);
        }
        fflush(stdout);
        #ifdef TIME_QUERY
        TIMER_INIT();
        TIMER_START();
        start_op->StartScan();
        TIMER_STOP();
        printf("[Query took %ld nanoseconds]\n",TIMER_RESULT());
        fflush(stdout);
        #else
        start_op->StartScan();
        fflush(stdout);
        #endif
    }

    template<typename D, typename A>
    DataFrame<D, A>* DataFrame<D, A>::Project(const std::vector<std::string>& fields) {
        auto pair = createMapFunction(fields);
        auto ptr = new op::Projection{std::move(pair.first), std::move(pair.second)};
        all_ops.emplace_back(ptr);
        return this;
    }

    template<typename D, typename A>
    DataFrame<D, A>* DataFrame<D, A>::Rename(const std::vector<std::string>& fields) {
        index_maker = ezpsf::index_gen::MapIndexGenerator{utils::mapFromContainer(fields)};
        return this;
    }

    template<typename D, typename A>
    DataFrame<D, A>* DataFrame<D, A>::Limit(size_t limit) {
        auto ptr = new op::Limit(limit);
        all_ops.emplace_back(ptr);
        return this;
    }

    template<typename D, typename A>
    DataFrame<D, A>* DataFrame<D, A>::Filter(const std::vector<std::string>& fields) {
        auto ptr = new op::FilterOp(createFilterFunction(fields));
        all_ops.emplace_back(ptr);
        return this;
    }

    template<typename D, typename A>
    std::pair<Schema, std::function<record::Record(record::Record*)>>
    DataFrame<D, A>::createMapFunction(const std::vector<std::string>& fields) {
        psudb::DynamicArray<ezpsf::EzPsf> functions{fields.size()};
        psudb::DynamicArray<ezpsf::DataType> return_types{fields.size()};
        psudb::DynamicArray<Schema::FieldInfo> schema_args{fields.size()};

        for (int i = 0; i < fields.size(); ++i) {
            const auto& field = fields[i];
            const auto psf = ezpsf::getPsf(field, jit.get(), index_maker);
            functions[i] = psf.psf;
            return_types[i] = psf.type;
            schema_args[i] = Schema::FieldInfo(field, psf.type);
        }

        return {
            Schema{schema_args}, [functions, return_types](record::Record* rec) -> record::Record {
                record::Record record{functions.size()};
                for (int i = 0; i < functions.size(); ++i) {
                    switch (return_types[i]) {
                        case ezpsf::DataType::INT32_T: {
                            int value = 0;
                            const bool has_value = functions[i](rec, &value);
                            assert(has_value && "data_frame_impl.h -- has value for INT32_T");
                            record[i] = record::Value{value};
                            break;
                        }
                        case ezpsf::DataType::STR_T: {
                            adapter::StringRef value{};
                            const bool has_value = functions[i](rec, &value);
                            assert(has_value && "data_frame_impl.h -- has value for STR_T");
                            std::shared_ptr<char[]> shared{new char[value.Length()]};

                            memcpy(shared.get(), value.Data(), value.Length());
                            record.addBuffer(shared);
                            record[i] = record::Value(adapter::StringRef{shared.get(), value.Length()});
                            break;
                        }
                        default:
                            assert(false && "unsupported data type!");
                    }
                }
                return record;
            }
        };
    }

    template<typename D, typename A>
    plan::Filter DataFrame<D, A>::createFilterFunction(const std::vector<std::string>& clauses) {
        psudb::DynamicArray<Filter::CnfClause> cnf_clauses{clauses.size()};
        for (int i = 0; i < clauses.size(); ++i) {
            const auto& clause = clauses[i];
            const auto psf_info = ezpsf::getPsf(clause, jit.get(), index_maker);
            const auto psf_func = psf_info.psf;

            // pass psf_info by value -- should be ok to use even out of scope
            plan::Filter::FilterFunction filter_func = [psf_func](record::Record* rec) -> bool {
                int32_t res = 0;
                bool has_value = psf_func(rec, &res);

                if (has_value)
                    return res;
                return false;
            };
            if (ezpsf::canInline(psf_info.type))
                cnf_clauses[i] = plan::Filter::CnfClause{data.GetPsfId(clause), filter_func, 1};
            else
                cnf_clauses[i] = plan::Filter::CnfClause{data.GetPsfId(clause), filter_func, adapter::StringRef{}};

        }
        return plan::Filter{cnf_clauses};
    }
}
