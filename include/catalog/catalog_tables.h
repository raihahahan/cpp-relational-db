#pragma once

#include "catalog/catalog_table_base.h"
#include "catalog/catalog_types.h"
#include "catalog/catalog_codec.h"

namespace db::catalog {
// db_databases
class DatabasesCatalog
    : public CatalogTable<DatabaseInfo, codec::DatabaseInfoCodec> {
public:
    using Base = CatalogTable<DatabaseInfo, codec::DatabaseInfoCodec>;
    using Base::Base;

    std::optional<DatabaseInfo> Lookup(std::string_view db_name) const;
};

// db_tables
class TablesCatalog
    : public CatalogTable<TableInfo, codec::TableInfoCodec> {
public:
    using Base = CatalogTable<TableInfo, codec::TableInfoCodec>;
    using Base::Base;

    std::optional<TableInfo> Lookup(db_id_t db_id, std::string_view table_name) const;
};

// db_attributes
class AttributesCatalog
    : public CatalogTable<ColumnInfo, codec::ColumnInfoCodec> {
public:
    using Base = CatalogTable<ColumnInfo, codec::ColumnInfoCodec>;
    using Base::Base;

    std::vector<ColumnInfo> GetColumns(table_id_t table_id) const;
};

// db_types
class TypesCatalog
    : public CatalogTable<TypeInfo, codec::TypeInfoCodec> {
public:
    using Base = CatalogTable<TypeInfo, codec::TypeInfoCodec>;
    using Base::Base;

    std::vector<TypeInfo> GetTypes() const;
};
}
