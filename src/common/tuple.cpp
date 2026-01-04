#include "common/tuple.h"

namespace db::common {
Tuple::Tuple(
        std::vector<Value> values,
        std::shared_ptr<const Schema> schema
    )
        : _values{std::move(values)},
          _schema{std::move(schema)} {};

std::vector<Value> Tuple::GetValues() const {
    return _values;
};

}