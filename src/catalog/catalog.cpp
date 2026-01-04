#include "catalog/catalog.h"
#include "config/config.h"
#include <assert.h>
#include "util/uuid.h"
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
        const std::vector<RawColumnInfo>& columns
) {
    table_id_t table_id = util::GenerateUUID();
    file_id_t fid = util::GenerateUUID();
    auto hf = HeapFile::Create(_bm, _dm, fid);
    _tables.value().Insert(TableInfo{
                                table_id,
                                table_name,
                                fid,
                                hf.GetPageId()
                            });
    
    for (const auto& col : columns) {
        ColumnInfo info{
                        .col_name = col.col_name,
                        .ordinal_position = col.ordinal_position,
                        .table_id = table_id,
                        .type_id = col.type_id };
        _attributes.value().Insert(info);
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
    if (_dm->GetNumPages() <= 0) return false;
    char buf[config::PAGE_SIZE];
    _dm->ReadPage(ROOT_PAGE_ID, buf);
    auto* hdr = reinterpret_cast<const storage::DBHeaderPage*>(buf);
    return hdr->magic == config::DB_MAGIC;
}

void Catalog::LoadCatalogs() {
    auto table_hf = HeapFile::Open(
        _bm, 
        _dm, 
        DB_TABLES_FILE_ID,
        DB_TABLES_ROOT_PAGE_ID
    );

    auto attr_hf = HeapFile::Open(
        _bm, 
        _dm, 
        DB_ATTRIBUTES_FILE_ID,
        DB_ATTRIBUTES_ROOT_PAGE_ID
    );

    auto types_hf = HeapFile::Open(
        _bm, 
        _dm, 
        DB_TYPES_FILE_ID,
        DB_TYPES_ROOT_PAGE_ID
    );

    _tables.emplace(TablesCatalog(table_hf));
    _attributes.emplace(AttributesCatalog(attr_hf));
    _types.emplace(TypesCatalog(types_hf));
}

void Catalog::BootstrapCatalogs() {
    // page 0
    page_id_t pid = _dm->AllocatePage();
    assert(pid == ROOT_PAGE_ID);
    char page[config::PAGE_SIZE]{};
    auto* hdr = reinterpret_cast<storage::DBHeaderPage*>(page);
    hdr->magic = config::DB_MAGIC;
    _dm->WritePage(pid, reinterpret_cast<const char*>(page));

    // db_tables
    page_id_t p1 = _dm->AllocatePage();
    assert(p1 == DB_TABLES_ROOT_PAGE_ID);
    {
        auto* frame1 = _bm->request(p1);
        access::HeapFile::InitHeapPage(frame1->data);
        _bm->mark_dirty(frame1);
        _bm->unpin(frame1);
    }

    // db_attributes
    page_id_t p2 = _dm->AllocatePage();
    assert(p2 == DB_ATTRIBUTES_ROOT_PAGE_ID);
    {
        auto* frame2 = _bm->request(p2);
        access::HeapFile::InitHeapPage(frame2->data);
        _bm->mark_dirty(frame2);
        _bm->unpin(frame2);
    }

    // db_types
    page_id_t p3 = _dm->AllocatePage();
    assert(p3 == DB_TYPES_ROOT_PAGE_ID);
    {
        auto* frame3 = _bm->request(p3);
        access::HeapFile::InitHeapPage(frame3->data);
        _bm->mark_dirty(frame3);
        _bm->unpin(frame3);
    }

    _tables.emplace(TablesCatalog(
        HeapFile::Open(_bm, _dm, DB_TABLES_FILE_ID, DB_TABLES_ROOT_PAGE_ID)
    ));
    _attributes.emplace(AttributesCatalog(
        HeapFile::Open(_bm, _dm, DB_ATTRIBUTES_FILE_ID, DB_ATTRIBUTES_ROOT_PAGE_ID)
    ));
    _types.emplace(TypesCatalog(
        HeapFile::Open(_bm, _dm, DB_TYPES_FILE_ID, DB_TYPES_ROOT_PAGE_ID)
    ));

    InsertBuiltinTypes();
    InsertCatalogMetadata();

    _bm->flush_all();
}

void Catalog::InsertBuiltinTypes() {
    // INT
    _types.value().Insert(TypeInfo{
        .type_id = INT_TYPE,
        .size = INT_SIZE
    });

    // TEXT (variable length)
    _types.value().Insert(TypeInfo{
        .type_id = TEXT_TYPE,
        .size = TEXT_SIZE 
    });
}

void Catalog::InsertCatalogMetadata() {
    // db_tables
    _tables.value().Insert(TableInfo{
        .table_id = DB_TABLES_TABLE_ID,
        .table_name = DB_TABLES_TABLE,
        .heap_file_id = DB_TABLES_FILE_ID,
        .first_page_id = DB_TABLES_ROOT_PAGE_ID
    });

    for (const auto& col : TABLES_CATALOG_SCHEMA) {
        _attributes->Insert(col);
    }

    // db_attributes
    _tables.value().Insert(TableInfo{
        .table_id = DB_ATTRIBUTES_TABLE_ID,
        .table_name = DB_ATTRIBUTES_TABLE,
        .heap_file_id = DB_ATTRIBUTES_FILE_ID,
        .first_page_id = DB_ATTRIBUTES_ROOT_PAGE_ID
    });

    for (const auto& col : ATTR_CATALOG_SCHEMA) {
        _attributes->Insert(col);
    }

    // db_types
    _tables.value().Insert(TableInfo{
        .table_id = DB_TYPES_TABLE_ID,
        .table_name = DB_TYPES_TABLE,
        .heap_file_id = DB_TYPES_FILE_ID,
        .first_page_id = DB_TYPES_ROOT_PAGE_ID
    });

    for (const auto& col : TYPES_CATALOG_SCHEMA) {
        _attributes->Insert(col);
    }
}

TablesCatalog* Catalog::GetTablesCatalog() const {
    return const_cast<TablesCatalog*>(&(_tables.value()));
}
AttributesCatalog* Catalog::GetAttributesCatalog() const {
    return const_cast<AttributesCatalog*>(&(_attributes.value()));
}
TypesCatalog* Catalog::GetTypesCatalog() const {
    return const_cast<TypesCatalog*>(&(_types.value()));
}

db::storage::BufferManager* Catalog::GetBm() const {
    return _bm;
}
db::storage::DiskManager* Catalog::GetDm() const {
    return _dm;
}


}