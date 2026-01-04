#pragma once
#include <optional>
#include "common/tuple.h"

namespace db::executor {
using Tuple = db::common::Tuple;

class Operator {
public: 
    virtual void Open() = 0;
    virtual std::optional<Tuple> Next() = 0;
    virtual void Close() = 0;
    virtual ~Operator() = default;
};
}