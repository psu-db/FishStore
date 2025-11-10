#include <utility>

// Defines the way the psf will choose an index for a given field_name
// This is with respect to how the PSF's record argument works.
// For example, given record [1, 20, "joe"], and PSF "(Int) age * 2 + (Int) id",
// we need some way to map the record's elements to the correct PSF variables.
//
// The original FishStore implementation was to give variables increasing indices,
// and requiring the record to be structured according to the Psf.
//
// So if all records are structured as {id, age, name}, then our example record [1, 20, "joe"]
// would need to be converted to [20, 1] before being passed to the PSF (joe may be optionally there
// there's no negative or positive either way).
//
// This file provides a way to map the PSF variables to an index based upon the name of the variable
// so in our previous example, if we used our MapIndexGenerator, we can instead define a mapping from
// id -> 0, age -> 1, name -> 2, so even though age is defined first, it will still end up with index 1
// even if id was not used.
//
// Created by Max Norfolk on 1/13/24.


#pragma once
namespace fishstore::ezpsf::index_gen {

    // the original index generator method. (each argument needs to be the order they are encountered)
    class CounterIndexGenerator {
    public:
        uint64_t mapFieldId(const std::string &field_name) {
            return counter++;
        }

    private:
        uint64_t counter = 0;
    };

    // Map must have the [] operator defined which maps the string to uint64_t.
    template<typename Map>
    class MapIndexGenerator {
    public:
        explicit MapIndexGenerator(Map map) : map(std::move(map)) {}

        uint64_t mapFieldId(const std::string &field_name) {
            if (map.count(field_name) == 0) {
                std::cout << "[index_generator.h] failed to find name: '" << field_name << "'.\n";
            }
            return map.at(field_name);
        }

    private:
        Map map;
    };

    template<typename Map>
    class SavedCounterGenerator {
    public:
        SavedCounterGenerator() : map() {}

        explicit SavedCounterGenerator(Map map) : map(std::move(map)) {}

        // if field name is present, reuse the existing id
        // if field name isn't present, give it a new index,
        // counting up from 0.
        uint64_t mapFieldId(const std::string &field_name) {
            auto pair = map.try_emplace(field_name, counter);
            if (pair.second) { // if inserted, increment counter
                counter++;
            }
            return pair.first->second; // return the iterator's value
        }

        // if field name is present, reuse the existing id
        // if field name isn't present, throw std::out_of_range
        uint64_t findId(const std::string &field_name) {
            return map.at(field_name);
        }

    private:
        Map map;
        uint64_t counter = 0;
    };
}