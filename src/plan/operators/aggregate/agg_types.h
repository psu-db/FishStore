// Enum for the different aggregate types supported (unfinished at the moment)
//
// Created by Max Norfolk on 2/27/24.

#pragma once

namespace fishstore::plan::op::aggregate {
    enum class AggregateType {
        COUNT,
        SUM,
        AVG,
        MIN,
        MAX,
        ERROR_AGG
    };
}