// plan specific record which is just an array of C++ values, rather than
// FishStore specific record.
//
// Created by Max Norfolk on 12/13/23.


#pragma once

#include "core/record.h"
#include "plan/types.h"
#include "plan/record.h"
#include "adapters/simdjson_adapter.h"

namespace fishstore::plan::record {
    typedef fishstore::core::Record FishStoreRecord;
    typedef fishstore::ezpsf::DataType DataType;

    // field extract is used to actually extract fields from a record adhering to a specific schema
    // This class is a template, so it can actually parse payloads, whereas schema just stores basic information
    // about the field names, and data types.
    template<typename A>
    class FieldExtract {
    public:
        explicit FieldExtract(const Schema& schema_) : schema(schema_),
                                                       parser(A::NewOnDemandParser()),
                                                       lookups(schema.size()) {
            auto it = schema.begin();
            while (it.next()) {
                auto idx = it.getCurrentIndex();
                lookups[idx] = typename A::ondemand_parser_t::lookup_t{it->name};
            }
        }

        // explicit copy constructor because we need to create a new parser
        FieldExtract(const FieldExtract& copy_me)
            : schema(copy_me.schema), parser(A::NewOnDemandParser()), lookups(copy_me.lookups) {}

        // returns the record when we extract the fields from the fishstore record
        [[nodiscard]] inline record::Record apply(const FishStoreRecord* record) {
            record::Record ret(schema.size());
            apply(record, ret);
            return ret;
        }

        // modifies the passed record. assumes the record is the correct size
        // returns a map containing a set of PsfId's and Values with the PSF, which will contain all key pointers for
        // more efficient filtering
        // also returns a bool which if true means the record was successfully defined for the schema
        // if false, then it wasn't, and doesn't contain all values
        inline std::pair<tsl::hopscotch_map<PsfId, record::Value>, bool> apply(const FishStoreRecord* record,
                                                                               record::Record& inplace_rec) {
            assert(inplace_rec.size() == schema.size());
            inplace_rec.clearBuffers();

            auto schema_it = schema.psf_begin();

            // key pointers
            const auto PTR_CNT = record->header.ptr_cnt;
            tsl::hopscotch_map<PsfId, record::Value> kpts_map(PTR_CNT);

            for (int i = 0; i < PTR_CNT; ++i) {
                auto kp = record->get_ptr(i);
                if (kp->mode == GENERAL) {
                    kpts_map.emplace(PsfId::General(kp->general_psf_id), Value{kp->get_value(), kp->value_size});
                } else {
                    kpts_map.emplace(PsfId::Inline(kp->inline_psf_id), Value{kp->value});
                }

                // schema extra
                if (kp->mode == GENERAL && schema_it.next(PsfId::General(kp->general_psf_id))) {
                    auto index = schema_it.getCurrentIndex();
                    if (schema_it->type == DataType::STR_T) {
                        std::shared_ptr<char[]> shared{new char[kp->value_size]};

                        memcpy(shared.get(), kp->get_value(), kp->value_size);
                        inplace_rec.addBuffer(shared);
                        inplace_rec[index] = Value(shared.get(), kp->value_size);
                    }
                    else if (schema_it->type == DataType::DOUBLE_T) {
                        double temp = *reinterpret_cast<const double *>(kp->get_value());
                        inplace_rec[index] = Value(temp);
                    }
                } else if (schema_it.next(PsfId::Inline(kp->inline_psf_id))) {
                    assert(kp->mode == INLINE && "mode should be inline!");

                    auto index = schema_it.getCurrentIndex();
                    inplace_rec[index] = Value(kp->value);
                }
            }

            // dynamic parser
            parser.Load(record->payload(), record->payload_size());
            while (schema_it.next()) {
                const uint32_t index = schema_it.getCurrentIndex();
                auto field = parser.findField(lookups.at(index));

                const auto type = schema_it->type;
                if (type != DataType::STR_T) {
                    if (type == DataType::INT32_T) {
                        auto n = field.GetAsInt();
                        if (!n.HasValue())
                            return {{}, false};


                        inplace_rec[index] = Value(n.Value());
                    } else if (type == DataType::DOUBLE_T) {
                        auto n = field.GetAsDouble();
                        if (!n.HasValue())
                            return {{}, false};

                        inplace_rec[index] = Value(n.Value());
                    } else if (type == DataType::FLOAT_T) {
                        auto n = field.GetAsFloat();
                        if (!n.HasValue())
                            return {{}, false};

                        printf("Found float: %f", n.Value());
                        inplace_rec[index] = Value(n.Value());
                    } else
                        assert(false && "unsupported type!");
                } else {
                    auto ref = field.GetAsStringRef();
                    if (!ref.HasValue())
                        return {{}, false};

                    adapter::StringRef val = ref.Value();
                    std::shared_ptr<char[]> shared{new char[val.Length()]};

                    memcpy(shared.get(), val.Data(), val.Length());
                    inplace_rec.addBuffer(shared);
                    inplace_rec[index] = Value(adapter::StringRef{shared.get(), val.Length()});
                }
            }

            return {kpts_map, true};
        }

        const Schema& schema;

        
    private:
        static constexpr uint32_t GENERAL = 0;
        static constexpr uint32_t INLINE = 1;

        typename A::ondemand_parser_t parser;
        psudb::DynamicArray<typename A::ondemand_parser_t::lookup_t> lookups;
    };
}
