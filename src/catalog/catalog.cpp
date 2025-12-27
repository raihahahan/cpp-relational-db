#include "catalog/catalog.h"
#include "catalog/catalog_bootstrap.h"
#include "access/heap/heap_file.h"

namespace db::catalog {

explicit Catalog::Catalog(BufferManager* bm, DiskManager* dm)
    : _bm{bm}, _dm{dm} {}

void Catalog::Init() {
    if (IsInitialised()) {
        LoadCatalogs();
        return;
    }

    BootstrapCatalogs();
}

// db operations
db_id_t Catalog::CreateDatabase(
            const std::string& db_name) {
    // insert row into db_databases
}



// table operations
table_id_t Catalog::CreateTable(
        db_id_t db_id,
        const std::string& table_name,
        const std::vector<ColumnInfo>& columns
    ) {
    // insert row into db_tables
    
}


// bootstrapping
bool Catalog::IsInitialised() {
    
}

void Catalog::LoadCatalogs(db_id_t db_id) {
    auto databases_info = LookupTable(db_id, DB_DATABASES_TABLE);
    auto tables_info = LookupTable(db_id, DB_TABLES_TABLE);
    auto attributes_info = LookupTable(db_id, DB_ATTRIBUTES_TABLE);
    auto types_info = LookupTable(db_id, DB_TYPES_TABLE);

    _db_databases.emplace(HeapFile::Open(_bm, _dm, 
                        DB_DATABASES_FILE_ID,
                        databases_info.value_or(INVALID_PAGE_ID).first_page_id));

    _db_tables.emplace(HeapFile::Open(_bm, _dm, 
                        DB_TABLES_FILE_ID,
                        tables_info.value_or(INVALID_PAGE_ID).first_page_id));

    _db_attributes.emplace(HeapFile::Open(_bm, _dm, 
                        DB_ATTRIBUTES_FILE_ID,
                        attributes_info.value_or(INVALID_PAGE_ID).first_page_id));

    _db_types.emplace(HeapFile::Open(_bm, _dm, 
                        DB_TYPES_FILE_ID,
                        types_info.value_or(INVALID_PAGE_ID).first_page_id));
}

void Catalog::BootstrapCatalogs() {
    _db_databases.emplace(HeapFile::Create(_bm, _dm, 
                            DB_DATABASES_FILE_ID));
    _db_tables.emplace(HeapFile::Create(_bm, _dm, 
                            DB_TABLES_FILE_ID));
    _db_attributes.emplace(HeapFile::Create(_bm, _dm, 
                            DB_ATTRIBUTES_FILE_ID));
    _db_types.emplace(HeapFile::Create(_bm, _dm, 
                            DB_TYPES_FILE_ID));

    InsertBuiltinTypes();
    InsertCatalogMetadata();
}


void Catalog::InsertBuiltinTypes() {
    
}

void Catalog::InsertCatalogMetadata() {

}





}