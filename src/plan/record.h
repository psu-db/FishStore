// Record
//
// Created by Max Norfolk on 1/11/24.


#pragma once
namespace fishstore::plan::record {
    union Value {
        fishstore::adapter::StringRef string;
        int32_t int32;
        double dbl;
        float flt;

        Value() : string({nullptr, 0}) {}

        explicit Value(const fishstore::adapter::StringRef& string) : string(string) {}

        explicit Value(const char* ptr, size_t size) : string(adapter::StringRef(ptr, size)) {}

        explicit Value(const int32_t int32) : int32(int32) {}
        explicit Value(const double dbl) : dbl(dbl) {}
        explicit Value(const float flt) : flt(flt) {}

        explicit Value(const uint32_t int32) : int32(static_cast<int32_t>(int32)) {}
    };

    // TODO make record its own class with better methods
    class Record : public psudb::DynamicArray<Value> {
    public:
        Record() = default;

        explicit Record(const size_t& size)
            : DynamicArray<Value>(size) {}

        explicit Record(const std::initializer_list<Value>& init)
            : DynamicArray<Value>(init) {}

        explicit Record(const std::vector<Value>& copy_vector)
            : DynamicArray<Value>(copy_vector) {}

        explicit Record(const DynamicArray<Value>& orig)
            : DynamicArray<Value>(orig) {}


        [[nodiscard]] std::string toString(const Schema& schema) const {
            std::string ret;
            auto it = schema.begin();
            while (it.next()) {
                switch (it->type) {
                    case ezpsf::DataType::INT32_T:
                        ret += std::to_string(this->at(it.getCurrentIndex()).int32) + ", ";
                        break;
                    case ezpsf::DataType::STR_T:
                        ret += std::string_view(this->at(it.getCurrentIndex()).string);
                        ret += ", ";
                        break;
                    case ezpsf::DataType::DOUBLE_T:
                        ret += std::to_string(this->at(it.getCurrentIndex()).dbl);
                        ret += ", ";
                        break;
                    default:
                        assert(false && "Unsupported operation");
                }
            }
            return ret.substr(0, ret.length() - 2);
        }

        [[nodiscard]] static Record merge(const Record& a, const Record& b) {
            Record ret{a.size() + b.size()};
            int i = 0;
            for (const auto& value: a)
                ret[i++] = value;
            for (const auto& value: b)
                ret[i++] = value;
            for (const auto& ptr: a.buffers)
                ret.buffers.emplace_back(ptr);
            for (const auto& ptr: b.buffers)
                ret.buffers.emplace_back(ptr);
            return ret;
        }

        void clearBuffers() { buffers.clear(); }

        void addBuffer(const std::shared_ptr<char[]>& new_buf) {
            buffers.emplace_back(new_buf);
        }

    private:
        std::vector<std::shared_ptr<char[]>> buffers;
    };


    /////////////////////////////////////////////////////
    // Classes defined for using custom hash & equal functions
    // IMPORTANT: These classes do NOT make a copy of the schema
    // therefore, the schema must be saved somewhere before
    /////////////////////////////////////////////////////
    // custom equal function based upon the schema
    struct RecordEqual {
        explicit RecordEqual(const Schema* const record_schema) : record_schema(*record_schema) {}

        bool operator()(const record::Record& rec1, const record::Record& rec2) const {
            auto it = record_schema.begin();
            while (it.next()) {
                auto index = it.getCurrentIndex();
                auto type = it->type;
                if (type == ezpsf::DataType::INT32_T) {
                    if (rec1[index].int32 != rec2[index].int32)
                        return false;
                } else {
                    if (ezpsf::str_info::FUNC_STR_CMP(rec1[index].string, rec2[index].string) != 0) {
                        return false;
                    }
                }
            }
            return true;
        }

    private:
        const Schema& record_schema;
    };

    // custom hash based upon the schema
    struct HashRecord {
        explicit HashRecord(const Schema* const record_schema) : record_schema(*record_schema) {}

        std::size_t operator()(const record::Record& rec) const {
            // Hash based upon the Clion generated one defined in /src/plan/types.h
            std::size_t seed = 0x27CB2BF7;

            auto it = record_schema.begin();
            while (it.next()) {
                const auto value = rec[it.getCurrentIndex()];


                if (it->type == ezpsf::DataType::INT32_T) {
                    seed ^= (seed << 6) + (seed >> 2) + 0x275FC473 + core::Utility::GetHashCode(value.int32);
                } else {
                    seed ^= (seed << 6) + (seed >> 2) + 0x510C6FF4 + ezpsf::str_info::FUNC_STR_HASH(value.string);
                }
            }
            return seed;
        }

    private:
        const Schema& record_schema;
    };
}
