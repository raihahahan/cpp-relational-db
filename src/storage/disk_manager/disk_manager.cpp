#include "storage/disk_manager/disk_manager.h"
#include "config/config.h"

namespace db::storage {
DiskManager::DiskManager(const std::string &db_file) {
    db_io_ = std::fstream(db_file, std::ios::in | std::ios::out | std::ios::binary);
    if (!db_io_.is_open()) {
        // create file if not exists
        db_io_.open(db_file, std::ios::out | std::ios::binary);
        db_io_.close();
        db_io_.open(db_file, std::ios::in | std::ios::out | std::ios::binary);
    }

    next_page_id_ = GetNumPages();
};

DiskManager::~DiskManager() {
    db_io_.close();
};

void DiskManager::ReadPage(page_id_t page_id, char* page_data) {
    const size_t offset = static_cast<size_t>(page_id) * Config::PAGE_SIZE;
    db_io_.seekg(offset);
    db_io_.read(page_data, Config::PAGE_SIZE);
}

void DiskManager::WritePage(page_id_t page_id, const char* page_data) {
    const size_t offset = static_cast<size_t>(page_id) * Config::PAGE_SIZE;
    db_io_.seekp(offset);
    db_io_.write(page_data, Config::PAGE_SIZE);
    db_io_.flush();
}

db::storage::page_id_t DiskManager::AllocatePage() {
    if (!free_list.empty()) {
        // free list available
        // use a current free frame instead of adding to free list
        auto id = free_list.back();
        free_list.pop_back();
        return id;
    }
    
    // free list empty: all existing page used
    // append a new one
    return next_page_id_++;
}

void DiskManager::DeallocatePage(page_id_t page_id) {
    free_list.push_back(page_id);
}

int DiskManager::GetNumPages() const {
    db_io_.seekg(0, std::ios::end);
    return db_io_.tellg() / Config::PAGE_SIZE;
}
}
