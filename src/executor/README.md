# Executor Layer Overview

The executor layer is responsible for evaluating query plans and producing result tuples. It consumes _relations_ and _operators_, executes them using the iterator (Volcano) model, and materialises results row-by-row.

The `/executor` folder contains the following components:

```
+---------------------------------------+
|               Executor                | Drives execution of a physical
|    - Execute()                        | operator tree.
|    - Collect or discard results       |
+-------------------+-------------------+
                    |
                    v
+----------------------------------------+
|               Operator                 | Abstract base class for all
|    - Open() / Next() / Close()         | physical operators.
+-------------------+--------------------+
                    |
                    v
+---------------------------------------+
|        Physical Operators              | Concrete execution nodes:
|    - SeqScanOp (leaf)                  | - sequential scan
|    - FilterOp                          | - selection
|    - ProjectionOp                      | - column projection
|    - LimitOp                           | - row limiting
+---------------------------------------+
```

## 1. Architecture Overview

The executor uses the Volcano (iterator) execution model:

- `Open()` initialises operator state
- `Next()` produces one tuple at a time
- `Close()` releases resources

Each operator pulls tuples from its child, processes them, and returns new tuples upstream.

### Core Components

| Component        | Description             | Responsibility                         |
| ---------------- | ----------------------- | -------------------------------------- |
| **Executor**     | Query driver            | Executes a physical plan end-to-end    |
| **Operator**     | Abstract execution node | Defines `Open / Next / Close` contract |
| **SeqScanOp**    | Leaf operator           | Reads tuples directly from a Relation  |
| **FilterOp**     | Unary operator          | Applies predicates to incoming tuples  |
| **ProjectionOp** | Unary operator          | Selects a subset of columns            |
| **LimitOp**      | Unary operator          | Stops execution after N tuples         |

## 2. Operators and Their Roles

### Leaf vs Non-Leaf Operators

| Operator Type         | Input       | Reason                                |
| --------------------- | ----------- | ------------------------------------- |
| **Leaf operator**     | `Relation&` | Directly produces tuples from storage |
| **Non-leaf operator** | `Operator`  | Transforms tuples from child operator |

### 2.1 SeqScanOp (Leaf Operator)

```cpp
class SeqScanOp : public Operator {
public:
    explicit SeqScanOp(model::Relation& rel);
};
```

**Responsibilities**

- Iterates over all records in a relation
- Decodes records into logical `Tuple`s
- Serves as the entry point for execution

**Why it takes a Relation**

- It is the only operator that touches storage
- Higher operators remain storage-agnostic

### 2.2 FilterOp (Unary Operator)

```cpp
class FilterOp : public Operator {
public:
    FilterOp(std::unique_ptr<Operator> child, Predicate pred);
};
```

**Responsibilities**

- Pulls tuples from its child
- Applies a predicate
- Emits only matching tuples

**Execution behaviour**

- Uses a `while` loop inside `Next()`
- Skips tuples until predicate succeeds

### 2.3 ProjectionOp (Unary Operator)

```cpp
class ProjectionOp : public Operator {
public:
    ProjectionOp(
        std::unique_ptr<Operator> child,
        std::unordered_set<uint16_t> col_pos,
        std::shared_ptr<const Schema> out_schema
    );
};
```

**Responsibilities**

- Selects a subset of columns from each tuple
- Produces a new tuple with a new schema

**Why schema is passed in**

- Projection changes tuple shape
- Schema must remain immutable and shared

### 2.4 LimitOp (Unary Operator)

```cpp
class LimitOp : public Operator {
public:
    LimitOp(std::unique_ptr<Operator> child, size_t limit);
};
```

**Responsibilities**

- Stops execution after `limit` tuples
- Acts as a short-circuiting operator

**Behaviour**

- Maintains an internal counter
- Returns `std::nullopt` once limit is reached

## 3. Operator Composition

Operators are composed bottom-up, forming a physical execution plan.

### Example: `SELECT name FROM students WHERE id >= 2 LIMIT 1`

```cpp
auto scan = std::make_unique<SeqScanOp>(*students_table);

auto filter = std::make_unique<FilterOp>(
    std::move(scan),
    [](const Tuple& t) {
        return std::get<uint32_t>(t.GetValues()[0]) >= 2;
    }
);

auto project = std::make_unique<ProjectionOp>(
    std::move(filter),
    std::unordered_set<uint16_t>{2},
    out_schema
);

auto limit = std::make_unique<LimitOp>(
    std::move(project),
    1
);

Executor exec{std::move(limit)};
auto results = exec.ExecuteAndCollect();
```

Each operator:

- Owns its child (`std::unique_ptr`)
- Pulls tuples lazily
- Is unaware of the full plan

## 4. Executor

The `Executor` is a **thin orchestration layer**.

```cpp
class Executor {
public:
    explicit Executor(std::unique_ptr<Operator> plan);

    void Execute();
    std::vector<Tuple> ExecuteAndCollect();
};
```

### Responsibilities

- Calls `Open()` on the root operator
- Repeatedly calls `Next()`
- Calls `Close()` once execution finishes

### Two Execution Modes

| Method                | Use case                              |
| --------------------- | ------------------------------------- |
| `Execute()`           | Fire-and-forget (e.g. INSERT, DELETE) |
| `ExecuteAndCollect()` | SELECT queries                        |

## 5. Ownership & Pointer Semantics

| Component         | Ownership                       | Reason                          |
| ----------------- | ------------------------------- | ------------------------------- |
| Operator children | `std::unique_ptr`               | Operators form a strict tree    |
| Schemas           | `std::shared_ptr<const Schema>` | Immutable, shared across tuples |
| Relations         | reference (`Relation&`)         | Executor does not own storage   |
| Tuples            | value type                      | Logical, ephemeral results      |

This prevents:

- accidental sharing of operators
- lifetime bugs
- hidden cycles

## Testing Instructions

```bash
make test_operators
make test_executor

```

Tests cover:

- individual operators
- catalog table execution
- operator composition
- full plan execution via `Executor`
