#pragma once

#include "executor/operator.h"
#include "model/relation.h"

namespace db::executor {
class SeqScanOp : public Operator {
public:
    explicit SeqScanOp(std::shared_ptr<model::Relation> rel);
    void Open() override;
    std::optional<Tuple> Next() override;
    void Close() override;

private:
    std::shared_ptr<model::Relation> _rel;
    HeapIterator _iter;
};
}