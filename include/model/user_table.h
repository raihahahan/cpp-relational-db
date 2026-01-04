#pragma once

#include "model/relation.h"
#include "catalog/catalog_types.h"
#include "access/heap/heap_file.h"
#include <vector>
#include <optional>

using ColumnInfo = db::catalog::ColumnInfo;
using HeapFile = db::access::HeapFile;
using table_id_t = db::catalog::table_id_t;

// as of current impl (1/1/26), valid value types
// are only int and std::string and no NULL support

namespace db::model {
class UserTable : public Relation {
public:
    explicit UserTable(HeapFile hf, std::vector<ColumnInfo> schema, table_id_t table_id);
    std::optional<RID> Insert(const std::vector<common::Value> values);
    Tuple Decode(const Record& rec) const override;
private:
    std::vector<ColumnInfo> _schema;
    table_id_t _table_id;
};
}