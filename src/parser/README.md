# Parser Layer Overview

The parser layer transforms a raw SQL string into a validated, type-checked analyzed statement. It follows PostgreSQL's three-stage architecture: Lexer -> Parser -> Semantic Analyzer. Supported statements: SELECT, INSERT, UPDATE, and DELETE.

The `/parser` folder contains the following components:

| Component    | Header              | Implementation | Role                                           |
| ------------ | ------------------- | -------------- | ---------------------------------------------- |
| **Lexer**    | `parser/lexer.h`    | `lexer.cpp`    | Tokenises SQL input                            |
| **AST**      | `parser/ast.h`      | (header only)  | Defines the raw parse tree node types          |
| **Parser**   | `parser/parser.h`   | `parser.cpp`   | Builds AST from tokens                         |
| **Analyzer** | `parser/analyzer.h` | `analyzer.cpp` | Validates and enriches AST into `AnalyzedStmt` |

## 1. Pipeline

A SQL string flows through three stages before reaching the planner:

1. **Lexer** -- SQL string -> stream of `Token`s
2. **Parser** -- tokens -> raw AST (`SelectStmt`, `InsertStmt`, `UpdateStmt`, or `DeleteStmt`)
3. **Analyzer** -- raw AST + Catalog -> `AnalyzedStmt` (resolved table/column info, validated types)

```
"SELECT name FROM users WHERE age > 25"
    |
    v  Lexer
[SELECT][name][FROM][users][WHERE][age][>][25]
    |
    v  Parser
SelectStmt {
    target_list: [ResTarget(ColumnRef("name"))]
    from_table: "users"
    where: BinaryExpr(ColumnRef("age"), ">", Literal(25))
}
    |
    v  Analyzer (+ Catalog lookup)
AnalyzedStmt {
    type: Select
    select_query: Query {
        range_table: TableInfo{users}
        target_list: [TargetEntry("name", TEXT_TYPE, resno=2)]
        where_clause: AnalyzedBinaryExpr(
            AnalyzedColumnRef(age, INT_TYPE), ">", AnalyzedLiteral(25, INT_TYPE)
        )
    }
}
```

## 2. Lexer

The `Lexer` takes a `std::string_view` and produces tokens one at a time via `Next()`, or all at once via `LexicalParse()`.

### Token Types

| TokenType       | Examples                              |
| --------------- | ------------------------------------- |
| `Keyword`       | `SELECT`, `FROM`, `WHERE`, `AND`, ... |
| `Identifier`    | `users`, `age`, `my_table`            |
| `Number`        | `42`, `0`, `100`                      |
| `StringLiteral` | `'hello'`, `'it''s'`                  |
| `Operator`      | `=`, `!=`, `<>`, `<=`, `>=`, `+`, `-` |
| `Dot`           | `.` (for `table.column`)              |
| `Comma`         | `,`                                   |
| `LParen/RParen` | `(`, `)`                              |
| `Semicolon`     | `;`                                   |
| `EndOfFile`     | end of input                          |

### Key Behaviours

- **Case-insensitive keywords**: `SELECT`, `select`, `SeLeCt` all produce `TokenType::Keyword` with the original casing preserved in the lexeme.
- **40 reserved keywords**: Covers current SQL support (`SELECT`, `FROM`, `WHERE`, `LIMIT`, `AND`, `OR`, `NOT`, `AS`, `NULL`, `TRUE`, `FALSE`) plus future keywords (`JOIN`, `CREATE`, `INSERT`, `ORDER BY`, etc.).
- **String literals**: Single-quoted with `''` escape for embedded quotes. `'it''s'` produces the value `it's`.
- **Two-character operators**: `<=`, `>=`, `!=`, `<>` are recognised as single tokens.
- **Positioning**: Each token carries a `pos` (byte offset) for error reporting.

## 3. AST (Abstract Syntax Tree)

The AST is defined in `parser/ast.h`. All nodes inherit from `AstNode`. Expression nodes inherit from `Expr`.

### Node Types

