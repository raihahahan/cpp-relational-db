#include "access/heap/heap_file.h"
#include "catalog/catalog_bootstrap.h"
#include <concepts>
#include <vector>
#include <span>

namespace db::catalog {

using HeapFile = db::access::HeapFile;

template <typename Row, typename Codec>
concept CatalogCodec =
    requires(const Row& row, std::span<const uint8_t> bytes) {
        { Codec::Encode(row) } -> std::same_as<std::vector<uint8_t>>;
        { Codec::Decode(bytes) } -> std::same_as<Row>;
    };

template <typename Row, typename Codec>
requires CatalogCodec<Row, Codec>
class CatalogTable {
public:
    explicit CatalogTable(HeapFile hf) : _hf{std::move(hf)} {};
    void Insert(const Row& row) {
        auto bytes = Codec::Encode(row);
        _hf.Insert(bytes.data(), bytes.size());
    }

private:
    HeapFile _hf;
};
}

