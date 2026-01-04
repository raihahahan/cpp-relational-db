#pragma once

#include <memory>
#include <vector>
#include <unordered_set>
#include "catalog/catalog_types.h"
#include "common/value.h"
#include "access/record.h"


namespace db::common {
using Schema = std::vector<db::catalog::ColumnInfo>;

class Tuple {
public:
    Tuple(
        std::vector<Value> values,
        std::shared_ptr<const Schema> schema
    );

    std::vector<Value> GetValues() const;
    std::shared_ptr<const Schema> GetSchema() const;

private:
    std::vector<Value> _values;
    std::shared_ptr<const Schema> _schema;
};
}