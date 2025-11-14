#pragma once

#include <cstdint>
#include <fstream>
#include <string>
#include <vector> 

namespace db::storage {
using page_id_t = int32_t;

class DiskManager {
public:
    explicit DiskManager(const std::string &db_file);
    DiskManager(const DiskManager& other) = delete;
    DiskManager& operator=(const DiskManager& other) = delete;
    DiskManager(DiskManager&& other) = delete;
    DiskManager& operator=(DiskManager&& other) = delete;

    ~DiskManager();

    void ReadPage(page_id_t page_id, char* page_data);
    void WritePage(page_id_t page_id, const char* page_data);
    page_id_t AllocatePage();
    void DeallocatePage(page_id_t page_id);

    int GetNumPages() const;

private:
    mutable std::fstream db_io_;
    std::vector<page_id_t> free_list;
    page_id_t next_page_id_;
};
}
