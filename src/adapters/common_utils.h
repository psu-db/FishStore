// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license.

#pragma once

#include <cstdint>

namespace fishstore::adapter {
    class StringRef {
    public:
        StringRef() : ptr(nullptr), size(0) {}

        StringRef(const char* ptr_, size_t size_) : ptr(ptr_), size(size_) {}

        [[nodiscard]] constexpr const char* Data() const {
            return ptr;
        }

        [[nodiscard]] constexpr size_t Length() const {
            return size;
        }

        explicit constexpr operator std::string_view() const { return {ptr, size}; }

        explicit operator std::string() const { return {ptr, size}; }

    private:
        const char* ptr;
        size_t size;
    };

    template<typename T>
    struct Nullable {
        Nullable() : has_value(false), value() {}

        Nullable(const T& value_) : has_value(true), value(value_) {}

        // Constructs a Nullable, which may have a value, and if it does, then
        // it will have the given value
        Nullable(const bool has_value_, const T& value_) : has_value(has_value_), value(value_) {}

        [[nodiscard]] bool HasValue() const {
            return has_value;
        }

        const T& Value() {
            return value;
        }

    private:
        bool has_value;
        T value;
    };

    using NullableInt = Nullable<int32_t>;
    using NullableLong = Nullable<int64_t>;
    using NullableFloat = Nullable<float>;
    using NullableDouble = Nullable<double>;
    using NullableBool = Nullable<bool>;
    using NullableStringRef = Nullable<StringRef>;
    using NullableString = Nullable<std::string>;
}

std::ostream& operator<<(std::ostream& out, const fishstore::adapter::StringRef& str) {
    out << std::string_view(str.Data(), str.Length());
    return out;
}
