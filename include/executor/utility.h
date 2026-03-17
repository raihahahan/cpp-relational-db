#pragma once
#include <string>
#include "parser/analyzer.h"
#include "model/table_manager.h"

namespace db::executor {

struct UtilityResult {
    std::string message;
};

UtilityResult ExecuteCreateTable(
    const parser::AnalyzedCreateTable& stmt,
    model::TableManager& table_mgr);

}
