#include "executor/utility.h"

namespace db::executor {

UtilityResult ExecuteCreateTable(const parser::AnalyzedCreateTable& stmt, model::TableManager& table_mgr) {
    table_mgr.CreateTable(stmt.table_name, stmt.columns);
    return UtilityResult{"CREATE TABLE"};
}

}
