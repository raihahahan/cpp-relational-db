#include "server/server.h"
#include "config/config.h"
#include <filesystem>
#include <format>

namespace db::server {
std::string makePath(std::string db_name);

void DbServer::Init() {
    for (const auto& entry :
        std::filesystem::directory_iterator(config::DATA_PATH)) {
            if (!entry.is_regular_file()) continue;
            auto path = entry.path();
            if (path.extension() != ".db") continue;
            std::string name = path.stem().string();
            _cache[name] = std::make_unique<storage::DiskManager>(path.string());
    }
}

storage::DiskManager* DbServer::OpenDatabase(const std::string& db_name) {
    auto it = _cache.find(db_name);
    if (it != _cache.end()) {
        return it->second.get();
    }

    std::string path = makePath(db_name);

    if (!std::filesystem::exists(path)) {
        return nullptr;
    }

    auto dm = std::make_unique<storage::DiskManager>(path);
    auto* ptr = dm.get();
    _cache.emplace(db_name, std::move(dm));
    return ptr;
}

bool DbServer::CreateDatabase(const std::string& db_name) {
    if (_cache.count(db_name)) return false;
    std::string path = makePath(db_name);
    if (std::filesystem::exists(path)) return false;

    auto dm = std::make_unique<storage::DiskManager>(path);
    _cache.emplace(db_name, std::move(dm));
    return true;
}

bool DbServer::DeleteDatabase(const std::string& db_name) {
    auto it = _cache.find(db_name);
    if (it == _cache.end()) return false;
    _cache.erase(it);
    std::string path = makePath(db_name);
    std::filesystem::remove(path);
    return true;
}

// util
std::string makePath(std::string db_name) {
    return std::format("{}/{}.db", config::DATA_PATH, db_name);
}
}