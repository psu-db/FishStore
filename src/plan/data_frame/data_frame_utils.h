// Small helper functions used for data frames
//
// Created by Max Norfolk on 2/26/24.

#pragma once

#include <string>
#include <vector>
#include "tsl/hopscotch_map.h"

namespace fishstore::plan::data_frame::utils {
    using Map = tsl::hopscotch_map<std::string, uint32_t>;

    template<typename ContainerOfStrings> //
    Map mapFromContainer(const ContainerOfStrings &c) {
        Map m{c.size()};

        for (int i = 0; i < c.size(); ++i) {
            m.emplace(c[i], i);
        }

        return m;
    }
}