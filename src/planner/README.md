# Planner Layer Overview

The planner layer converts an `AnalyzedStmt` into an executable operator tree. It has two stages: logical planning (what to do) and physical planning (how to do it). DDL statements (CREATE TABLE, DROP TABLE) bypass the planner and go directly to the utility executor.

The planner consumes output from the parser's Analyzer and produces physical operators for the Executor.

```
AnalyzedStmt (Query | AnalyzedInsert | AnalyzedUpdate | AnalyzedDelete)
    |
    v  LogicalPlanner::Build()
+----------------------------------------+
|           Logical Plan Tree            | Algebra: Scan, Filter, Project, Limit,
|    LogicalScan -> LogicalFilter ->     | Insert, Update, Delete
|    LogicalProject -> LogicalLimit      |
+-------------------+--------------------+
                    |
                    v  PhysicalPlanner::Build()
+----------------------------------------+
|         Physical Operator Tree         | Volcano operators: SeqScanOp,
|    SeqScanOp -> FilterOp ->            | FilterOp, ProjectionOp, LimitOp,
|    ProjectionOp -> LimitOp             | InsertOp, UpdateOp, DeleteOp
+----------------------------------------+
```

## 1. Components

| Component         | Header                         | Implementation              | Role                                      |
| ----------------- | ------------------------------ | --------------------------- | ----------------------------------------- |
| **LogicalPlanner** | `planner/logical/logical_planner.h` | `logical_planner.cpp`        | Builds logical plan tree from AnalyzedStmt |
| **PhysicalPlanner** | `planner/physical/physical_planner.h` | `physical_planner.cpp`       | Maps logical nodes to executor operators   |
| **LogicalPlan**   | `planner/logical/logical_plan.h` | (header only)               | Base class and LogicalPlanType enum        |
| **PlanningContext** | `planner/logical/logical_planner.h` | (struct)                    | Holds TableManager* for plan building     |

## 2. Logical Plan Nodes

All nodes live in `planner/logical/nodes/`. Each implements `LogicalPlanType Type()` and `Children()`.

| Node            | Type Enum   | Statement | Children | Purpose                              |
| --------------- | ----------- | --------- | -------- | ------------------------------------ |
| **LogicalScan** | Scan        | All       | none     | Sequential scan of a table           |
| **LogicalFilter** | Filter    | All       | 1        | Applies WHERE predicate              |
| **LogicalProject** | Project  | SELECT    | 1        | Selects output columns               |
| **LogicalLimit** | Limit     | SELECT    | 1        | Caps output row count                |
| **LogicalInsert** | Insert   | INSERT    | none     | Holds value rows to insert           |
| **LogicalUpdate** | Update   | UPDATE    | 1 (Scan+Filter) | Applies SET to matched rows   |
| **LogicalDelete** | Delete   | DELETE    | 1 (Scan+Filter) | Marks matched rows for deletion |

### 2.1 LogicalScan

Leaf node. Holds table name. No children.

```cpp
LogicalScan(std::string table_name);
std::string TableName() const;
```

### 2.2 LogicalFilter

Wraps a child plan and a predicate (AnalyzedExpr tree). The predicate is cloned from the AnalyzedStmt at plan build time.

```cpp
LogicalFilter(LogicalPlanPtr child, std::unique_ptr<parser::AnalyzedExpr> pred);
const parser::AnalyzedExpr& Predicate() const;
LogicalPlan& Child() const;
```

### 2.3 LogicalProject

Wraps a child and specifies output columns by name and ordinal position. Used for SELECT target list.

```cpp
LogicalProject(LogicalPlanPtr child, std::vector<std::string> cols, std::vector<uint16_t> positions);
const std::vector<std::string>& Columns() const;
const std::vector<uint16_t>& Positions() const;
```

### 2.4 LogicalLimit

Wraps a child and a row limit. Short-circuits execution after N rows.

```cpp
LogicalLimit(LogicalPlanPtr child, size_t limit);
size_t Limit() const;
```

### 2.5 LogicalInsert