| Node         | Purpose                                              |
| ------------ | ---------------------------------------------------- |
| `ColumnRef`  | Column reference, with optional `table_qualifier`    |
| `Literal`    | Integer, string, or NULL literal                     |
| `BinaryExpr` | Two operands with an operator (`=`, `AND`, `+`)      |
| `UnaryExpr`  | One operand with an operator (`NOT`, unary `-`)      |
| `ResTarget`  | A `SELECT` list entry: expression + optional alias   |
| `StarTarget` | Represents `SELECT *`                                |
| `SelectStmt` | Top-level SELECT with targets, FROM, WHERE, LIMIT    |
| `DeleteStmt` | `DELETE FROM table [WHERE ...]`                      |
| `InsertStmt` | `INSERT INTO table [cols] VALUES (...),...`           |
| `SetClause`  | Single `col = expr` pair used in UPDATE              |
| `UpdateStmt` | `UPDATE table SET ... [WHERE ...]`                   |

`SelectTarget` is a `std::variant<StarTarget, ResTarget>`, allowing `SELECT *` and `SELECT col` to coexist in the same target list type.

### Important: The AST is Unresolved

The AST captures **syntax only**. A `ColumnRef` with `name = "age"` has no idea which table it belongs to, what type it is, or what ordinal position it has. That resolution happens in the Analyzer.

## 4. Parser

The `Parser` is a **recursive descent parser** that consumes tokens from the Lexer and produces an AST node (`SelectStmt`, `InsertStmt`, `UpdateStmt`, or `DeleteStmt`).

### Entry Point

```cpp
auto ast = Parser::Parse("SELECT name FROM users WHERE age > 25");
// returns std::unique_ptr<AstNode>
```

The parser inspects the first keyword to decide which statement type to parse.

### Grammar (Supported Subset)

```
statement    ::= select_stmt | insert_stmt | update_stmt | delete_stmt

select_stmt  ::= SELECT target_list FROM table_name
                  [WHERE expr] [LIMIT number] [;]

insert_stmt  ::= INSERT INTO table_name [( column_list )]
                  VALUES ( expr_list ) [, ( expr_list )]* [;]

update_stmt  ::= UPDATE table_name SET set_clause [, set_clause]*
                  [WHERE expr] [;]

delete_stmt  ::= DELETE FROM table_name [WHERE expr] [;]

set_clause   ::= identifier '=' expr
column_list  ::= identifier (',' identifier)*
expr_list    ::= expr (',' expr)*

target_list  ::= '*' | target_entry (',' target_entry)*
target_entry ::= expr [AS alias]

expr         ::= or_expr
or_expr      ::= and_expr (OR and_expr)*
and_expr     ::= not_expr (AND not_expr)*
not_expr     ::= NOT not_expr | comparison
comparison   ::= additive (('=' | '!=' | '<>' | '<' | '>' | '<=' | '>=') additive)?
additive     ::= primary (('+' | '-') primary)*
primary      ::= identifier ['.' identifier]
               | number | string | NULL | TRUE | FALSE
               | '(' expr ')'
               | '-' primary
```

### Operator Precedence (lowest to highest)

1. `OR`
2. `AND`
3. `NOT`
4. Comparisons (`=`, `!=`, `<>`, `<`, `>`, `<=`, `>=`)
5. Additive (`+`, `-`)
6. Primary (literals, column refs, parenthesised expressions)

Each precedence level has its own parsing method. This is the **precedence climbing** technique, where each method calls the next-higher-precedence method as its operand parser.

### Special Handling

- `TRUE` is parsed as `Literal(Integer, "1")`, `FALSE` as `Literal(Integer, "0")` since there is no boolean type yet.
- `NULL` is parsed as `Literal(Null, "")`.
- Qualified column references like `users.age` are parsed by `parse_primary_expr()` checking for a `.` after an identifier.

## 5. Semantic Analyzer

The `Analyzer` transforms a raw AST node into an `AnalyzedStmt`. It requires a `Catalog&` to resolve names.

### Entry Point

```cpp
Analyzer analyzer{catalog};
auto stmt = analyzer.Analyze(*ast);
// returns std::unique_ptr<AnalyzedStmt>
```

The `Analyze()` method dispatches based on the concrete AST type (`SelectStmt`, `InsertStmt`, `UpdateStmt`, or `DeleteStmt`), filling in the appropriate field of `AnalyzedStmt`.

### What It Does

