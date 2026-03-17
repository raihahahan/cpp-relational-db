#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "catalog/catalog_bootstrap.h"
#include "error/dberror.h"
#include "executor/executor.h"
#include "model/table_manager.h"
#include "parser/analyzer.h"
#include "parser/lexer.h"
#include "parser/parser.h"
#include "planner/logical/logical_planner.h"
#include "planner/physical/physical_planner.h"
#include "storage/buffer_manager/buffer_manager.h"
#include "storage/buffer_manager/replacement_policies/clock_policy.h"
#include "storage/disk_manager/disk_manager.h"

using namespace db;

static void print_tuple(const common::Tuple &t) {
    const auto &vals = t.GetValues();
    for (size_t i = 0; i < vals.size(); ++i) {
        if (i > 0)
            std::cout << " | ";
        std::visit(
            [](auto &&v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, uint32_t>)
                    std::cout << v;
                else if constexpr (std::is_same_v<T, std::string>)
                    std::cout << v;
                else
                    std::cout << "?";
            },
            vals[i]);
    }
    std::cout << "\n";
}

int main() {
    const std::string db_file = "sql_engine.db";

    storage::DiskManager dm{db_file};
    storage::BufferManager bm{storage::ReplacementPolicyType::CLOCK, &dm};
    catalog::Catalog catalog{&bm, &dm};
    catalog.Init();
    model::TableManager table_mgr{&catalog};

    // Seed a sample table: users(id INT, name TEXT, age INT)
    std::vector<catalog::RawColumnInfo> schema = {
        {"id", catalog::INT_TYPE, 1},
        {"name", catalog::TEXT_TYPE, 2},
        {"age", catalog::INT_TYPE, 3},
    };
    table_mgr.CreateTable("users", schema);
    auto users = table_mgr.OpenTable("users");
    users->Insert({common::Value{uint32_t(1)}, common::Value{std::string("Alice")},
                   common::Value{uint32_t(30)}});
    users->Insert({common::Value{uint32_t(2)}, common::Value{std::string("Bob")},
                   common::Value{uint32_t(25)}});
    users->Insert({common::Value{uint32_t(3)}, common::Value{std::string("Carol")},
                   common::Value{uint32_t(35)}});
    users->Insert({common::Value{uint32_t(4)}, common::Value{std::string("Dave")},
                   common::Value{uint32_t(28)}});
    users->Insert({common::Value{uint32_t(5)}, common::Value{std::string("Eve")},
                   common::Value{uint32_t(22)}});

    std::cout << "SQL Engine ready. Table 'users' seeded with 5 rows.\n";
    std::cout << "Columns: id (INT), name (TEXT), age (INT)\n";
    std::cout << "Enter SQL (one line) or 'quit':\n\n";

    std::string line;
    while (true) {
        std::cout << "sql> ";
        if (!std::getline(std::cin, line))
            break;
        if (line == "quit" || line == "exit")
            break;
        if (line.empty())
            continue;

        try {
            auto ast = parser::Parser::Parse(line);
            parser::Analyzer analyzer{catalog};
            auto stmt = analyzer.Analyze(*ast);
            planner::PlanningContext ctx{&table_mgr};

            if (stmt->type == parser::StmtType::Select) {
                auto logical_plan = planner::LogicalPlanner::Build(*stmt->select_query);
                auto physical_plan = planner::PhysicalPlanner::Build(*logical_plan, ctx);
                executor::Executor exec{std::move(physical_plan)};
                auto results = exec.ExecuteAndCollect();

                std::cout << "--- ";
                for (size_t i = 0; i < stmt->select_query->target_list.size(); ++i) {
                    if (i > 0) std::cout << " | ";
                    std::cout << stmt->select_query->target_list[i].name;
                }
                std::cout << " ---\n";

                for (const auto &row : results) {
                    print_tuple(row);
                }
                std::cout << "(" << results.size() << " rows)\n\n";
            } else {
                planner::LogicalPlanPtr logical_plan;
                if (stmt->type == parser::StmtType::Insert) {
                    logical_plan = planner::LogicalPlanner::Build(*stmt->insert_query);
                } else if (stmt->type == parser::StmtType::Update) {
                    logical_plan = planner::LogicalPlanner::Build(*stmt->update_query);
                } else if (stmt->type == parser::StmtType::Delete) {
                    logical_plan = planner::LogicalPlanner::Build(*stmt->delete_query);
                }
                auto physical_plan = planner::PhysicalPlanner::Build(*logical_plan, ctx);
                executor::Executor exec{std::move(physical_plan)};
                auto results = exec.ExecuteAndCollect();

                uint32_t count = 0;
                if (!results.empty()) {
                    count = std::get<uint32_t>(results[0].GetValues()[0]);
                }
                    
                std::cout << "(" << count << " rows affected)\n\n";
            }

        } catch (const DbError &e) {
            std::cerr << "ERROR: " << e.what() << "\n\n";
        } catch (const std::exception &e) {
            std::cerr << "ERROR: " << e.what() << "\n\n";
        }
    }

    return 0;
}
