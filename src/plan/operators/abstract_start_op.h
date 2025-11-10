// A class that has a start method to start the query
//
// Created by Max Norfolk on 3/26/24.
//

#pragma once

#include "plan/operators/abstract_op.h"

namespace fishstore::plan::op {
    class AbstractStartOp : public AbstractOp {
    public:
        // Call StartScan to start the entire query.
        // it will automatically setup and initialize all threads and recursively touch each
        // record for each level
        // TODO setup threads
        virtual void StartScan() = 0;
    };
}
