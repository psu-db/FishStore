#pragma once

#include <cstdint>
#include <cassert>
#include <utility>
#include <vector>

#ifdef _MSC_VER
#define NOMINMAX
#endif

#include <adapters/csv.hpp>
#include "adapters/common_utils.h"


namespace fishstore::adapter {
    using namespace csv;

    // Represents a SimdJson field
    class CppCSVField {
    public:
        // constructs a SimdJsonField with a given simdjson value
        CppCSVField(int64_t id_, CSVField value_)
            : field_id(id_) {
            is_null = value_.is_null();
            is_float = value_.is_float();
            is_int = value_.is_int();
            is_str = value_.is_str();
            if (is_float) {
                float_val = value_.get<double>();
            } else if (is_int) {
                int_val = value_.get<long>();
            } else {
                str_val = value_.get_sv();
            }
        }

        CppCSVField() : field_id(-1), is_null(true) {}

        inline int64_t FieldId() const {
            return field_id;
        }

        inline NullableBool GetAsBool() const {
            return {!is_null && is_int, int_val == 0 ? false : true};
        }

        inline NullableInt GetAsInt() const {
            return {!is_null && is_int, static_cast<int>(int_val)};
        }

        inline NullableLong GetAsLong() const {
            return {!is_null && is_int, static_cast<long>(int_val)};
        }

        inline NullableFloat GetAsFloat() const {
            return {!is_null && is_float, static_cast<float>(float_val)};
        }

        inline NullableDouble GetAsDouble() const {
            return {!is_null && is_float, static_cast<double>(float_val)};
        }

        inline NullableString GetAsString() const {
            return {!is_null && is_str, std::string(str_val)};
        }

        inline NullableStringRef GetAsStringRef() const {
            //auto view = value.get_sv();
            return {!is_null && is_str, StringRef{str_val.data(), str_val.length()}};
        }

    private:
        int64_t field_id;

        union {
            size_t int_val;
            double float_val;
        };

        std::string_view str_val;

        bool is_null;
        bool is_int;
        bool is_float;
        bool is_str;
    };

    template<size_t field_cnt>
    class CppCSVRecord {
    public:
        CppCSVRecord() = default;

        CppCSVRecord(const CSVRow& row, const std::vector<size_t> field_nos) {
            // auto ptr = row[0].get_sv();
            // auto last_field = row[row.size() - 1].get_sv();
            // printf("%p vs %p\n", ptr.data(), last_field.data());
            // assert(ptr.data() <= last_field.data() && "Corrupted Record");

            // raw_rec = StringRef{ptr.data(), last_field.data() - ptr.data() + last_field.size()};

raw_rec = StringRef{row.raw_line().data(), row.raw_line().size()};
            
            for (size_t i = 0; i < field_nos.size(); ++i) {
                if (field_nos[i] < row.size())
                    fields.emplace_back(i, row[field_nos[i]]);
            }
        }

        inline const std::vector<CppCSVField>& GetFields() const {
            return fields;
        }

        inline StringRef GetRawText() const {
            return raw_rec;
        }

    public:
        std::vector<CppCSVField> fields;
        StringRef raw_rec;
    };

    template<size_t field_cnt>
    class CppCSVParser {
    public:
        CppCSVParser(const std::vector<std::string>& fields) {
            for (auto& field: fields) {
                field_nos.emplace_back(std::stoi(field));
            }
        }


        inline void Load(const char* buffer, size_t length) {
            CSVFormat format;
            format.header_row(-1);
            std::stringstream stream{std::string{buffer, length}};
            reader = std::make_unique<CSVReader>(stream, format);
            it = reader->begin();
        }

        inline bool HasNext() {
            return it != reader->end();
        }

        inline const CppCSVRecord<field_cnt>& NextRecord() {
            assert(it != reader->end());
            record = CppCSVRecord<field_cnt>(*it, field_nos);
            ++it;
            return record;
        }

    private:
        std::unique_ptr<CSVReader> reader; // unique ptr to handle calling load multiple times
        CSVReader::iterator it;
        CppCSVRecord<field_cnt> record;

        std::vector<size_t> field_nos;
    };

    template<size_t field_cnt>
    class CppOnDemandCSVParser {
    public:
        typedef struct lookup_t {
            lookup_t() : field_num(-1) {}

            lookup_t(const std::string& ref) : field_num(std::atol(ref.c_str())) {}

            size_t field_num;
        } lookup_t;

        inline void Load(const char* buffer, size_t length) {
            CSVFormat format;
            format.header_row(-1);
            std::stringstream stream(std::string_view(buffer, length).data());
            reader = std::make_unique<CSVReader>(stream, format);
            it = reader->begin();
        }


        CppCSVField findField(const lookup_t& lookup) {
            auto row = *it;
            if (lookup.field_num < row.size())
                return CppCSVField{-1, row[lookup.field_num]};
            return CppCSVField{};
        }

        void setRealTime(const std::string& field) {
            real_time = lookup_t{field};
        }

        std::string findRealTime() {
            if (real_time.field_num == -1)
                return "";
            auto val = findField(real_time).GetAsString();
            assert(val.HasValue());
            return val.Value();
        }

    private:
        lookup_t real_time;

        std::unique_ptr<CSVReader> reader;
        CSVReader::iterator it;
    };

    template<size_t field_cnt>
    class CppCSVAdapter {
    public:
        typedef CppCSVParser<field_cnt> parser_t;
        typedef CppCSVField field_t;
        typedef CppCSVRecord<field_cnt> record_t;
        typedef CppOnDemandCSVParser<field_cnt> ondemand_parser_t;

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
