#include "catalog/catalog.h"
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

// db operations
db_id_t Catalog::CreateDatabase(const std::string& db_name) { 
    // insert row into db_databases 
    db_id_t db_id = AllocateDatabaseId();
    _databases.value().Insert(DatabaseInfo{db_id, db_name});
    return db_id;
}

std::optional<DatabaseInfo> Catalog::GetDatabase(const std::string& db_name) const {
    auto db_info = _databases.value().Lookup(db_name);
    return db_info;
}

// table operations
table_id_t Catalog::CreateTable(
        db_id_t db_id,
        const std::string& table_name,
        const std::vector<ColumnInfo>& columns
) {
    table_id_t table_id = AllocateTableId();
    file_id_t fid = AllocateFileId();
    auto hf = HeapFile::Create(_bm, _dm, fid);
    _tables.value().Insert(TableInfo{
                                table_id,
                                db_id,
                                table_name,
                                fid,
                                hf.GetPageId()
                            });
}

std::optional<TableInfo> Catalog::LookupTable(
                        db_id_t db_id,
                        const std::string& table_name
) const {
    return _tables.value().Lookup(db_id, table_name);
}

std::vector<ColumnInfo> Catalog::GetTableColumns(table_id_t table_id) const {
    return _attributes.value().GetColumns(table_id);
}

// bootstrapping
bool Catalog::IsInitialised() const {

}

void Catalog::LoadCatalogs(db_id_t db_id) {
    auto databases_info = LookupTable(db_id, DB_DATABASES_TABLE);
    auto tables_info = LookupTable(db_id, DB_TABLES_TABLE);
    auto attributes_info = LookupTable(db_id, DB_ATTRIBUTES_TABLE);
    auto types_info = LookupTable(db_id, DB_TYPES_TABLE);

    _databases.emplace(DatabasesCatalog(&HeapFile::Open(
        _bm, 
        _dm, 
        DB_DATABASES_FILE_ID,
        databases_info.value_or(INVALID_PAGE_ID).first_page_id
    )));
    
    _tables.emplace(TablesCatalog(&HeapFile::Open(
        _bm, 
        _dm, 
        DB_TABLES_FILE_ID,
        tables_info.value_or(INVALID_PAGE_ID).first_page_id
    )));
    
    _attributes.emplace(AttributesCatalog(&HeapFile::Open(
        _bm, 
        _dm, 
        DB_ATTRIBUTES_FILE_ID,
        attributes_info.value_or(INVALID_PAGE_ID).first_page_id
    )));
    
    _types.emplace(TypesCatalog(&HeapFile::Open(
        _bm, 
        _dm, 
        DB_TYPES_FILE_ID,
        types_info.value_or(INVALID_PAGE_ID).first_page_id
    )));
}

void Catalog::BootstrapCatalogs() {
    _databases.emplace(DatabasesCatalog(&HeapFile::Create(_bm, _dm, DB_DATABASES_FILE_ID)));
    _tables.emplace(TablesCatalog(&HeapFile::Create(_bm, _dm, DB_TABLES_FILE_ID)));
    _attributes.emplace(AttributesCatalog(&HeapFile::Create(_bm, _dm, DB_ATTRIBUTES_FILE_ID)));
    _types.emplace(TypesCatalog(&HeapFile::Create(_bm, _dm, DB_TYPES_FILE_ID)));

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
    // db_databases
    _tables.value().Insert(TableInfo{
        .table_id = DB_DATABASES_TABLE_ID,
        .db_id = DEFAULT_DB_ID,
        .table_name = DB_DATABASES_TABLE,
        .heap_file_id = DB_DATABASES_FILE_ID,
        .first_page_id = _databases->GetFirstPage()
    });

    _attributes.value().Insert({DB_DATABASES_TABLE_ID, 1, "db_id", 1, 1});
    _attributes.value().Insert({DB_DATABASES_TABLE_ID, 2, "db_name", 2, 2});

    // db_tables
    _tables.value().Insert(TableInfo{
        .table_id = DB_TABLES_TABLE_ID,
        .db_id = DEFAULT_DB_ID,
        .table_name = DB_TABLES_TABLE,
        .heap_file_id = DB_TABLES_FILE_ID,
        .first_page_id = _tables->GetFirstPage()
    });

    _attributes.value().Insert({DB_TABLES_TABLE_ID, 1, "table_id", 1, 1});
    _attributes.value().Insert({DB_TABLES_TABLE_ID, 2, "db_id", 1, 2});
    _attributes.value().Insert({DB_TABLES_TABLE_ID, 3, "table_name", 2, 3});
    _attributes.value().Insert({DB_TABLES_TABLE_ID, 4, "heap_file_id", 1, 4});
    _attributes.value().Insert({DB_TABLES_TABLE_ID, 5, "first_page_id", 1, 5});

    // db_attributes
    _tables.value().Insert(TableInfo{
        .table_id = DB_ATTRIBUTES_TABLE_ID,
        .db_id = DEFAULT_DB_ID,
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
        .db_id = DEFAULT_DB_ID,
        .table_name = DB_TYPES_TABLE,
        .heap_file_id = DB_TYPES_FILE_ID,
        .first_page_id = _types->GetFirstPage()
    });

    _attributes.value().Insert({DB_TYPES_TABLE_ID, 1, "type_id", 1, 1});
    _attributes.value().Insert({DB_TYPES_TABLE_ID, 2, "size", 1, 2});

    // insert default database
    _databases.value().Insert(DatabaseInfo{
        .db_id = DEFAULT_DB_ID,
        .db_name = DEFAULT_DB_NAME
    });
}
}