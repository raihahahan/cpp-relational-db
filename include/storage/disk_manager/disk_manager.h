#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector> 
#include "storage/disk_manager/idisk_manager.h"

namespace db::storage {
using page_id_t = int32_t;

class DiskManager : public IDiskManager {
public:
    explicit DiskManager(const std::string &db_file);
    DiskManager(const DiskManager& other) = delete;
    DiskManager& operator=(const DiskManager& other) = delete;
    DiskManager(DiskManager&& other) = delete;
    DiskManager& operator=(DiskManager&& other) = delete;

    ~DiskManager() override;

    void ReadPage(page_id_t page_id, char* page_data) override;
    void WritePage(page_id_t page_id, const char* page_data) override;
    page_id_t AllocatePage() override;
    void DeallocatePage(page_id_t page_id) override;

    int GetNumPages() const;

private:
    mutable std::fstream db_io_;
    std::vector<page_id_t> free_list;
    page_id_t next_page_id_;
};
}
