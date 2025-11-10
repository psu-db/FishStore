// List of abstract classes to extend
//
// Created by Max Norfolk on 7/24/23.


#pragma once

#include <typeindex>

#include "core/async_result_types.h"
#include "jit/datatypes/types.h"

namespace fishstore::ezpsf::ast {
    class ExprAst {
    public:
        ExprAst(const std::type_index& type_info, size_t hashcode)
            : type_info_(type_info),
              hashcode_(hashcode * 31 + type_info_.hash_code()) {}

        virtual ~ExprAst() = default;

        virtual llvm::Value* code() = 0;

        virtual DataType type() = 0;

        [[nodiscard]] constexpr size_t hash() const { return hashcode_; }

        [[nodiscard]] constexpr bool operator==(const ExprAst& other) const {
            return hashcode_ == other.hashcode_
                   && type_info_ == other.type_info_
                   && equalHelper(&other);
        }

        static constexpr size_t combineHash(const std::initializer_list<size_t>& hashes) {
            size_t ret = 17;
            for (const auto hash: hashes) {
                ret = ret * 31 + hash;
            }
            return ret;
        }



    protected:
        /// It is guaranteed that ExprAst* will NOT be nullptr!
        [[nodiscard]] virtual bool equalHelper(const ExprAst*) const = 0;

    private:
        std::type_index type_info_;
        size_t hashcode_;
    };
}
