// Creates an iterator that follows the std iterator requirements
// for a range, similar to pythons range() function
//
// Create by Max Norfolk on 2/5/24

#pragma once

#include <cstdlib>

namespace psudb {
    template<size_t N = 1>
    class Range {
    private:
        class Iterator {
        public:
            Iterator(std::size_t v) : value(v) {}

            Iterator &operator++() {
                value += N;
                return *this;
            }

            Iterator operator++(int) {
                std::size_t old = value;
                value += N;
                return Iterator(old);
            }

            std::size_t operator-(const Iterator &other) const;

            std::size_t operator*() { return value; }

            // the iterator operator != is defined as <, because if we did Range<2>(0,3), we would get
            // 0, 2, 4, ... and never stop!
            std::size_t operator!=(const Iterator &other) const { return value < other.value; };

        private:
            std::size_t value;
        };

    public:

        // creates a range from [start, end) -- remember end is not inclusive
        Range(size_t start, size_t end) : start(start), end_(end) {}

        [[nodiscard]] Iterator begin() const {
            return {start};
        }

        [[nodiscard]] Iterator end() const { return {end_}; }

    private:
        std::size_t start;
        std::size_t end_;
    };

    // N == 1
    template<>
    std::size_t Range<1>::Iterator::operator-(const Range::Iterator &other) const {
        return this->value - other.value;
    }

    // N != 1
    template<size_t N>
    std::size_t Range<N>::Iterator::operator-(const Range::Iterator &other) const {
        const std::size_t dist = this->value - other.value;

        // 3 - 0 with N = 2, would be 2, because 0+=2 = 2, and 2+=2 = 4, which is >= 3!
        return dist / N + (dist % N != 0);
    }
}