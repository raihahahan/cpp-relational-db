#pragma once

#include <cstdint>
#include <optional>
#include <span>

namespace db::storage {
using page_id_t = int32_t;

struct Slot {
    uint16_t offset;
    uint16_t length;
};

struct PageHeader {
    uint16_t num_slots;
    uint16_t free_space_offset;
};

class SlottedPage {
public:
    SlottedPage();
    SlottedPage(char* page_data, uint16_t offset);
    SlottedPage(const SlottedPage& other);
    SlottedPage& operator=(const SlottedPage& other);
    ~SlottedPage();

    // initialisers
    // 1. existing page
    static SlottedPage FromBuffer(char* page_data, uint16_t offset);
    // 2. new page
    static void Init(char* page_data, uint16_t offset);
    
    // modifiers
    std::optional<uint16_t> Insert(const char* data, std::size_t len);
    std::optional<std::span<const char>> Get(uint16_t slot_id);
    bool Update(uint16_t slot_id, const char* new_data, std::size_t len);
    bool Delete(uint16_t slot_id);

    size_t FreeSpace() const; 
    uint16_t GetNumSlots();

private:
    PageHeader* GetHeader() const;
    Slot* GetSlot(uint16_t slot_id) const;
    char* _data; // non-owning pointer of actual data
    uint16_t _offset;
};
}