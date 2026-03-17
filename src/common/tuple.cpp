#include "common/tuple.h"

namespace db::common {
Tuple::Tuple(std::vector<Value> values,
             std::shared_ptr<const Schema> schema,
             std::optional<access::RID> rid)
    : _values{std::move(values)},
      _schema{std::move(schema)},
      _rid{rid} {}

std::vector<Value> Tuple::GetValues() const { 
  return _values; 
}

std::shared_ptr<const Schema> Tuple::GetSchema() const { 
  return _schema; 
}

std::optional<access::RID> Tuple::GetRID() const { 
  return _rid; 
}
}