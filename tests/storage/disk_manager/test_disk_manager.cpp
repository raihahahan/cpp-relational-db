#include "storage/disk_manager/disk_manager.h"
#include <gtest/gtest.h>
#include <filesystem>
#include "config/config.h"

#define TEST_FILE "file.db"

namespace db::storage {
class DiskManagerTest : public testing::Test {
protected:

    DiskManagerTest() {
        if (std::filesystem::exists(TEST_FILE)) {
            std::filesystem::remove(TEST_FILE);
        }

        dm = std::make_unique<DiskManager>(TEST_FILE);
    }
    std::unique_ptr<DiskManager> dm;
};

TEST_F(DiskManagerTest, AllocatePageTest) {
    page_id_t id = dm->AllocatePage();
    // initial allocation
    EXPECT_EQ(id, 0);
}

TEST_F(DiskManagerTest, WriteBufferTest) {
    // write buffer to page id 0
    page_id_t id = dm->AllocatePage();

    char write_buf[db::config::PAGE_SIZE];
    std::memset(write_buf, 0, db::config::PAGE_SIZE);
    dm->WritePage(id, write_buf);

    // read page id 0
    char read_buf[db::config::PAGE_SIZE];
    std::memset(read_buf, 0, db::config::PAGE_SIZE);
    dm->ReadPage(id, read_buf);
    EXPECT_EQ(std::strcmp(read_buf, write_buf), 0);
}

TEST_F(DiskManagerTest, IdIncrementTest) {
    page_id_t id = dm->AllocatePage();
    EXPECT_EQ(id, 0);
    id = dm->AllocatePage();
    EXPECT_EQ(id, 1);
}

TEST_F(DiskManagerTest, DeallocatePageTest) {
    dm->AllocatePage(); // page_id = 0
    dm->AllocatePage(); // page_id = 1

    // deallocate page 0
    dm->DeallocatePage(0); 

    // allocate again. should use freed page id 0
    page_id_t id = dm->AllocatePage();
    EXPECT_EQ(id, 0);
}
}