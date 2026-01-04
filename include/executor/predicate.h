#pragma once
#include "common/tuple.h"
#include <functional>

namespace db::executor {
using Tuple = common::Tuple;
struct Predicate {
public:
    explicit Predicate(std::function<bool(Tuple tup)> func);
    bool Evaluate(Tuple tup);

private:
    std::function<bool(Tuple tup)> _func;
};
}