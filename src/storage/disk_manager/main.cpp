#include "storage/disk_manager/disk_manager.h"
#include <iostream>


int main() {
    db::storage::DiskManager dm{"test.db"};
    std::cout << "Num pages: " << dm.GetNumPages() << std::endl;
    auto pid = dm.AllocatePage();
    char buf[8192];
    dm.WritePage(pid, buf);
    std::cout << "Allocated page: " << pid << std:: endl;
    std::cout << "Num pages: " << dm.GetNumPages() << std::endl;
    return 0;
}