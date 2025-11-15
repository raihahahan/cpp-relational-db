#pragma once

#include "storage/disk_manager/idisk_manager.h"
#include "config/config.h"
#include <unordered_map>
#include <vector>
#include <cstring>

namespace db::storage {

class MockDiskManager : public IDiskManager {
public:
    std::unordered_map<page_id_t, std::vector<char>> store;

    void ReadPage(page_id_t pid, char* out) override {
        auto& buf = store[pid];
        if (buf.empty()) buf.resize(config::PAGE_SIZE, 0);
        memcpy(out, buf.data(), config::PAGE_SIZE);
    }

    void WritePage(page_id_t pid, const char* data) override {
        auto& buf = store[pid];
        buf.assign(data, data + config::PAGE_SIZE);
    }

    page_id_t AllocatePage() override {
        static page_id_t next = 0;
        return next++;
    }

    void DeallocatePage(page_id_t) override {}
};

}
