// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <cstdint>
#include <cassert>
#include <utility>
#include <vector>

#ifdef _MSC_VER
#define NOMINMAX
#endif

#include <simdjson.h>
#include "adapters/common_utils.h"

constexpr size_t DEFAULT_BATCH_SIZE = 1 << 24;


namespace fishstore::adapter {
    using namespace simdjson;

    // Represents a SimdJson field
    class SIMDJsonField {
    public:
        // constructs a SimdJsonField with a given simdjson value
        SIMDJsonField(int64_t id_, const simdjson_result<ondemand::value>& value_)
            : field_id(id_), simd_value(value_) {}

        inline int64_t FieldId() const {
            return field_id;
        }

        inline NullableBool GetAsBool() const {
            bool val;
            bool has_value = (simd_value.get(val) == error_code::SUCCESS);
            return {has_value, val};
        }

        inline NullableInt GetAsInt() const {
            int64_t val;
            bool has_value = (simd_value.get(val) == error_code::SUCCESS);
            return {has_value, static_cast<int32_t>(val)};
        }

        inline NullableLong GetAsLong() const {
            int64_t val;
            bool has_value = (simd_value.get(val) == error_code::SUCCESS);
            return {has_value, val};
        }

        inline NullableFloat GetAsFloat() const {
            double val;
            bool has_value = (simd_value.get(val) == error_code::SUCCESS);
            return {has_value, static_cast<float>(val)};
        }

        inline NullableDouble GetAsDouble() const {
            double val;
            bool has_value = (simd_value.get(val) == error_code::SUCCESS);
            return {has_value, val};
        }

        inline NullableString GetAsString() const {
            // the simdjson get method only supports std::string_view, so we must
            // turn it into a std::string after
            std::string_view temp{};
            bool has_value = (simd_value.get(temp) == error_code::SUCCESS);
            return {has_value, std::string(temp)};
        }

        inline NullableStringRef GetAsStringRef() const {
            std::string_view temp{};
            bool has_value = (simd_value.get(temp) == error_code::SUCCESS);
            StringRef str_ref{temp.data(), temp.length()};
            return {has_value, str_ref};
        }

    private:
        int64_t field_id;
        mutable simdjson_result<ondemand::value> simd_value;
    };

    // represents the type of field that will be looked up (object or array)
    enum class SIMDJsonFieldType {
        OBJECT,
        ARRAY
    };

    // represents the type of field that will be looked up and
    // the name or index that should be looked up
    // this is stored in a vector in SIMDJsonFieldLookup
    struct SIMDJsonFieldLookupElement {
        explicit SIMDJsonFieldLookupElement(const uint32_t start, const uint32_t len)
            : type(SIMDJsonFieldType::OBJECT), str_start(start), str_len(len) {}

        explicit SIMDJsonFieldLookupElement(const size_t array_index)
            : type(SIMDJsonFieldType::ARRAY), array_index(array_index) {}

        SIMDJsonFieldType type;

        union {
            uint64_t array_index;

            struct {
                uint32_t str_start;
                uint32_t str_len;
            };
        };

        std::string_view getStringView(const char* ptr) const { return std::string_view{ptr + str_start, str_len}; }
    };

    // helper class used to look up simdjson fields
    class SIMDJsonFieldLookup {
    public:
        // creates a simdjson field from a string
        SIMDJsonFieldLookup() = default;

        explicit SIMDJsonFieldLookup(std::string lookup_str) : buffer(std::move(lookup_str)) {
            const char* start = buffer.data();
            char* end = buffer.data();
            while (true) {
                if (*end == '.' || *end == '\0') {
                    // end of  a field
                    const uint32_t str_len = end - start;
                    if (str_len == 0) {
                        break;
                    }

                    SIMDJsonFieldLookupElement item{static_cast<uint32_t>(start - buffer.data()), str_len};
                    lookups.push_back(item);

                    if (*end == '\0') {
                        break;
                    }

                    start = end + 1;
                } else if (*start == '[') {
                    // strtol will move the end pointer right after the last number, so to the ']'.
                    const size_t index = std::strtol(++start, &end, 10);
                    SIMDJsonFieldLookupElement item{index};
                    lookups.push_back(item);
                    start = end + 1;
                }
                end++;
            }
        }

