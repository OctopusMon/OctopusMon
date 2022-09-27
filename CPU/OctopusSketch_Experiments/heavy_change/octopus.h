#ifndef _OCTOPUS_H_
#define _OCTOPUS_H_

#include "HeavyPart.h"
#include "coco.h"

template<int bucket_num, int tot_memory_in_bytes>
class OctopusSketch
{
public:
    static constexpr int heavy_mem = bucket_num * sizeof(Bucket);
    static constexpr int light_mem = tot_memory_in_bytes - heavy_mem;
    static constexpr bool all_light = false;

    HeavyPart<bucket_num> heavy_part;
    CocoSketch light_part;

    OctopusSketch(): light_part(light_mem) {}
    ~OctopusSketch(){}

    void clear() {
        heavy_part.clear();
        light_part.clear();
    }

    void insert(TUPLES key, int f = 1) {
        TUPLES swap_key;
        uint32_t swap_val = 0;
        if (all_light) {
            light_part.insert(key, f);
            return;
        }

        int result = heavy_part.insert(key, swap_key, swap_val, f);
        switch(result)
        {
            case 0: return;
            case 1:{
                if(HIGHEST_BIT_IS_1(swap_val))
                    light_part.insert(swap_key, GetCounterVal(swap_val));
                else
                    light_part.insert(swap_key, swap_val);
                return;
            }
            case 2: light_part.insert(key, f);  return;
            default:
                printf("error return value !\n");
                exit(1);
        }
    }

    int query(TUPLES key) {
        uint32_t heavy_result = heavy_part.query(key);
        if(heavy_result == 0 || HIGHEST_BIT_IS_1(heavy_result))
        {
            int light_result = light_part.query(key);
            return (int)GetCounterVal(heavy_result) + light_result;
        }
        return heavy_result;
    }

    int get_bucket_num() { return heavy_part.get_bucket_num(); }

    void *operator new(size_t sz) {
        constexpr uint32_t alignment = 64;
        size_t alloc_size = (2 * alignment + sz) / alignment * alignment;
        void *ptr = ::operator new(alloc_size);
        void *old_ptr = ptr;
        void *new_ptr = ((char*)std::align(alignment, sz, ptr, alloc_size) + alignment);
        ((void **)new_ptr)[-1] = old_ptr;

        return new_ptr;
    }

    void operator delete(void *p) {
        ::operator delete(((void**)p)[-1]);
    }

    HashMap query_all() {
        HashMap mp1 = heavy_part.query_all(), mp2 = light_part.query_all();
        for (auto it = mp1.begin(); it != mp1.end(); it++) {
            mp2[it->first] += GetCounterVal(it->second);
        }
        return mp2;
    }

    uint32_t query_partial_key(TUPLES key, KeyType type, uint32_t mask=0xffffffff) {
        if (type == five_tuples)
            return query(key);
        HashMap mp = query_all();
        uint32_t ans = 0;
        for (auto it = mp.begin(); it != mp.end(); it++) {
            if (
                type == srcIP_dstIP   && key.srcIP() == it->first.srcIP() && key.dstIP()   == it->first.dstIP()   ||
                type == srcIP_srcPort && key.srcIP() == it->first.srcIP() && key.srcPort() == it->first.srcPort() ||
                type == dstIP_dstPort && key.dstIP() == it->first.dstIP() && key.dstPort() == it->first.dstPort() ||
                type == srcIP         && (key.srcIP() & mask) == (it->first.srcIP() & mask) ||
                type == dstIP         && (key.dstIP() & mask) == (it->first.dstIP() & mask)
            ) {
                ans += it->second;
            }
        }
        return ans;
    }
};

#endif // _OCTOPUS_H_