Leaf node. Holds table name, target columns, and analyzed value expressions. No children. Values are evaluated at physical plan build time via `EvaluateInsertValues()`.

### 2.6 LogicalUpdate

Child is Scan (optionally wrapped in Filter). Holds table name and SET assignments (column, expression pairs). Assignments are evaluated at physical plan build time via `EvaluateAssignments()`.

### 2.7 LogicalDelete

Child is Scan (optionally wrapped in Filter). Holds table name only.

## 3. Plan Build Order

### SELECT

```
LogicalScan -> [LogicalFilter] -> LogicalProject -> [LogicalLimit]
```

Filter and Limit are optional. Project is always present (even for SELECT *).

### INSERT

```
LogicalInsert (no children)
```

### UPDATE

```
LogicalScan -> [LogicalFilter] -> LogicalUpdate
```

### DELETE

```
LogicalScan -> [LogicalFilter] -> LogicalDelete
```

## 4. Physical Planner

`PhysicalPlanner::Build(LogicalPlan&, PlanningContext&)` recursively walks the logical tree and constructs a tree of executor operators.

| Logical Node   | Physical Operator |
| -------------- | ----------------- |
| LogicalScan    | SeqScanOp         |
| LogicalFilter  | FilterOp          |
| LogicalProject | ProjectionOp      |
| LogicalLimit   | LimitOp           |
| LogicalInsert  | InsertOp          |
| LogicalUpdate  | UpdateOp          |
| LogicalDelete  | DeleteOp          |

`PlanningContext` provides `TableManager*` so the physical planner can resolve table names to `Relation*` via `OpenTable()`.

## 5. CompilePredicate

`CompilePredicate(const parser::AnalyzedExpr&)` converts an `AnalyzedExpr` tree into a `std::function<bool(Tuple)>` (wrapped as `executor::Predicate`). Used by FilterOp and by UpdateOp/DeleteOp for matching rows.

Supports:

- `AnalyzedColumnRef`: tuple slot lookup by ordinal
- `AnalyzedLiteral`: constant value (INT, TEXT, NULL)
- `AnalyzedBinaryExpr`: AND, OR, comparisons (=, !=, <>, <, >, <=, >=)
- `AnalyzedUnaryExpr`: NOT

For comparisons, column refs and literals are compiled into value-extractor lambdas. The planner does not support nested expressions in comparisons (e.g. `col + 1 > 5` would require extending the compiler).

## 6. Plan-Time Evaluation

Some logical nodes hold expressions that must be evaluated before execution:

| Function                | Used By      | Purpose                                      |
| ----------------------- | ------------ | -------------------------------------------- |
| `EvaluateExprToValue()` | Insert, Update | Evaluates AnalyzedLiteral to common::Value  |
| `EvaluateInsertValues()` | LogicalInsert | Converts all INSERT value rows to Values   |
| `EvaluateAssignments()` | LogicalUpdate | Converts SET col=expr to (ordinal, Value)   |

These run once at physical plan build time. Only literals are supported; column refs in SET expressions would require runtime evaluation (not implemented).

## 7. Entry Points

```cpp
// From driver (main.cpp or test_integration)
planner::PlanningContext ctx{&table_mgr};

if (stmt->type == StmtType::Select)
    logical = LogicalPlanner::Build(*stmt->select_query);
else if (stmt->type == StmtType::Insert)
    logical = LogicalPlanner::Build(*stmt->insert_query);
else if (stmt->type == StmtType::Update)
    logical = LogicalPlanner::Build(*stmt->update_query);
else if (stmt->type == StmtType::Delete)
    logical = LogicalPlanner::Build(*stmt->delete_query);

auto physical = PhysicalPlanner::Build(*logical, ctx);
executor::Executor exec{std::move(physical)};
```

## 8. Future Work

- Cost-based plan selection (e.g. index scan vs seq scan, join algorithms)
- Multiple physical implementations per logical node
- Runtime expression evaluation for UPDATE SET (column refs, arithmetic)
