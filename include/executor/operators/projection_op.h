#pragma once

#include "executor/operator.h"
#include <memory>
#include "catalog/catalog_types.h"
#include <string>

namespace db::executor {


class ProjectionOp : public Operator {
public:
    explicit ProjectionOp(std::unique_ptr<Operator> child,
                            std::unordered_set<uint16_t> col_pos,
                            std::shared_ptr<const common::Schema> out_schema);
    void Open() override;
    std::optional<Tuple> Next() override;
    void Close() override;
    Tuple Project(const Tuple& tup);

private:
    std::unique_ptr<Operator> _child;
    std::unordered_set<uint16_t> _col_pos;
    std::shared_ptr<const common::Schema> _out_schema;
};
    
}