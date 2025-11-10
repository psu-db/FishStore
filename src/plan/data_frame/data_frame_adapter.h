// Adapter for ezpsf stuff, but defined on the record::Record for dataframes
//
// Created by Max Norfolk on 1/13/24.


#pragma once

#include <cassert>
#include <string>

#include "plan/record.h"
#include "adapters/common_utils.h"
#include "jit/datatypes/conversion_declaration.h"

namespace fishstore::plan::data_frame {
    class DfValue {
    public:
        [[nodiscard]] adapter::NullableBool GetAsBool() const {
            return {static_cast<bool>(value.int32)};
        }

        [[nodiscard]] adapter::NullableInt GetAsInt() const {
            return {value.int32};
        }

        [[nodiscard]] adapter::NullableLong GetAsLong() const {
            assert(false && "Df::Value::GetAsLong() not implemented");
            return {true, 0};
        }

        [[nodiscard]] ezpsf::type_conversion::NullableDouble GetAsDouble() const {
            return {value.dbl};
        }

        [[nodiscard]] adapter::NullableStringRef GetAsStringRef() const {
            return {value.string};
        }

        record::Value value;
    };
}