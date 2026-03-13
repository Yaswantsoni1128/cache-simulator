#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <optional>

enum class ReplacementPolicy { LRU, FIFO };

struct AccessResult {
    bool hit;
    std::optional<uint64_t> evicted_tag; // tag of evicted line, if any
};

class Cache {
public:
    Cache(uint64_t cache_size_bytes, uint64_t block_size_bytes, unsigned associativity, ReplacementPolicy rp);
    // Access an address. Returns true if hit.
    AccessResult access(uint64_t address, uint64_t access_id);
    void display() const; // print cache state
    uint64_t get_hits() const;
    uint64_t get_misses() const;
    void reset_stats();

private:
    struct Line {
        bool valid = false;
        uint64_t tag = 0;
        uint64_t timestamp = 0; // for LRU
        uint64_t fifo_order = 0; // for FIFO
    };

    struct Set {
        std::vector<Line> lines;
    };

    uint64_t cache_size_bytes_;
    uint64_t block_size_bytes_;
    unsigned associativity_;
    ReplacementPolicy rp_;

    uint64_t num_sets_;
    uint64_t num_lines_;

    std::vector<Set> sets_;

    uint64_t hits_ = 0;
    uint64_t misses_ = 0;
    uint64_t global_fifo_counter_ = 0;

    // helpers
    uint64_t block_number(uint64_t address) const;
    uint64_t set_index(uint64_t block_num) const;
    uint64_t tag_of(uint64_t block_num) const;
    AccessResult access_set(Set &s, uint64_t tag, uint64_t access_id);
};

// A helper class: fully-associative LRU cache used for miss classification and optional comparison.
class FullyAssociativeLRU {
public:
    FullyAssociativeLRU(uint64_t cache_size_bytes, uint64_t block_size_bytes);
    bool access(uint64_t address, uint64_t access_id);
    void reset();
private:
    struct Line { bool valid=false; uint64_t tag=0; uint64_t timestamp=0; };
    uint64_t block_size_bytes_;
    uint64_t num_lines_;
    std::vector<Line> lines_;
};
