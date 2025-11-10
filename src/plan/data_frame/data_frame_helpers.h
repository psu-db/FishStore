// Helpers for the data frame
//
// Created by Max Norfolk 2/26/2024

#pragma once

#include "plan/data_frame/data_frame_aggregation.h"

#include "plan/operators/table_scans.h"

namespace fishstore::plan::data_frame {
    struct FilterStringInfo {
        FilterStringInfo(std::string named_psf, std::string value, int clause_idx)
            : named_psf(std::move(named_psf)),
              value(std::move(value)),
              clause_idx(clause_idx) {}

        std::string named_psf;
        std::string value;
        int clause_idx;
    };

    struct FilterIntInfo {
        FilterIntInfo(std::string named_psf, int value, int clause_idx)
            : named_psf(std::move(named_psf)),
              value(value),
              clause_idx(clause_idx) {
        }
        std::string named_psf;
        int value;
        int clause_idx;
    };

    template<typename D, typename A>
    class DataFrame<D, A>::TableScanHelper {
    public:
        explicit TableScanHelper(DataLoader<D, A>& data, const std::vector<std::string>& fields,
                                 const std::vector<std::string>& origin)
            : data(data) {
            df = std::make_unique<DataFrame<D, A>>(data);

            psudb::DynamicArray<Schema::FieldInfo> schema_arr{fields.size()};
            psudb::DynamicArray<std::string> field_names{fields.size()};

            for (int i = 0; i < fields.size(); ++i) {
                const auto& field = fields[i];
                const auto info = ezpsf::getPsf(field, df->jit.get());

                std::string parsed_field = info.fields[0];
                field_names[i] = parsed_field;
                if (origin.size() > i)
                    parsed_field = origin[i];
                schema_arr[i] = Schema::FieldInfo(parsed_field, info.field_types[0], data.GetPsfId(field));
            }

            ts_schema = Schema{schema_arr};
            df->index_maker = ezpsf::index_gen::MapIndexGenerator{utils::mapFromContainer(field_names)};
        }

        std::unique_ptr<DataFrame> Filter(std::vector<std::string> clauses, bool force = false,
                                          PsfId filter_psf_id = NO_PSF, int value = -1) {
            if (!addr_filter.empty())
                clauses.emplace_back(addr_filter);
            auto filter = df->createFilterFunction(clauses);

            if (force && filter_psf_id != NO_PSF) {
                filter.unsafe().at(0).id = filter_psf_id;
                filter.unsafe().at(0).i_val = value;
            }

            auto ptr = new op::TableScan<D, A>{
                *data.fishstore.get(), data.psf_map, ts_schema, filter, lower_addr,
                upper_addr
            };


            df->SetStartScan(ptr);
            return std::move(df);
        }




        std::unique_ptr<DataFrame> FilterUsePsf(std::vector<std::string> clauses, const std::vector<FilterIntInfo>& infos) {
            if (!addr_filter.empty())
                clauses.emplace_back(addr_filter);
            auto filter = df->createFilterFunction(clauses);

            for (auto && info : infos) {
                filter.unsafe().at(info.clause_idx).id = data.GetPsfId(info.named_psf);
                filter.unsafe().at(info.clause_idx).i_val = info.value;
            }


            auto ptr = new op::TableScan<D, A>{
                *data.fishstore.get(), data.psf_map, ts_schema, filter, lower_addr,
                upper_addr
            };


            df->SetStartScan(ptr);
            return std::move(df);
        }


        std::unique_ptr<DataFrame> FilterPlan(const std::vector<std::string>& clauses, std::vector<op::ScanInfo> plan) {
            auto filter = df->createFilterFunction(clauses);


            auto ptr = new op::TableScan<D, A>{
                *data.fishstore.get(), data.psf_map, ts_schema, filter, lower_addr,
                upper_addr, false, plan
            };


            df->SetStartScan(ptr);
            return std::move(df);
        }

        std::unique_ptr<DataFrame> FilterString(std::vector<std::string> clauses, PsfId filter_psf_id,
                                                const std::string& value) {
            if (!addr_filter.empty())
                clauses.emplace_back(addr_filter);
            auto filter = df->createFilterFunction(clauses);

            if (filter_psf_id != NO_PSF) {
                filter.unsafe().at(0).id = filter_psf_id;
                auto temp = new std::string(value);
                filter.unsafe().at(0).s_val = {temp->c_str(), temp->length()};
            }

            auto ptr = new op::TableScan<D, A>{
                *data.fishstore.get(), data.psf_map, ts_schema, filter, lower_addr,
                upper_addr
            };


            df->SetStartScan(ptr);
            return std::move(df);
        }


