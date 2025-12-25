#include "storage/page/slotted_page.h"
#include "config/config.h"
#include <optional>

namespace db::storage {

/*
+------------------------+  <- page start
| Base offset            |
+------------------------+ 
| PageHeader             |
| - num_slots            |
| - free_space_offset    | 
+------------------------+
| SlotDirectory[]        |  grows upward
| (offset, length)       |
+------------------------+
|        FREE SPACE      |
+------------------------+
| Record Data            |  grows downward
+------------------------+  <- page end

lower address               higher address
┌─────────────────────────────────┐
page_data[0]          page_data[PAGE_SIZE-1]

- grows upward: increasing addresses towards the end of the page
- grows downward: decreasing addresses towards the beginning of the page
*/

SlottedPage::SlottedPage(char* page_data, uint16_t offset) : 
                            _data{page_data}, _offset{offset} {};
SlottedPage::SlottedPage() : _data{nullptr}, _offset{0} {};
SlottedPage::SlottedPage(const SlottedPage& other) : 
                            _data{other._data}, _offset{other._offset} {};
SlottedPage& SlottedPage::operator=(const SlottedPage& other) {
    if (this != &other) {
        _data = other._data;
        _offset = other._offset;
    }

    return *this;
};

SlottedPage::~SlottedPage() = default;

SlottedPage SlottedPage::FromBuffer(char* page_data, uint16_t offset) {
    SlottedPage sp{page_data, offset};
    return sp;
};

void SlottedPage::Init(char* page_data, uint16_t offset) {
    auto* header = reinterpret_cast<PageHeader*>(page_data + offset);
    header->num_slots = 0;
    header->free_space_offset = config::PAGE_SIZE;
};

std::optional<uint16_t> SlottedPage::Insert(const char* data, std::size_t len) {
    // required new space:
    // 1. additional slot: sizeof(Slot)
    // 2. additional data: len
    if (FreeSpace() < len + sizeof(Slot)) return std::nullopt;
    auto* header = GetHeader();

    // 1. write data to start_offset + len
    header->free_space_offset -= len;
    std::memcpy(_data + header->free_space_offset, data, len);

    // 2. create new slot
    uint16_t slot_offset = _offset + sizeof(PageHeader) + 
                                header->num_slots * sizeof(Slot);
    Slot new_slot{header->free_space_offset, static_cast<uint16_t>(len)};
    std::memcpy(_data + slot_offset, &new_slot, sizeof(Slot));
    uint16_t slot_id = header->num_slots;
    header->num_slots++;

    return slot_id;
};

std::optional<std::span<const char>> SlottedPage::Get(uint16_t slot_id) {
    auto* header = GetHeader();
    // check if slot is valid
    if (slot_id >= header->num_slots) return std::nullopt;

    auto* slot = GetSlot(slot_id);
    if (slot->length == 0) {
        return std::nullopt; // deleted
    }

    return std::span<const char>(_data + slot->offset, slot->length);
};

bool SlottedPage::Update(uint16_t slot_id, const char* new_data, std::size_t len) {
    auto* header = GetHeader();
    // check if slot is valid
    if (slot_id >= header->num_slots) return false;
    
    auto* slot = GetSlot(slot_id);
    
    if (slot->length >= len) {
        // case 1: old data is larger than or equal to new data
        // if larger than, then new data will have garbage values
        std::memcpy(_data + slot->offset, new_data, len);
        slot->length = len;
    } else {
        // case 2: need to bring to new offset
        if (FreeSpace() < len) return false;

        // 1. update slot
        header->free_space_offset -= len;
        slot->offset = header->free_space_offset;
        slot->length = len;

        // 2. create data
        std::memcpy(_data + header->free_space_offset, new_data, len);
    }

    return true;
};

bool SlottedPage::Delete(uint16_t slot_id) {
    auto* header = GetHeader();
    if (slot_id >= header->num_slots) return false;

    auto* slot = GetSlot(slot_id);
    slot->length = 0;
    return true;
};

PageHeader* SlottedPage::GetHeader() const {
    return reinterpret_cast<PageHeader*>(_data + _offset);
};

Slot* SlottedPage::GetSlot(uint16_t slot_id) const {
    return reinterpret_cast<Slot*>(_data + _offset + sizeof(PageHeader) + 
                                                slot_id * sizeof(Slot));
};

size_t SlottedPage::FreeSpace() const {
    auto* header = reinterpret_cast<PageHeader*>(_data);
    size_t slot_dir_end =
        _offset + sizeof(PageHeader) + header->num_slots * sizeof(Slot);

    return header->free_space_offset - slot_dir_end;
};

uint16_t SlottedPage::GetNumSlots() {
    auto* header = GetHeader();
    return header->num_slots;
}
}