        [[nodiscard]] simdjson_result<ondemand::value> find(ondemand::object source) const {
            auto fields_it = lookups.begin();
            const char* ptr = buffer.c_str();

            // always starts with an object
            auto ret = source.find_field_unordered(fields_it->getStringView(ptr));
            ++fields_it;
            // iterate through the entire lookups
            while (fields_it != lookups.end()) {
                // if object, or array find the correct value
                if (fields_it->type == SIMDJsonFieldType::OBJECT) {
                    ret = ret.find_field_unordered(fields_it->getStringView(ptr));
                } else if (fields_it->type == SIMDJsonFieldType::ARRAY) {
                    ret = ret.at(fields_it->array_index);
                }
                ++fields_it;
            }

            return ret;
        }

        [[nodiscard]] bool empty() const { return lookups.empty(); }

    private:
        std::vector<SIMDJsonFieldLookupElement> lookups;
        std::string buffer;
    };

    class SIMDJsonRecord {
    public:
        friend class SIMDJsonParser;

        SIMDJsonRecord() = default;

        SIMDJsonRecord(ondemand::document_reference doc, const std::vector<SIMDJsonFieldLookup>& lookups) {
            obj = doc.get_object();

            auto ref = obj.raw_json().value();
            raw_text = {ref.data(), ref.length()};
            obj.reset();


            int i = 0;
            for (const auto& lookup: lookups) {
                const auto value = lookup.find(obj);
                // check the value was found if not, don't add to vector
                if (value.error() == simdjson::SUCCESS) {
                    fields.emplace_back(i, value);
                }
                ++i;
            }
        }

        inline const std::vector<SIMDJsonField>& GetFields() const {
            return fields;
        }

        inline StringRef GetRawText() const {
            return raw_text;
        }

    public:
        mutable ondemand::object obj;
        StringRef raw_text;
        std::vector<SIMDJsonField> fields;
    };

    class SIMDJsonParser {
    public:
        SIMDJsonParser(const std::vector<std::string>& field_names) {
            field_lookups.reserve(field_names.size());
            for (auto& item: field_names) {
                field_lookups.emplace_back(item);
            }
        }

        SIMDJsonParser(const SIMDJsonParser&) = delete;

        inline void Load(const char* buffer, size_t length) {
            if (parser.iterate_many(buffer, length, DEFAULT_BATCH_SIZE).get(docs) != simdjson::SUCCESS)
                return;
            docs_it = docs.begin();
        }

        inline bool HasNext() {
            return docs_it != docs.end();
        }

        inline const SIMDJsonRecord& NextRecord() {
            assert(docs_it != docs.end());
            record = SIMDJsonRecord(*docs_it, field_lookups);
            ++docs_it;
            return record;
        }

    private:
        std::vector<SIMDJsonFieldLookup> field_lookups;

        // keep these around for memory safety reasons
        ondemand::parser parser;
        ondemand::document_stream docs;
        ondemand::document_stream::iterator docs_it;

        SIMDJsonRecord record;
    };

    class SIMDJsonOnDemandParser {
    public:
        typedef SIMDJsonFieldLookup lookup_t;

        inline void Load(const char* buffer, size_t length) {
            doc = parser.iterate(buffer, length, length + SIMDJSON_PADDING);
        }

        SIMDJsonField findField(const std::string_view& key) {
            return {-1, doc.find_field_unordered(key)};
        }

        SIMDJsonField findField(const lookup_t& lookup) {
            return {-1, lookup.find(doc)};
        }

        void setRealTime(const std::string& field) {
            real_time = lookup_t{field};
        }

        std::string findRealTime() {
            if (real_time.empty())
                return "";
            auto val = findField(real_time).GetAsString();
            assert(val.HasValue());
            return val.Value();
        }


        lookup_t real_time{};

        ondemand::parser parser{};
        ondemand::document doc;
    };

    class SIMDJsonAdapter {
    public:
        typedef SIMDJsonOnDemandParser ondemand_parser_t;
        typedef SIMDJsonParser parser_t;
        typedef SIMDJsonField field_t;
        typedef SIMDJsonRecord record_t;

        inline static ondemand_parser_t NewOnDemandParser() {
            return ondemand_parser_t{};
        }

        inline static parser_t* NewParser(const std::vector<std::string>& fields) {
            return new parser_t{fields};
        }

        inline static void Load(parser_t* const parser, const char* payload, size_t length, size_t offset = 0) {
            assert(offset <= length);
            parser->Load(payload + offset, length - offset);
        }

        inline static bool HasNext(parser_t* const parser) {
            return parser->HasNext();
        }

        inline static const record_t& NextRecord(parser_t* const parser) {
            return parser->NextRecord();
        }
    };
}
