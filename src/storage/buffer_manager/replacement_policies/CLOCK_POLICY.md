# CLOCK Replacement Policy

The **CLOCK** replacement policy is the default page-eviction algorithm used by the buffer manager. It implements an efficient approximation of Least Recently Used (LRU) without the overhead of maintaining full LRU lists. CLOCK is widely used in real database systems due to its simplicity, low memory overhead, and good practical performance under mixed workloads.

This module contains the following:

- `IReplacementPolicy` — abstract strategy interface
- `ClockPolicy` — an implementation of CLOCK usable by the `BufferManager`

The policy integrates with the BufferManager through the Strategy Pattern, allowing CLOCK to be substituted with LRU, LFU, RANDOM, or other policies without modifying the BufferManager itself.

## 1. Overview

CLOCK approximates LRU by tracking a single boolean reference bit per frame and rotating a conceptual “clock hand” around the frame array. Whenever a page is accessed, its reference bit is set. During eviction, CLOCK scans sequentially until it finds a frame whose reference bit is clear and whose pin count is zero.

![CLOCK](docs/clock.png)

The algorithm guarantees:

- O(1) update cost on page access
- O(n) worst-case scan time (usually small in practice)
- No heap allocations or linked lists
- Data structures proportional to buffer pool size

CLOCK is designed specifically for page replacement in buffer pools where pin/unpin semantics are respected.

## 2. Data Structures

The `ClockPolicy` maintains:

```cpp
std::vector<int> ref_bits;  // reference bits for each frame
size_t hand;                 // current position of the clock hand
```

The policy stores no state inside `Frame` itself. All metadata is kept internal to the policy.

## 3. Integration with the BufferManager

The BufferManager notifies the policy at certain lifecycle events:

- `record_acess(frame)`
  Called when a page is accessed or pinned on a buffer hit.

- `record_load(frame)`
  Called when a page is loaded into a frame for the first time.

- `record_unpin(frame)`
  Called when a frame’s pin count decreases; may become an eviction candidate.

The BufferManager calls:

- `choose_victim(frame_ptrs)`
  When eviction is required.

The policy returns a pointer to a frame that has `pin_count == 0` and whose reference bit has been cleared, marking it as a suitable victim.

## 4. Algorithm

### 4.1 Access and Load

On any page access (hit or load):

```
ref_bits[frame_id] = 1
```

This marks the frame as recently used.

### 4.2 Eviction

When eviction is necessary, CLOCK proceeds as follows:

1. Inspect the frame at the current clock hand.
2. If `pin_count > 0`, skip the frame.
3. If its reference bit is `1`, clear the bit and advance the hand.
4. If its reference bit is `0` and the frame is unpinned, select it as the victim.

The algorithm continues cycling until a victim is found.

## 5. ClockPolicy Interface

A simplified version:

```cpp
class ClockPolicy : public IReplacementPolicy {
public:
    explicit ClockPolicy(size_t pool_size);

    void record_access(Frame* f) override;
    void record_load(Frame* f) override;
    void record_unpin(Frame* f) override;

    Frame* choose_victim(const std::vector<Frame*>& frames) override;

private:
    std::vector<int> ref_bits;
    size_t hand;
};
```

### Key behaviors:

- `record_access`
  Sets the reference bit upon page hit.

- `record_load`
  Clears the reference bit when a frame is loaded (new page).

- `record_unpin`
  CLOCK does not require special handling here; frame may become a candidate.

- `choose_victim`
  Performs the CLOCK sweep until it finds a frame with:

  - `ref_bits[id] == 1`
  - `pin_count == 0`

## 6. Complexity and Characteristics

### Time Complexity

| Operation      | Cost      |
| -------------- | --------- |
| Access/Load    | O(1)      |
| Eviction (avg) | O(1)–O(n) |
| Eviction worst | O(n)      |

CLOCK typically performs significantly better than naive LRU because:

- It avoids map updates
- It avoids two-way linked lists
- It avoids timestamp maintenance

### Memory Overhead

- One int per frame
- One pointer-sized clock hand

## 7. Summary

The CLOCK replacement policy provides:

- An efficient and lightweight approximation of LRU
- Simple and fast metadata management
- Strong real-world performance
- Seamless integration with BufferManager through the Strategy Pattern
- Ability to switch to other policies (LRU, LFU, RANDOM) without modifying BufferManager
