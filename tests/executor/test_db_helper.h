#pragma once

#include "storage/disk_manager/disk_manager.h"
#include "storage/buffer_manager/buffer_manager.h"
#include "storage/buffer_manager/replacement_policies/clock_policy.h"
#include "catalog/catalog.h"
#include "model/table_manager.h"

struct TestDB {
    std::unique_ptr<db::storage::DiskManager> dm;
    std::unique_ptr<db::storage::BufferManager> bm;
    std::unique_ptr<db::catalog::Catalog> catalog;
    std::unique_ptr<db::model::TableManager> table_mgr;

    explicit TestDB(const std::string& path) {
        dm = std::make_unique<db::storage::DiskManager>(path);
        bm = std::make_unique<db::storage::BufferManager>(
            db::storage::ReplacementPolicyType::CLOCK, dm.get()
        );
        catalog = std::make_unique<db::catalog::Catalog>(bm.get(), dm.get());
        catalog->Init();
        table_mgr = std::make_unique<db::model::TableManager>(catalog.get());
    }
};
