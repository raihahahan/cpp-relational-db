#include "common/tuple.h"
#include <functional>

namespace db::executor {
using Tuple = common::Tuple;
struct Predicate {
    explicit Predicate(std::function<bool(Tuple tup)> func);
    bool Evaluate(Tuple tup);
};
}