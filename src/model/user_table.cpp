#include "model/user_table.h"
#include "model/dynamic_codec.h"

namespace db::model {
UserTable::UserTable(HeapFile hf, std::vector<ColumnInfo> schema, table_id_t table_id) :
                        Relation{std::move(hf)},
                        _schema{std::move(schema)},
                        _table_id{table_id} {};

std::optional<RID> UserTable::Insert(const std::vector<Value> values) {
    auto bytes = DynamicCodec::Encode(values, _schema);
    return InsertRaw(bytes, bytes.size());
}
}