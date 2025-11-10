// A simple header that contains the required record/field accessing functions that can be linked into the JIT system
// to access fields from a given record.
//
// Created by Max Norfolk on 7/7/23.

#pragma once

#include <cstdint>
#include "adapters/common_utils.h"

namespace fishstore::ezpsf {
    typedef void EzRecord;

    using StringRef = adapter::StringRef;

    namespace type_conversion {
        typedef adapter::NullableBool NullableBool;
        typedef adapter::NullableInt NullableInt32;
        typedef adapter::NullableLong NullableInt64;
        typedef adapter::NullableDouble NullableDouble;
        typedef adapter::NullableStringRef NullableStringRef;

        // record_t should have the at() method defined, and additionally
        // the returned value should have the GetAsX() method, which returns
        // nullable that has a Value() and HasValue() method
        template<typename record_t>
        class TypeConversion {
        public:
            static void getBool(EzRecord *record, uint64_t field_identifier, NullableBool *return_ptr) {
                auto rec = static_cast<record_t *>(record);
                auto &field = rec->at(field_identifier);
                auto val = field.GetAsBool();
                *return_ptr = {val.HasValue(), val.Value()};
            }

            static void getInt32(EzRecord *record, uint64_t field_identifier, NullableInt32 *return_ptr) {
                auto rec = static_cast<record_t *>(record);
                auto &field = rec->at(field_identifier);
                auto val = field.GetAsInt();
                *return_ptr = {val.HasValue(), val.Value()};
            }

            static void getInt64(EzRecord *record, uint64_t field_identifier, NullableInt64 *return_ptr) {
                auto rec = static_cast<record_t *>(record);
                auto &field = rec->at(field_identifier);
                auto val = field.GetAsLong();
                *return_ptr = {val.HasValue(), val.Value()};
            }

            static void getDouble(EzRecord *record, uint64_t field_identifier, NullableDouble *return_ptr) {
                auto rec = static_cast<record_t *>(record);
                auto &field = rec->at(field_identifier);
                auto val = field.GetAsDouble();
                *return_ptr = {val.HasValue(), val.Value()};
            }

            static void getStringRef(EzRecord *record, uint64_t field_identifier, NullableStringRef *return_ptr) {
                auto rec = static_cast<record_t *>(record);
                auto &field = rec->at(field_identifier);
                auto val = field.GetAsStringRef();
                StringRef ref = {val.Value().Data(), val.Value().Length()};
                *return_ptr = {val.HasValue(), ref};
            }
        };
    }
}
