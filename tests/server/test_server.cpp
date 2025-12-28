#include <gtest/gtest.h>
#include <filesystem>

#include "server/server.h"
#include "storage/disk_manager/disk_manager.h"
#include "config/config.h"

using namespace db;
using namespace db::server;
using namespace db::storage;

class DbServerTest : public ::testing::Test {
protected:
    std::string data_dir = config::DATA_PATH;

    void SetUp() override {
        std::filesystem::create_directories(data_dir);

        // clean up old test dbs
        for (auto& e : std::filesystem::directory_iterator(data_dir)) {
            if (e.path().extension() == ".db") {
                std::filesystem::remove(e.path());
            }
        }
    }

    void TearDown() override {
        for (auto& e : std::filesystem::directory_iterator(data_dir)) {
            if (e.path().extension() == ".db") {
                std::filesystem::remove(e.path());
            }
        }
    }
};

TEST_F(DbServerTest, CreateDatabase) {
    DbServer server;
    server.Init();

    EXPECT_TRUE(server.CreateDatabase("testdb"));
    EXPECT_FALSE(server.CreateDatabase("testdb")); // duplicate
}

TEST_F(DbServerTest, OpenExistingDatabase) {
    DbServer server;
    server.Init();

    ASSERT_TRUE(server.CreateDatabase("mydb"));

    DiskManager* dm = server.OpenDatabase("mydb");
    ASSERT_NE(dm, nullptr);
}

TEST_F(DbServerTest, OpenNonExistentDatabase) {
    DbServer server;
    server.Init();

    DiskManager* dm = server.OpenDatabase("does_not_exist");
    EXPECT_EQ(dm, nullptr);
}

TEST_F(DbServerTest, RestartLoadsDatabases) {
    {
        DbServer server;
        server.Init();
        ASSERT_TRUE(server.CreateDatabase("persisted"));
    }

    // simulate restart
    DbServer server;
    server.Init();

    DiskManager* dm = server.OpenDatabase("persisted");
    ASSERT_NE(dm, nullptr);
}

TEST_F(DbServerTest, DeleteDatabase) {
    DbServer server;
    server.Init();

    ASSERT_TRUE(server.CreateDatabase("todelete"));
    ASSERT_TRUE(server.DeleteDatabase("todelete"));

    DiskManager* dm = server.OpenDatabase("todelete");
    EXPECT_EQ(dm, nullptr);
}

TEST_F(DbServerTest, DeleteNonExistentDatabase) {
    DbServer server;
    server.Init();

    EXPECT_FALSE(server.DeleteDatabase("ghost"));
}
