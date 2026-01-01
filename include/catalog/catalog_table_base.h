#include "access/heap/heap_file.h"
#include "catalog/catalog_bootstrap.h"
#include "model/relation.h"
#include <concepts>
#include <vector>
#include <span>

namespace db::catalog {

using HeapFile = db::access::HeapFile;
using Relation = model::Relation;

template <typename Row, typename Codec>
concept CatalogCodec =
    requires(const Row& row, std::span<const uint8_t> bytes) {
        { Codec::Encode(row) } -> std::same_as<std::vector<uint8_t>>;
        { Codec::Decode(bytes) } -> std::same_as<Row>;
    };

template <typename Row, typename Codec>
requires CatalogCodec<Row, Codec>
class CatalogTable : public model::Relation {
    static_assert(!std::is_trivially_copyable_v<Row>,
        "Catalog rows must use explicit codecs, not memcpy");
public:
    explicit CatalogTable(HeapFile hf) : Relation{std::move(hf)} {};
    std::optional<db::access::RID> Insert(const Row& row) {
        auto bytes = Codec::Encode(row);
        auto rid = InsertRaw(bytes, bytes.size());
        return rid;
    }
    page_id_t GetFirstPage() const {
        return _hf.GetPageId();
    }

};
}

