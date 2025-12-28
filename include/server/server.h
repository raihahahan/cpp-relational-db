#pragma once 

#include <unordered_map>
#include <memory>
#include "storage/disk_manager/disk_manager.h"

namespace db::server {

class DbServer {
public:
    DbServer() = default;
    void Init();
    storage::DiskManager* OpenDatabase(const std::string& db_name);
    bool CreateDatabase(const std::string& db_name);
    bool DeleteDatabase(const std::string& db_name);

private:
    std::unordered_map<std::string, 
                        std::unique_ptr<storage::DiskManager>> _cache;
};
}