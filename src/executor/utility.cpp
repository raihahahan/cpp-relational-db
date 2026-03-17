#include "executor/utility.h"

namespace db::executor {

UtilityResult ExecuteCreateTable(const parser::AnalyzedCreateTable& stmt, model::TableManager& table_mgr) {
    table_mgr.CreateTable(stmt.table_name, stmt.columns);
    return UtilityResult{"CREATE TABLE"};
}

UtilityResult ExecuteDropTable(
    const parser::AnalyzedDropTable& stmt,
    catalog::Catalog& catalog,
    model::TableManager& table_mgr) {
    if (!stmt.table_found) {
        return UtilityResult{"DROP TABLE"};
    }
    catalog.DropTable(stmt.table.table_id);
    table_mgr.EvictTable(stmt.table_name);
    return UtilityResult{"DROP TABLE"};
}

}