        std::unique_ptr<DataFrame>
        FilterStrings(std::vector<std::string> clauses, std::vector<FilterStringInfo> infos) {
            if (!addr_filter.empty())
                clauses.emplace_back(addr_filter);
            auto filter = df->createFilterFunction(clauses);

            for (auto&& info: infos) {
                filter.unsafe().at(info.clause_idx).id = data.GetPsfId(info.named_psf);
                auto temp = new std::string(info.value);
                filter.unsafe().at(info.clause_idx).s_val = {temp->c_str(), temp->length()};
            }

            auto ptr = new op::TableScan<D, A>{
                *data.fishstore.get(), data.psf_map, ts_schema, filter, lower_addr,
                upper_addr
            };


            df->SetStartScan(ptr);
            return std::move(df);
        }


        std::unique_ptr<DataFrame>
        FilterStringsTraditional(std::vector<std::string> clauses, std::vector<FilterStringInfo> infos) {
            if (!addr_filter.empty())
                clauses.emplace_back(addr_filter);
            auto filter = df->createFilterFunction(clauses);

            for (auto&& info: infos) {
                filter.unsafe().at(info.clause_idx).id = data.named_psfs.at(info.named_psf);
                auto temp = new std::string(info.value);
                filter.unsafe().at(info.clause_idx).s_val = {temp->c_str(), temp->length()};
            }

            std::vector plan = {op::ScanInfo{PsfInfo{FS_ID, lower_addr, upper_addr}, 0}};
            for (auto&& clause: filter.unsafe()) {
                auto psf_info = data.psf_map.find(clause.id);
                if (psf_info == data.psf_map.end())
                    continue;
                if (psf_info->second.start <= lower_addr && psf_info->second.end >= upper_addr) {
                    if (psf_info->first.isInline())
                        plan = {op::ScanInfo{PsfInfo{psf_info->first, lower_addr, upper_addr}, clause.i_val}};
                    else
                        plan = {op::ScanInfo{PsfInfo{psf_info->first, lower_addr, upper_addr}, clause.s_val}};
                }
            }

            auto ptr = new op::TableScan<D, A>{
                *data.fishstore.get(), data.psf_map, ts_schema, filter, lower_addr,
                upper_addr, false, plan
            };


            df->SetStartScan(ptr);
            return std::move(df);
        }

        TableScanHelper* From(PsfAddress lower, PsfAddress upper = fishstore::core::Address::kMaxAddress) {
            lower_addr = lower;
            upper_addr = upper;
            return this;
        }

        // gets sliding window backwards
        TableScanHelper* FromEnd(size_t offset_from_end) {
            lower_addr = data.getTailAddress() - offset_from_end;
            if (lower_addr > data.getTailAddress())
                lower_addr = data.getHeadAddress();
            upper_addr = core::Address::kMaxAddress;
            return this;
        }

        /// Does a scan from lower (inclusive) to upper (exclusive)
        TableScanHelper* From(const std::string& lower, const std::string& upper, const std::string& ts) {
            auto start = data.fishstore->findTime(lower);
            auto end = data.fishstore->findTime(upper);

            lower_addr = start.first.control();
            upper_addr = core::Address{end.first.page() + 1, 0}.control() - 1;
            printf("lower_addr: %ld, upper_addr: %ld\n", lower_addr, upper_addr);
            addr_filter = "(Str) " + ts + " >= '" + lower + "' && (Str) " + ts + " < '" + upper + "'";
            return this;
        }

    private:
        Schema ts_schema{};
        PsfAddress lower_addr = 0;
        PsfAddress upper_addr = fishstore::core::Address::kMaxAddress;

        std::string addr_filter;

        std::unique_ptr<DataFrame<D, A>> df;
        DataLoader<D, A>& data;
    };

    // for in memory group by
    template<typename D, typename A>
    class DataFrame<D, A>::GroupByHelper {
    public:
        GroupByHelper(DataFrame<D, A>& df, const std::vector<std::string>& group_by) : df(df) {
            auto pair = df.createMapFunction(group_by);
            schema = std::move(pair.first);
            group_by_func = std::move(pair.second);
        }

        /**
         * See data_frame_aggregation.h for AggregatePair and how they should be constructed
         */
        DataFrame<D, A>* Aggregate(const std::vector<aggregate::AggregatePair>& vec) {
            auto ptr = new op::InMemoryGroupBy{
                std::move(schema),
                std::move(group_by_func),
                aggregate::makeAggregateManager(df, vec),
            };

            df.all_ops.emplace_back(ptr);

            return &df;
        }