1. **Table resolution**: Looks up the target table via `Catalog::LookupTable()`. Throws `UndefinedTable` if not found.
2. **Column resolution**: For each `ColumnRef`, finds the matching `ColumnInfo` from the table's column list. Throws `UndefinedColumn` if not found.
3. **Wildcard expansion**: `SELECT *` is expanded into one `TargetEntry` per column, ordered by `ordinal_position`.
4. **Type inference**: Assigns `result_type` to every expression node. Integer literals get `INT_TYPE`, string literals get `TEXT_TYPE`, column refs get their catalog type.
5. **Type checking**: Validates that operator arguments have compatible types (e.g. `age > 25` is INT vs INT, OK; `name > 25` is TEXT vs INT, error).

### Output: `AnalyzedStmt`

`AnalyzedStmt` is a tagged union wrapping the four analyzed statement types:

| Field           | Type                            | Populated When         |
| --------------- | ------------------------------- | ---------------------- |
| `type`          | `StmtType` enum                 | Always                 |
| `select_query`  | `unique_ptr<Query>`             | `StmtType::Select`     |
| `insert_query`  | `unique_ptr<AnalyzedInsert>`    | `StmtType::Insert`     |
| `update_query`  | `unique_ptr<AnalyzedUpdate>`    | `StmtType::Update`     |
| `delete_query`  | `unique_ptr<AnalyzedDelete>`    | `StmtType::Delete`     |

### Per-Statement Analysis

**SELECT** produces a `Query`:

| Field           | Type                          | Content                                  |
| --------------- | ----------------------------- | ---------------------------------------- |
| `range_table`   | `catalog::TableInfo`          | Resolved table metadata                  |
| `table_columns` | `vector<catalog::ColumnInfo>` | All columns of the table                 |
| `target_list`   | `vector<TargetEntry>`         | Resolved output columns with `resno`     |
| `where_clause`  | `unique_ptr<AnalyzedExpr>`    | Type-checked filter expression (or null) |
| `limit_count`   | `optional<size_t>`            | Row limit (or nullopt)                   |

**INSERT** produces an `AnalyzedInsert`:

| Field            | Type                                          | Content                                |
| ---------------- | --------------------------------------------- | -------------------------------------- |
| `table`          | `catalog::TableInfo`                          | Resolved table                         |
| `target_columns` | `vector<catalog::ColumnInfo>`                 | Resolved column list (all columns)     |
| `values`         | `vector<vector<unique_ptr<AnalyzedExpr>>>`    | Type-checked value rows                |

**UPDATE** produces an `AnalyzedUpdate`:

| Field           | Type                                                        | Content                                |
| --------------- | ----------------------------------------------------------- | -------------------------------------- |
| `table`         | `catalog::TableInfo`                                        | Resolved table                         |
| `table_columns` | `vector<catalog::ColumnInfo>`                               | All columns of the table               |
| `assignments`   | `vector<pair<catalog::ColumnInfo, unique_ptr<AnalyzedExpr>>>` | Resolved SET assignments             |
| `where_clause`  | `unique_ptr<AnalyzedExpr>`                                  | Optional WHERE filter                  |

**DELETE** produces an `AnalyzedDelete`:

| Field           | Type                          | Content                                |
| --------------- | ----------------------------- | -------------------------------------- |
| `table`         | `catalog::TableInfo`          | Resolved table                         |
| `table_columns` | `vector<catalog::ColumnInfo>` | All columns of the table               |
| `where_clause`  | `unique_ptr<AnalyzedExpr>`    | Optional WHERE filter                  |

### AnalyzedExpr Hierarchy

Mirrors the raw `Expr` hierarchy but carries resolved metadata:

| Node                 | Additional Data                                 |
| -------------------- | ----------------------------------------------- |
| `AnalyzedColumnRef`  | `catalog::ColumnInfo` (table_id, type, ordinal) |
| `AnalyzedLiteral`    | `value` string + `LiteralType`                  |
| `AnalyzedBinaryExpr` | `op` + two `unique_ptr<AnalyzedExpr>`           |
| `AnalyzedUnaryExpr`  | `op` + `unique_ptr<AnalyzedExpr>`               |

All nodes carry `result_type` (a `catalog::type_id_t`).

### Type Checking Rules

| Operator             | Allowed Types      |
| -------------------- | ------------------ |
| `=`, `!=`, `<>`      | INT=INT, TEXT=TEXT |
| `<`, `>`, `<=`, `>=` | INT=INT only       |
| `+`, `-`             | INT=INT only       |
| `AND`, `OR`, `NOT`   | INT only           |

