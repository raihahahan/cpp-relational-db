#include "executor/executor.h"

namespace db::executor {
Executor::Executor(std::unique_ptr<Operator> plan) : _plan{std::move(plan)} {};

void Executor::Execute() {
    _plan->Open();
    while (true) {
        if (!_plan->Next()) break;
    }
    _plan->Close();
}

std::vector<Tuple> Executor::ExecuteAndCollect() {
    std::vector<Tuple> res;
    _plan->Open();
    while (auto tup = _plan->Next()) {
        res.push_back(*tup);
    }
    _plan->Close();
    return res;
}
}