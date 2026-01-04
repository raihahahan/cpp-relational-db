#pragma once

#include "executor/operator.h"
#include <memory>
#include <vector>

namespace db::executor {
class Executor {
public:
    explicit Executor(std::unique_ptr<Operator> plan);
    void Execute();
    std::vector<Tuple> ExecuteAndCollect();

private:
    std::unique_ptr<Operator> _plan;
};
}