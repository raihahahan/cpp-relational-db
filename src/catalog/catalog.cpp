#include "catalog/catalog.h"
#include "config/config.h"
#include <assert.h>
namespace db::catalog {
Catalog::Catalog(db::storage::BufferManager* bm,
                db::storage::DiskManager* dm) :
                    _bm{bm}, _dm{dm} {}

void Catalog::Init() { 
    if (IsInitialised()) { 
        LoadCatalogs(); 
        return; 
    } 
    BootstrapCatalogs(); 
}

// table operations
table_id_t Catalog::CreateTable(
        const std::string& table_name,
        const std::vector<ColumnInfo>& columns
) {
    table_id_t table_id = AllocateTableId();
    file_id_t fid = AllocateFileId();
    auto hf = HeapFile::Create(_bm, _dm, fid);
    _tables.value().Insert(TableInfo{
                                table_id,
                                table_name,
                                fid,
                                hf.GetPageId()
                            });
    
    for (const auto& col : columns) {
        _attributes.value().Insert(col);
    }
    return table_id;
}

std::optional<TableInfo> Catalog::LookupTable(const std::string& table_name) {
    return _tables.value().Lookup(table_name);
}

std::vector<ColumnInfo> Catalog::GetTableColumns(table_id_t table_id) {
    return _attributes.value().GetColumns(table_id);
}

// bootstrapping
bool Catalog::IsInitialised() const {
    if (_dm->GetNumPages() == 0) return false;
    char buf[config::PAGE_SIZE];
    _dm->ReadPage(config::ROOT_PAGE_ID, buf);
    auto* hdr = reinterpret_cast<const storage::DBHeaderPage*>(buf);
    return hdr->magic == config::DB_MAGIC;
}

void Catalog::LoadCatalogs() {
    auto table_hf = HeapFile::Open(
        _bm, 
        _dm, 
        DB_TABLES_FILE_ID,
        config::DB_TABLES_ROOT_PAGE_ID
    );

    auto attr_hf = HeapFile::Open(
        _bm, 
        _dm, 
        DB_ATTRIBUTES_FILE_ID,
        config::DB_ATTRIBUTES_ROOT_PAGE_ID
    );

    auto types_hf = HeapFile::Open(
        _bm, 
        _dm, 
        DB_TYPES_FILE_ID,
        config::DB_TYPES_ROOT_PAGE_ID
    );

    _tables.emplace(TablesCatalog(table_hf));
    _attributes.emplace(AttributesCatalog(attr_hf));
    _types.emplace(TypesCatalog(types_hf));
}

void Catalog::BootstrapCatalogs() {
    // page 0
    page_id_t pid = _dm->AllocatePage();
    assert(pid == config::ROOT_PAGE_ID);

    storage::DBHeaderPage hdr{ .magic = config::DB_MAGIC };
    _dm->WritePage(0, reinterpret_cast<const char*>(&hdr));

    // db_tables
    page_id_t p1 = _dm->AllocatePage();
    assert(p1 == config::DB_TABLES_ROOT_PAGE_ID);
    {
        auto* frame1 = _bm->request(p1);
        access::HeapFile::InitHeapPage(frame1->data);
        _bm->unpin(frame1);
        _bm->mark_dirty(frame1);
    }

    // db_attributes
    page_id_t p2 = _dm->AllocatePage();
    assert(p2 == config::DB_ATTRIBUTES_ROOT_PAGE_ID);
    {
        auto* frame2 = _bm->request(p2);
        access::HeapFile::InitHeapPage(frame2->data);
        _bm->unpin(frame2);
        _bm->mark_dirty(frame2);
    }

    // db_types
    page_id_t p3 = _dm->AllocatePage();
    assert(p3 == config::DB_TYPES_ROOT_PAGE_ID);
    {
        auto* frame3 = _bm->request(p3);
        access::HeapFile::InitHeapPage(frame3->data);
        _bm->unpin(frame3);
        _bm->mark_dirty(frame3);
    }

    _tables.emplace(TablesCatalog(
        HeapFile::Open(_bm, _dm, DB_TABLES_FILE_ID, config::DB_TABLES_ROOT_PAGE_ID)
    ));
    _attributes.emplace(AttributesCatalog(
        HeapFile::Open(_bm, _dm, DB_ATTRIBUTES_FILE_ID, config::DB_ATTRIBUTES_ROOT_PAGE_ID)
    ));
    _types.emplace(TypesCatalog(
        HeapFile::Open(_bm, _dm, DB_TYPES_FILE_ID, config::DB_TYPES_ROOT_PAGE_ID)
    ));

    InsertBuiltinTypes();
    InsertCatalogMetadata();
}

void Catalog::InsertBuiltinTypes() {
    // INT
    _types.value().Insert(TypeInfo{
        .type_id = 1,
        .size = 4
    });

    // TEXT (variable length)
    _types.value().Insert(TypeInfo{
        .type_id = 2,
        .size = 0   // 0 or -1 means varlen
    });
}

void Catalog::InsertCatalogMetadata() {
    // db_tables
    _tables.value().Insert(TableInfo{
        .table_id = DB_TABLES_TABLE_ID,
        .table_name = DB_TABLES_TABLE,
        .heap_file_id = DB_TABLES_FILE_ID,
        .first_page_id = _tables->GetFirstPage()
    });

    _attributes.value().Insert({DB_TABLES_TABLE_ID, 1, "table_id", 1, 1});
    _attributes.value().Insert({DB_TABLES_TABLE_ID, 2, "table_name", 2, 2});
    _attributes.value().Insert({DB_TABLES_TABLE_ID, 3, "heap_file_id", 1, 3});
    _attributes.value().Insert({DB_TABLES_TABLE_ID, 4, "first_page_id", 1, 4});

    // db_attributes
    _tables.value().Insert(TableInfo{
        .table_id = DB_ATTRIBUTES_TABLE_ID,
        .table_name = DB_ATTRIBUTES_TABLE,
        .heap_file_id = DB_ATTRIBUTES_FILE_ID,
        .first_page_id = _attributes->GetFirstPage()
    });

    _attributes.value().Insert({DB_ATTRIBUTES_TABLE_ID, 1, "table_id", 1, 1});
    _attributes.value().Insert({DB_ATTRIBUTES_TABLE_ID, 2, "col_id", 1, 2});
    _attributes.value().Insert({DB_ATTRIBUTES_TABLE_ID, 3, "col_name", 2, 3});
    _attributes.value().Insert({DB_ATTRIBUTES_TABLE_ID, 4, "type_id", 1, 4});
    _attributes.value().Insert({DB_ATTRIBUTES_TABLE_ID, 5, "ordinal_position", 1, 5});

    // db_types
    _tables.value().Insert(TableInfo{
        .table_id = DB_TYPES_TABLE_ID,
        .table_name = DB_TYPES_TABLE,
        .heap_file_id = DB_TYPES_FILE_ID,
        .first_page_id = _types->GetFirstPage()
    });

    _attributes.value().Insert({DB_TYPES_TABLE_ID, 1, "type_id", 1, 1});
    _attributes.value().Insert({DB_TYPES_TABLE_ID, 2, "size", 1, 2});
}
}