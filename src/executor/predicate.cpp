#include "executor/predicate.h"
namespace db::executor {
Predicate::Predicate(std::function<bool(Tuple tup)> func) : _func{func} {};
bool Predicate::Evaluate(Tuple tup) {
    return _func(tup);
}
}