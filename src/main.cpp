#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cctype>
#include <iomanip>
#include <unordered_set>
#include <vector>
#include "../include/cache.h"

using namespace std;

struct Stats {
    uint64_t hits = 0;
    uint64_t misses = 0;
    uint64_t compulsory = 0;
    uint64_t conflict = 0;
    uint64_t capacity = 0;
};

static void usage(const char *prog) {
    cout << "Usage: " << prog << " -i input.txt [-c cache_bytes] [-b block_bytes] [-a associativity] [-r lru|fifo] [-s] [--compare]\n";
}

int main(int argc, char **argv) {
    string infile;
    uint64_t cache_bytes = 16 * 1024; // 16KB default
    uint64_t block_bytes = 64; // 64B default
    unsigned associativity = 4;
    string rp_str = "lru";
    bool step_mode = false;
    bool compare = false;

    // parse args (simple)
    for (int i = 1; i < argc; ++i) {
        string s = argv[i];
        if (s == "-i" && i+1 < argc) infile = argv[++i];
        else if (s == "-c" && i+1 < argc) cache_bytes = stoull(argv[++i]);
        else if (s == "-b" && i+1 < argc) block_bytes = stoull(argv[++i]);
        else if (s == "-a" && i+1 < argc) associativity = (unsigned)stoul(argv[++i]);
        else if (s == "-r" && i+1 < argc) rp_str = argv[++i];
        else if (s == "-s") step_mode = true;
        else if (s == "--compare") compare = true;
        else { usage(argv[0]); return 1; }
    }
    if (infile.empty()) { usage(argv[0]); return 1; }

    ReplacementPolicy rp = ReplacementPolicy::LRU;
    if (rp_str == "fifo") rp = ReplacementPolicy::FIFO;

    cout << "Cache Simulator\n";
    cout << "Cache size: " << cache_bytes << " bytes, block: " << block_bytes << ", associativity: " << associativity << ", policy: " << rp_str << "\n";
    if (compare) cout << "Compare mode: running both LRU and FIFO on the same trace\n";

    Cache cache(cache_bytes, block_bytes, associativity, rp);
    FullyAssociativeLRU fa(cache_bytes, block_bytes); // for classification

    // for compare mode
    Cache cache_alt(cache_bytes, block_bytes, associativity, (rp == ReplacementPolicy::LRU) ? ReplacementPolicy::FIFO : ReplacementPolicy::LRU);
    FullyAssociativeLRU fa_alt(cache_bytes, block_bytes);

    ifstream fin(infile);
    if (!fin) { cerr << "cannot open input file: " << infile << "\n"; return 1; }

    unordered_set<uint64_t> seen_blocks;
    Stats stats;
    Stats stats_alt; // for compare

    string line;
    uint64_t access_id = 1;
    while (getline(fin, line)) {
        // trim and skip
        if (line.empty()) continue;
        // line format: R 0xADDR or W ADDR
        istringstream iss(line);
        string op; string addr_s;
        if (!(iss >> op >> addr_s)) continue;
        // parse address (hex or decimal)
        uint64_t addr = 0;
        try {
            if (addr_s.rfind("0x", 0) == 0 || addr_s.rfind("0X", 0) == 0) {
                addr = stoull(addr_s, nullptr, 16);
            } else {
                addr = stoull(addr_s, nullptr, 10);
            }
        } catch (...) { cerr << "bad address: " << addr_s << "\n"; continue; }

        uint64_t bnum = addr / block_bytes;
        bool first_time = (seen_blocks.find(bnum) == seen_blocks.end());
        if (first_time) seen_blocks.insert(bnum);

        // primary cache
        auto res = cache.access(addr, access_id);
        if (res.hit) stats.hits++; else stats.misses++;

        // classification: use fully-assoc LRU
        bool fa_hit = fa.access(addr, access_id);
        if (!fa_hit) {
            if (first_time) stats.compulsory++;
            else stats.capacity++; // fully-assoc also misses => capacity
        } else {
            if (!res.hit && fa_hit) {
                // primary missed but fully-assoc hit => conflict
                stats.conflict++;
            }
        }

        // compare run
        if (compare) {
            auto res_alt = cache_alt.access(addr, access_id);
            if (res_alt.hit) stats_alt.hits++; else stats_alt.misses++;
            fa_alt.access(addr, access_id); // advance ideal
        }

        if (step_mode) {
            cout << access_id << ": " << op << " " << addr_s << " -> " << (res.hit ? "HIT" : "MISS");
            if (res.evicted_tag) cout << ", evicted tag=0x" << hex << *res.evicted_tag << dec;
            cout << "\n";
            cache.display();
            cout << "\n";
            // pause - simple prompt
            cout << "Press Enter to continue...";
            cin.get();
        }

        ++access_id;
    }

    cout << "\n=== Results ===\n";
    cout << "Hits: " << stats.hits << "\n";
    cout << "Misses: " << stats.misses << "\n";
    double hit_rate = (stats.hits + stats.misses) ? (double)stats.hits / (double)(stats.hits + stats.misses) : 0.0;
    cout << fixed << setprecision(4);
    cout << "Hit rate: " << (hit_rate * 100.0) << "%\n";
    cout << "Miss classification: compulsory=" << stats.compulsory << ", conflict=" << stats.conflict << ", capacity=" << stats.capacity << "\n";

    if (compare) {
        cout << "\n=== Comparison (primary vs alternative) ===\n";
        cout << "Primary (" << rp_str << ") hits=" << stats.hits << ", misses=" << stats.misses << ", hit_rate=" << ((double)stats.hits/(stats.hits+stats.misses)*100.0) << "%\n";
        string alt_name = (rp == ReplacementPolicy::LRU) ? "fifo" : "lru";
        cout << "Alternative (" << alt_name << ") hits=" << stats_alt.hits << ", misses=" << stats_alt.misses << ", hit_rate=" << ((double)stats_alt.hits/(stats_alt.hits+stats_alt.misses)*100.0) << "%\n";
    }

    cout << "\nFinal cache state:\n";
    cache.display();

    return 0;
}
