#include "../include/cache.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>

using namespace std;

Cache::Cache(uint64_t cache_size_bytes, uint64_t block_size_bytes, unsigned associativity, ReplacementPolicy rp)
    : cache_size_bytes_(cache_size_bytes),
      block_size_bytes_(block_size_bytes),
      associativity_(associativity),
      rp_(rp)
{
    if (block_size_bytes_ == 0) throw invalid_argument("block size must be > 0");
    num_lines_ = cache_size_bytes_ / block_size_bytes_;
    if (num_lines_ == 0) throw invalid_argument("cache size must be at least one block");
    if (associativity_ == 0) throw invalid_argument("associativity must be > 0");
    num_sets_ = num_lines_ / associativity_;
    if (num_sets_ == 0) num_sets_ = 1; // fully associative fallback
    sets_.resize(num_sets_);
    for (auto &s : sets_) s.lines.resize(associativity_);
}

uint64_t Cache::block_number(uint64_t address) const {
    return address / block_size_bytes_;
}
uint64_t Cache::set_index(uint64_t block_num) const {
    return (block_num) % num_sets_;
}
uint64_t Cache::tag_of(uint64_t block_num) const {
    return block_num / num_sets_;
}

AccessResult Cache::access(uint64_t address, uint64_t access_id) {
    uint64_t bnum = block_number(address);
    uint64_t idx = set_index(bnum);
    uint64_t tag = tag_of(bnum);
    AccessResult res = access_set(sets_[idx], tag, access_id);
    if (res.hit) ++hits_; else ++misses_;
    return res;
}

AccessResult Cache::access_set(Set &s, uint64_t tag, uint64_t access_id) {
    // search for tag
    for (auto &l : s.lines) {
        if (l.valid && l.tag == tag) {
            // hit
            if (rp_ == ReplacementPolicy::LRU) l.timestamp = access_id;
            return {true, nullopt};
        }
    }
    // miss: find an empty line
    for (auto &l : s.lines) {
        if (!l.valid) {
            l.valid = true;
            l.tag = tag;
            l.timestamp = access_id;
            l.fifo_order = ++global_fifo_counter_;
            return {false, nullopt};
        }
    }
    // need to evict
    size_t evict_idx = 0;
    if (rp_ == ReplacementPolicy::LRU) {
        uint64_t oldest = UINT64_MAX;
        for (size_t i = 0; i < s.lines.size(); ++i) {
            if (s.lines[i].timestamp < oldest) { oldest = s.lines[i].timestamp; evict_idx = i; }
        }
    } else { // FIFO
        uint64_t smallest = UINT64_MAX;
        for (size_t i = 0; i < s.lines.size(); ++i) {
            if (s.lines[i].fifo_order < smallest) { smallest = s.lines[i].fifo_order; evict_idx = i; }
        }
    }
    uint64_t evicted = s.lines[evict_idx].tag;
    s.lines[evict_idx].tag = tag;
    s.lines[evict_idx].timestamp = access_id;
    s.lines[evict_idx].fifo_order = ++global_fifo_counter_;
    return {false, evicted};
}

void Cache::display() const {
    cout << "Cache state: (sets=" << num_sets_ << ", associativity=" << associativity_ << ")\n";
    for (uint64_t si = 0; si < num_sets_; ++si) {
        cout << "Set " << si << ": ";
        const auto &s = sets_[si];
        for (const auto &l : s.lines) {
            if (l.valid) cout << "[tag=" << hex << l.tag << dec << ",t=" << l.timestamp << "] ";
            else cout << "[empty] ";
        }
        cout << "\n";
    }
}

uint64_t Cache::get_hits() const { return hits_; }
uint64_t Cache::get_misses() const { return misses_; }
void Cache::reset_stats() { hits_ = misses_ = 0; }

// FullyAssociativeLRU
FullyAssociativeLRU::FullyAssociativeLRU(uint64_t cache_size_bytes, uint64_t block_size_bytes)
    : block_size_bytes_(block_size_bytes)
{
    if (block_size_bytes_ == 0) throw invalid_argument("block size must be > 0");
    num_lines_ = cache_size_bytes / block_size_bytes_;
    if (num_lines_ == 0) num_lines_ = 1;
    lines_.resize(num_lines_);
}

bool FullyAssociativeLRU::access(uint64_t address, uint64_t access_id) {
    uint64_t bnum = address / block_size_bytes_;
    // search
    for (auto &l : lines_) {
        if (l.valid && l.tag == bnum) {
            l.timestamp = access_id;
            return true;
        }
    }
    // miss: empty
    for (auto &l : lines_) {
        if (!l.valid) {
            l.valid = true; l.tag = bnum; l.timestamp = access_id; return false;
        }
    }
    // evict LRU
    size_t evict_idx = 0; uint64_t oldest = UINT64_MAX;
    for (size_t i = 0; i < lines_.size(); ++i) {
        if (lines_[i].timestamp < oldest) { oldest = lines_[i].timestamp; evict_idx = i; }
    }
    lines_[evict_idx].tag = bnum;
    lines_[evict_idx].timestamp = access_id;
    return false;
}

void FullyAssociativeLRU::reset() {
    for (auto &l : lines_) l.valid = false;
}
