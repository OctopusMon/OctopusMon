#ifndef _HEAVYPART_H_
#define _HEAVYPART_H_

#include "param.h"

template<int bucket_num>
class HeavyPart
{
public:
    Bucket buckets[bucket_num];
    HeavyPart() { this->clear(); }
    ~HeavyPart() {}

    void clear() { memset(buckets, 0, sizeof(Bucket) * bucket_num); }

    int insert(TUPLES key, TUPLES &swap_key, uint32_t &swap_val, uint32_t f = 1) {
        int pos = BOBHash(key) % bucket_num;
        uint32_t min_counter_val;
        int min_counter;

        do{
            int matched = -1, empty = -1;
            min_counter = 0;
            min_counter_val = GetCounterVal(buckets[pos].val[0]);
            for(int i = 0; i < COUNTER_PER_BUCKET - 1; i++){
                if(buckets[pos].key[i] == key){
                    matched = i;
                    break;
                }
                if(buckets[pos].key[i].empty() && empty == -1)
                    empty = i;
                if(min_counter_val > GetCounterVal(buckets[pos].val[i])){
                    min_counter = i;
                    min_counter_val = GetCounterVal(buckets[pos].val[i]);
                }
            }

            if(matched != -1){
                buckets[pos].val[matched] += f;
                return 0;
            }

            if(empty != -1){
                buckets[pos].key[empty] = key;
                buckets[pos].val[empty] = f;
                return 0;
            }
        }while(0);

        uint32_t guard_val = buckets[pos].val[MAX_VALID_COUNTER];
        guard_val = UPDATE_GUARD_VAL(guard_val);

        if(!JUDGE_IF_SWAP(GetCounterVal(min_counter_val), guard_val)){
            buckets[pos].val[MAX_VALID_COUNTER] = guard_val;
            return 2;
        }

        swap_key = buckets[pos].key[min_counter];
        swap_val = buckets[pos].val[min_counter];

        buckets[pos].val[MAX_VALID_COUNTER] = 0;
        buckets[pos].key[min_counter] = key;
        buckets[pos].val[min_counter] = 0x80000001;

        return 1;
    }

    int query(TUPLES key) {
        int pos = BOBHash(key) % bucket_num;

        for(int i = 0; i < MAX_VALID_COUNTER; ++i)
            if(buckets[pos].key[i] == key)
                return buckets[pos].val[i];

        return 0;
    }

    HashMap query_all() {
        HashMap mp;
        for (int i = 0; i < bucket_num; i++) {
            for (int j = 0; j < MAX_VALID_COUNTER; j++) {
                if (!buckets[i].key[j].empty()) {
                    mp[buckets[i].key[j]] = buckets[i].val[j];
                }
            }
        }
        return mp;
    }
};

#endif //_HEAVYPART_H_