    private:
        DataFrame<D, A>& df;
        Schema schema;
        op::InMemoryGroupBy::GroupByFunction group_by_func;
    };


    // for psf group by
    template<typename D, typename A>
    template<typename V>
    class DataFrame<D, A>::PsfGroupByHelper {
        static_assert(std::is_same_v<V, int> || std::is_same_v<V, adapter::StringRef>,
                      "V must be int or adapter::StringRef");

    public:
        explicit PsfGroupByHelper(DataLoader<D, A>& data, const std::string& ez_psf_group,
                                  const std::vector<std::string>& fields)
            : data(data) {
            df = std::make_unique<DataFrame<D, A>>(data);

            // handle the ez_psf group
            const ezpsf::EzPsfInfo group_psf_info = ezpsf::getPsf(ez_psf_group, df->jit.get());

            psf_id = data.GetPsfId(ez_psf_group);


            size_t schema_size = fields.size() + group_psf_info.fields.size();
            psudb::DynamicArray<Schema::FieldInfo> schema_arr{schema_size};
            psudb::DynamicArray<std::string> field_names{schema_size};

            size_t arr_offset = 0;
            for (; arr_offset < group_psf_info.fields.size(); ++arr_offset) {
                const auto field_name = group_psf_info.fields[arr_offset];
                field_names[arr_offset] = field_name;
                schema_arr[arr_offset] = Schema::FieldInfo{
                    field_name, group_psf_info.field_types[arr_offset], data.GetPsfId(field_name)
                };
            }


            for (const auto& field: fields) {
                const auto info = ezpsf::getPsf(field, df->jit.get());

                const auto& parsed_field = info.fields[0];
                field_names[arr_offset] = parsed_field;
                schema_arr[arr_offset] = Schema::FieldInfo{
                    parsed_field, info.field_types[0], data.GetPsfId(parsed_field)
                };
                ++arr_offset;
            }

            all_fields_schema = Schema{schema_arr};
            std::cout << "schema: " << all_fields_schema.toString() << std::endl;
            df->index_maker = ezpsf::index_gen::MapIndexGenerator{utils::mapFromContainer(field_names)};
        }

        PsfGroupByHelper* Filter(const std::vector<std::string>& clauses) {
            filter = df->createFilterFunction(clauses);
            return this;
        }

        PsfGroupByHelper* Groups(const psudb::DynamicArray<V>& possible_values_) {
            possible_values = possible_values_; // copy & move
            return this;
        }

        PsfGroupByHelper* From(PsfAddress lower, PsfAddress upper = fishstore::core::Address::kMaxAddress) {
            lower_addr = lower;
            upper_addr = upper;
            return this;
        }

        std::unique_ptr<DataFrame> Aggregate(const std::vector<aggregate::AggregatePair>& aggs) {
            auto manager = aggregate::makeAggregateManager(*df.get(), aggs);
            PsfInfo psf_info = df->data.GetPsfInfo(psf_id);
            auto union_op = new op::UnionStart{};

            auto& store = *df->data.fishstore.get();

            // we need a TableScan for start
            if (lower_addr < psf_info.start) {
                const auto ptr = new op::TableScan<D, A>{
                    store, df->data.psf_map, all_fields_schema, filter,
                    lower_addr, psf_info.start - 1
                };
                union_op->LinkChild(ptr);
                ptr->LinkParent(union_op);
                df->unlinked_op.emplace_back(ptr);
            }

            // we need a TableScan for end
            if (upper_addr > psf_info.end) {
                const auto ptr = new op::TableScan<D, A>{
                    store, df->data.psf_map, all_fields_schema, filter,
                    psf_info.end + 1, upper_addr
                };
                union_op->LinkChild(ptr);
                ptr->LinkParent(union_op);
                df->unlinked_op.emplace_back(ptr);
            }

            auto psf_group_by = new op::PsfScanGroupBy<D, A, V>{
                store, psf_id, all_fields_schema, manager.New(),
                possible_values, filter, lower_addr, upper_addr
            };
            union_op->LinkChild(psf_group_by);
            psf_group_by->LinkParent(union_op);
            df->unlinked_op.emplace_back(psf_group_by);
            df->SetStartScan(union_op);
            return std::move(df);
        }

    private:
        Schema all_fields_schema{};
        plan::Filter filter{};
        PsfAddress lower_addr = 0;
        PsfAddress upper_addr = fishstore::core::Address::kMaxAddress;

        PsfId psf_id = NO_PSF;
        psudb::DynamicArray<V> possible_values;


        std::unique_ptr<DataFrame<D, A>> df;
        DataLoader<D, A>& data;
    };
};