`NULL` is compatible with any type on either side of a comparison.

## 6. Error Handling

All errors throw `DbError` (from `error/dberror.h`) with a specific `ErrorCode`:

| ErrorCode         | When                                          |
| ----------------- | --------------------------------------------- |
| `SyntaxError`     | Lexer: unterminated string, invalid character |
| `ParseError`      | Parser: unexpected token, missing clause      |
| `UndefinedTable`  | Analyzer: table name not found in Catalog     |
| `UndefinedColumn` | Analyzer: column name not found in table      |
| `TypeMismatch`    | Analyzer: incompatible types for operator     |

Error messages include the byte offset position where the problem was detected.

## 7. Integration with Planner

The `AnalyzedStmt` output of the Analyzer feeds into the appropriate `LogicalPlanner::Build()` overload based on statement type:

- **SELECT**: `Build(const Query&)` produces Scan -> Filter -> Project -> Limit
- **INSERT**: `Build(const AnalyzedInsert&)` produces a `LogicalInsert` node (no children)
- **UPDATE**: `Build(const AnalyzedUpdate&)` produces Scan -> Filter -> Update
- **DELETE**: `Build(const AnalyzedDelete&)` produces Scan -> Filter -> Delete

The `AnalyzedExpr` tree in WHERE clauses is cloned (via `parser::clone()`) into `LogicalFilter` nodes, then compiled into a runtime predicate by the physical planner's `CompilePredicate()`. DML operators (`InsertOp`, `UpdateOp`, `DeleteOp`) return a single "count tuple" with the number of affected rows.

## Testing Instructions

```bash
# Run all parser-related tests (lexer + parser + analyzer + integration)
make test_parser_all

# Run individually
make test_lexer
make test_parser
make test_analyzer
make test_integration

# Interactive SQL REPL (seeds a users table with sample data)
make run_sql
```

## Standalone Parser Driver

There is a standalone driver at `src/parser/main.cpp` that reads SQL from stdin and prints the raw AST. It does **not** require the full engine, only the lexer and parser.

```bash
make build_parser
echo "SELECT name FROM users WHERE age > 25" | src/parser/build/parser_driver
```

## Adding a New Statement Type

When adding support for a new SQL statement (e.g. a new DML or DDL command), follow these steps in order:

1. **AST** (`include/parser/ast.h`): Define a new struct inheriting from `AstNode` for the statement (e.g. `CreateTableStmt`). Add any helper structs it needs (like `SetClause` for UPDATE).

2. **Parser** (`include/parser/parser.h`, `src/parser/parser.cpp`): Add a private parse method (e.g. `parse_create_table_stmt()`). Update the `Parse()` entry point to dispatch on the leading keyword.

3. **Analyzer** (`include/parser/analyzer.h`, `src/parser/analyzer.cpp`): Define the analyzed output struct (e.g. `AnalyzedCreateTable`). Add the new variant to `StmtType` enum and a field to `AnalyzedStmt`. Implement the analysis method that resolves names, checks types, and validates semantics. Update `Analyze()` to dispatch via `dynamic_cast`.

4. **Logical Planner** (`include/planner/logical/`): Add a `LogicalPlanType` enum value. Create a new plan node header in `nodes/`. Add a `Build()` overload to `LogicalPlanner`.

5. **Physical Planner** (`src/planner/physical/physical_planner.cpp`): Create the executor operator header and implementation (in `include/executor/operators/` and `src/executor/operators/`). Add a case in `PhysicalPlanner::Build()` to construct it. Add any plan-time evaluation helpers (e.g. `EvaluateInsertValues`).

6. **Driver** (`src/main.cpp`): Add a dispatch branch for the new `StmtType` in the main REPL loop.

7. **Build** (`CMakeLists.txt`): Add any new `.cpp` files to the `add_library(main ...)` target.

8. **Tests**: Add parser tests (does it parse correctly?), analyzer tests (does it reject bad input?), and integration tests (does it produce correct results end-to-end?).

9. **Engine layers** (if needed): Some statements require changes below the parser/planner, for example new Catalog methods, new storage operations, or new access method APIs. This should be rare for query-type statements but common for DDL.
