#pragma once
#include <string>
#include "catalog/catalog.h"
#include "parser/analyzer.h"
#include "model/table_manager.h"

namespace db::executor {

struct UtilityResult {
    std::string message;
};

UtilityResult ExecuteCreateTable(
    const parser::AnalyzedCreateTable& stmt,
    model::TableManager& table_mgr);

UtilityResult ExecuteDropTable(
    const parser::AnalyzedDropTable& stmt,
    catalog::Catalog& catalog,
    model::TableManager& table_mgr);

}
