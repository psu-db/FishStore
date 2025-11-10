// A filter consists of a number of clauses and-ed (conjunction-ed) together.
// for example, x > 0 && y != "abc"
// A clause may also be an "or" of two things, e.g. (a > 0 || b != "abc"). This must be
// implemented as a single clause.
//
// These clauses may optionally be associated with a PsfId, if there is a Psf built on the clause
//
// Created by Max Norfolk on 12/12/23.


#pragma once

#include <functional>
#include <utility>
#include <tsl/hopscotch_set.h>

#include "plan/types.h"
#include "plan/record_helper.h"

namespace fishstore::plan {
    class Filter {
    public:
        typedef std::function<bool(record::Record*)> FilterFunction;

        struct CnfClause {
            CnfClause() : id(NO_PSF), func(nullptr) {}

            explicit CnfClause(FilterFunction func) : id(NO_PSF), func(std::move(func)) {}

            explicit CnfClause(PsfId id, FilterFunction func, adapter::StringRef s_val)
                : id(id), func(std::move(func)), s_val(s_val) {}

            explicit CnfClause(PsfId id, FilterFunction func, int32_t i_val)
                : id(id), func(std::move(func)), i_val(i_val) {}

            core::PsfLookup id;

            union {
                adapter::StringRef s_val;
                int32_t i_val;
            };

            FilterFunction func;
        };

        // creates a filter that accepts everything
        Filter() = default;

        explicit Filter(const psudb::DynamicArray<CnfClause>& clauses) : clauses(clauses) {}

        // checks the record given the PSF that has already checked it
        [[nodiscard]] inline bool checkKpts(const PsfId id, record::Record& record,
                                            const tsl::hopscotch_map<PsfId, record::Value>& kpts_map) const {
            assert(id != FS_ID &&
                "use check(record::Record*) instead of check(PsfId, record::Record*) when doing a FullScan!");


            for (const auto& clause: clauses) {
                // if id matches, skip checking because we know this record passes the PSF check already
                if (id == clause.id)
                    continue;

                auto it = kpts_map.find(clause.id);
                if (it != kpts_map.end()) {
                    if (clause.id.isInline()) {
                        if (it->second.int32 != clause.i_val)
                            // if PSF maps to other value, then this clause maps to false
                            return false;
                    } else {
                        if (ezpsf::str_info::FUNC_STR_CMP(it->second.string, clause.s_val) != 0) // not equal
                            return false;
                    }

                    continue; // this CNF clause evaluated to true
                }


                // if any of the Cnf clauses are false, return false as conjunction
                if (!clause.func(&record))
                    return false;
            }
            return true;
        }

        // checks the record without being a subfilter from a psf
        [[nodiscard]] inline bool checkKpts(record::Record& record,
                                            const tsl::hopscotch_map<PsfId, record::Value>& kpts_map) const {
            for (const auto& clause: clauses) {
                const auto it = kpts_map.find(clause.id);

                if (it != kpts_map.end()) {
                    if (clause.id.isInline()) {
                        if (it->second.int32 != clause.i_val)
                            // if PSF maps to other value, then this clause maps to false
                            return false;
                    } else {
                        if (ezpsf::str_info::FUNC_STR_CMP(it->second.string, clause.s_val) != 0) // not equal
                            return false;
                    }

                    continue; // this CNF clause evaluated to true
                }


                // if any of the Cnf clauses are false, return false as conjunction
                if (!clause.func(&record)) {
                    return false;
                }
            }
            return true;
        }
        

        // checks the record without being a subfilter from a psf
        [[nodiscard]] inline bool check(record::Record& record) const {
            for (const auto& clause: clauses) {
                // if any of the Cnf clauses are false, return false as conjunction
                if (!clause.func(&record)) {
                    return false;
                }
            }
            return true;
        }

        // Constructs a set of the PsfIds that can be used by this Filter. This is defined based upon the CnfClauses
        [[nodiscard]] tsl::hopscotch_set<PsfId> usedIds() const {
            tsl::hopscotch_set<PsfId> set(clauses.size());
            for (const auto& clause: clauses) {
                set.emplace(clause.id);
            }
            set.erase(NO_PSF);
            return set;
        }

        [[nodiscard]] auto begin() const { return clauses.begin(); }
        [[nodiscard]] auto end() const { return clauses.end(); }

        psudb::DynamicArray<CnfClause>& unsafe() { return clauses; }

    private:
        psudb::DynamicArray<CnfClause> clauses;
    };
}
