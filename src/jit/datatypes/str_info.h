// String type operations
//
// Created by Max Norfolk on 6/3/23.


#pragma once

#include <cctype>
#include <algorithm>

#include "jit/datatypes/conversion_declaration.h"
#include "core/utility.h"

namespace fishstore::ezpsf::str_info {
    // negative if s1 < s2. 0 if s1 == s2. positive if s1 > s2.
    int FUNC_STR_CMP(StringRef s1, StringRef s2) {
        const uint64_t num_to_cmp = std::min(s1.Length(), s2.Length());
        const auto result = std::strncmp(s1.Data(), s2.Data(), num_to_cmp);

        // if they are equal, verify the size is equal. Otherwise, return based upon the size, such that the smaller
        // is smaller than larger
        if (result == 0)
            return static_cast<int64_t>(s1.Length()) - static_cast<int64_t>(s2.Length());
        return result;
    }

    // returns the size as an uint64_t. This is unused as it can be accessed by the struct
    inline uint64_t FUNC_STR_LEN(StringRef s) {
        assert(false && "This should be accessed through the struct!");
        return s.Length();
    }


    static inline uint64_t FUNC_STR_HASH(StringRef s) {
        // 40343 is a "magic constant" that works well,
        // 38299 is another good value.
        // Both are primes and have a good distribution of bits.
        constexpr uint64_t kMagicNum = 40343;
        uint64_t hashState = s.Length();

        for (size_t idx = 0; idx < s.Length(); ++idx) {
            hashState = kMagicNum * hashState + s.Data()[idx];
        }

        // The final scrambling helps with short keys that vary only on the high order bits.
        // Low order bits are not always well distributed so shift them to the high end, where they'll
        // form part of the 14-bit tag.
        return core::Utility::Rotr64(kMagicNum * hashState, 6);
    }

    constexpr inline bool ichar_equal(const char a, const char b) {
        return std::tolower(a) == std::tolower(b);
    }

    constexpr inline bool char_equal(const char a, const char b) {
        return a == b;
    }

    /// Returns true if it was able to find s2 within s1.
    /// Code consists of 2 bits for determining the type, and 1 bit for determining if it is case sensitive.
    /// The type is the least significant 2 bits, and case is the 3rd least significant
    /// if type == 0, then it must be at the start (abc%),
    /// if it is 1, then it must be at the end (%abc),
    /// if it is 2 then it may be any where (%abc%)
    /// if the case insensitive bit is set, then it will be a case insensitive
    static inline bool FUNC_STR_REGEX(StringRef s1, StringRef s2, int code) {
        fflush(stdout);
        if (s1.Length() < s2.Length())
            return false;

        int type = 0b11 & code;
        bool is_insensitive = code & 0b100;
        auto cmp = is_insensitive ? ichar_equal : char_equal;

        if (type == 0) {
            const auto t1 = std::string_view{s1.Data(), s2.Length()};
            const auto t2 = std::string_view{s2};
            return std::equal(t1.begin(), t1.end(), t2.begin(), t2.end(), cmp);
        }

        if (type == 1) {
            const auto t1 = std::string_view{s1.Data() + s1.Length() - s2.Length(), s2.Length()};
            const auto t2 = std::string_view{s2};
            return std::equal(t1.begin(), t1.end(), t2.begin(), t2.end(), cmp);
        }

        if (type == 2) {
            const auto t1 = std::string_view{s1};
            const auto t2 = std::string_view{s2};
            return std::search(t1.begin(), t1.end(), t2.begin(), t2.end(), cmp) != t1.end();
        }
        assert("Illegal State!");
        return false;
    }


    llvm::StringRef REF_STR_CMP = "@__STRING_INFO_CMP";
    llvm::StringRef REF_STR_LEN = "@__STRING_INFO_LEN";
    llvm::StringRef REF_STR_REGEX = "@__STRING_INFO_REGEX";
}
