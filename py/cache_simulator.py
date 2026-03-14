#!/usr/bin/env python3
"""
Simple Cache Simulator (Python)
Features: LRU and FIFO replacement, miss classification via fully-associative LRU, compare mode, step mode.
Usage similar to the C++ version.
"""
import argparse
from collections import deque
import math
import sys

class Line:
    def __init__(self):
        self.valid = False
        self.tag = None
        self.timestamp = 0
        self.fifo_order = 0

class Set:
    def __init__(self, ways):
        self.lines = [Line() for _ in range(ways)]

class Cache:
    def __init__(self, cache_bytes, block_bytes, associativity, policy='lru'):
        if block_bytes <= 0:
            raise ValueError('block size must be > 0')
        self.cache_bytes = cache_bytes
        self.block_bytes = block_bytes
        self.assoc = associativity
        self.policy = policy.lower()
        self.num_lines = max(1, cache_bytes // block_bytes)
        self.num_sets = max(1, self.num_lines // max(1, associativity))
        self.sets = [Set(self.assoc) for _ in range(self.num_sets)]
        self.hits = 0
        self.misses = 0
        self.global_fifo_counter = 0

    def block_number(self, addr):
        return addr // self.block_bytes
    def set_index(self, block_num):
        return block_num % self.num_sets
    def tag_of(self, block_num):
        return block_num // self.num_sets

    def access(self, addr, access_id):
        bnum = self.block_number(addr)
        idx = self.set_index(bnum)
        tag = self.tag_of(bnum)
        res = self._access_set(self.sets[idx], tag, access_id)
        if res['hit']:
            self.hits += 1
        else:
            self.misses += 1
        return res

    def _access_set(self, s, tag, access_id):
        # search
        for l in s.lines:
            if l.valid and l.tag == tag:
                if self.policy == 'lru':
                    l.timestamp = access_id
                return {'hit': True, 'evicted_tag': None}
        # find empty
        for l in s.lines:
            if not l.valid:
                l.valid = True
                l.tag = tag
                l.timestamp = access_id
                self.global_fifo_counter += 1
                l.fifo_order = self.global_fifo_counter
                return {'hit': False, 'evicted_tag': None}
        # evict
        evict_idx = 0
        if self.policy == 'lru':
            oldest = float('inf')
            for i, l in enumerate(s.lines):
                if l.timestamp < oldest:
                    oldest = l.timestamp
                    evict_idx = i
        else: # fifo
            smallest = float('inf')
            for i, l in enumerate(s.lines):
                if l.fifo_order < smallest:
                    smallest = l.fifo_order
                    evict_idx = i
        evicted = s.lines[evict_idx].tag
        s.lines[evict_idx].tag = tag
        s.lines[evict_idx].timestamp = access_id
        self.global_fifo_counter += 1
        s.lines[evict_idx].fifo_order = self.global_fifo_counter
        return {'hit': False, 'evicted_tag': evicted}

    def display(self):
        print(f"Cache state: (sets={self.num_sets}, associativity={self.assoc})")
        for si, s in enumerate(self.sets):
            parts = []
            for l in s.lines:
                if l.valid:
                    parts.append(f"[tag=0x{l.tag:x},t={l.timestamp}]")
                else:
                    parts.append('[empty]')
            print(f"Set {si}: {' '.join(parts)}")

class FullyAssociativeLRU:
    def __init__(self, cache_bytes, block_bytes):
        if block_bytes <= 0:
            raise ValueError('block size must be > 0')
        self.block_bytes = block_bytes
        self.num_lines = max(1, cache_bytes // block_bytes)
        self.lines = []  # list of (blocknum, timestamp)

    def access(self, addr, access_id):
        bnum = addr // self.block_bytes
        for i, (bn, ts) in enumerate(self.lines):
            if bn == bnum:
                # update timestamp
                self.lines[i] = (bn, access_id)
                return True
        # miss
        if len(self.lines) < self.num_lines:
            self.lines.append((bnum, access_id))
            return False
        # evict LRU
        oldest_idx = 0
        oldest_ts = float('inf')
        for i, (bn, ts) in enumerate(self.lines):
            if ts < oldest_ts:
                oldest_ts = ts
                oldest_idx = i
        self.lines[oldest_idx] = (bnum, access_id)
        return False

def parse_address(s):
    s = s.strip()
    if s.lower().startswith('0x'):
        return int(s, 16)
    return int(s, 10)

def main():
    parser = argparse.ArgumentParser(description='Cache Simulator (Python)')
    parser.add_argument('-i', required=True, help='input trace file')
    parser.add_argument('-c', type=int, default=16*1024, help='cache size bytes')
    parser.add_argument('-b', type=int, default=64, help='block size bytes')
    parser.add_argument('-a', type=int, default=4, help='associativity')
    parser.add_argument('-r', choices=['lru','fifo'], default='lru', help='replacement policy')
    parser.add_argument('-s', action='store_true', help='step mode')
    parser.add_argument('--compare', action='store_true', help='compare LRU vs FIFO')
    args = parser.parse_args()

    cache = Cache(args.c, args.b, args.a, args.r)
    fa = FullyAssociativeLRU(args.c, args.b)
    if args.compare:
        alt_policy = 'fifo' if args.r == 'lru' else 'lru'
        cache_alt = Cache(args.c, args.b, args.a, alt_policy)
        fa_alt = FullyAssociativeLRU(args.c, args.b)
    else:
        cache_alt = None
        fa_alt = None

    seen_blocks = set()
    stats = {'hits':0,'misses':0,'compulsory':0,'conflict':0,'capacity':0}
    stats_alt = {'hits':0,'misses':0}

    access_id = 1
    try:
        with open(args.i, 'r') as f:
            for line in f:
                line = line.strip()
                if not line or line.startswith('#'):
                    continue
                parts = line.split()
                if len(parts) < 2:
                    continue
                op, addr_s = parts[0], parts[1]
                try:
                    addr = parse_address(addr_s)
                except Exception:
                    print(f"bad address: {addr_s}", file=sys.stderr)
                    continue
                bnum = addr // args.b
                first_time = bnum not in seen_blocks
                if first_time:
                    seen_blocks.add(bnum)

                res = cache.access(addr, access_id)
                if res['hit']:
                    stats['hits'] += 1
                else:
                    stats['misses'] += 1

                fa_hit = fa.access(addr, access_id)
                if not fa_hit:
                    if first_time:
                        stats['compulsory'] += 1
                    else:
                        stats['capacity'] += 1
                else:
                    if not res['hit'] and fa_hit:
                        stats['conflict'] += 1

                if args.compare:
                    res_alt = cache_alt.access(addr, access_id)
                    if res_alt['hit']:
                        stats_alt['hits'] += 1
                    else:
                        stats_alt['misses'] += 1
                    fa_alt.access(addr, access_id)

                if args.s:
                    print(f"{access_id}: {op} {addr_s} -> {'HIT' if res['hit'] else 'MISS'}", end='')
                    if res['evicted_tag'] is not None:
                        print(f", evicted_tag=0x{res['evicted_tag']:x}", end='')
                    print()
                    cache.display()
                    input('Press Enter to continue...')

                access_id += 1
    except FileNotFoundError:
        print('cannot open input file', args.i, file=sys.stderr)
        sys.exit(1)

    total = stats['hits'] + stats['misses']
    hit_rate = (stats['hits'] / total) * 100.0 if total > 0 else 0.0

    print('\n=== Results ===')
    print('Hits:', stats['hits'])
    print('Misses:', stats['misses'])
    print(f'Hit rate: {hit_rate:.4f}%')
    print(f"Miss classification: compulsory={stats['compulsory']}, conflict={stats['conflict']}, capacity={stats['capacity']}")

    if args.compare:
        total_alt = stats_alt['hits'] + stats_alt['misses']
        hit_rate_alt = (stats_alt['hits'] / total_alt) * 100.0 if total_alt > 0 else 0.0
        print('\n=== Comparison ===')
        print(f"Primary ({args.r}) hits={stats['hits']}, misses={stats['misses']}, hit_rate={hit_rate:.4f}%")
        print(f"Alternative ({alt_policy}) hits={stats_alt['hits']}, misses={stats_alt['misses']}, hit_rate={hit_rate_alt:.4f}%")

    print('\nFinal cache state:')
    cache.display()

if __name__ == '__main__':
    main()
