#ifndef _HEAVYPART_H_
#define _HEAVYPART_H_

#include "param.h"
// #define USING_SIMD_ACCELERATION

template<int bucket_num>
class HeavyPart
{
public:
    Bucket buckets[bucket_num];
    HeavyPart() { this->clear(); }
    ~HeavyPart() {}

    void clear() { memset(buckets, 0, sizeof(Bucket) * bucket_num); }

    int insert(TUPLES key, TUPLES &swap_key, vector<uint32_t> &swap_val, int interval_id) {
        int pos = BOBHash(key) % bucket_num;
        uint32_t min_counter_val;
        int min_counter;

        do{
            /* find if there has matched bucket */
            int matched = -1, empty = -1;
            min_counter = 0;
            min_counter_val = GetCounterVal(buckets[pos].counter[0].tot);
            for(int i = 0; i < COUNTER_PER_BUCKET - 1; i++){
                if(buckets[pos].counter[i].key == key){
                    matched = i;
                    break;
                }
                if(buckets[pos].counter[i].key.empty() && empty == -1)
                    empty = i;
                if(min_counter_val > GetCounterVal(buckets[pos].counter[0].tot)){
                    min_counter = i;
                    min_counter_val = GetCounterVal(buckets[pos].counter[0].tot);
                }
            }

            /* if matched */
            if(matched != -1){
                buckets[pos].counter[matched].tot++;
                buckets[pos].counter[matched].cnt[interval_id]++;
                return 0;
            }

            /* if there has empty bucket */
            if(empty != -1){
                buckets[pos].counter[empty].key = key;
                buckets[pos].counter[empty].tot++;
                buckets[pos].counter[empty].cnt[interval_id]++;
                return 0;
            }
        }while(0);

        /* update guard val and comparison */
        uint32_t guard_val = buckets[pos].counter[MAX_VALID_COUNTER].tot;
        guard_val = UPDATE_GUARD_VAL(guard_val);

        if(!JUDGE_IF_SWAP(GetCounterVal(min_counter_val), guard_val)){
            buckets[pos].counter[MAX_VALID_COUNTER].tot = guard_val;
            return 2;
        }

        swap_key = buckets[pos].counter[min_counter].key;
        swap_val.clear();
        for (int i = 0; i < INTERVAL_CNT; i++)
            swap_val.push_back(buckets[pos].counter[min_counter].cnt[i]);

        buckets[pos].counter[MAX_VALID_COUNTER].tot = 0;
        buckets[pos].counter[min_counter].key = key;
        buckets[pos].counter[min_counter].tot = 0x80000001;
        for (int i = 0; i < INTERVAL_CNT; i++)
            buckets[pos].counter[min_counter].cnt[i] = 0;
        buckets[pos].counter[min_counter].cnt[interval_id]++;

        return 1;
    }

    vector<uint32_t> query(TUPLES key) {
        int pos = BOBHash(key) % bucket_num;

        for(int i = 0; i < MAX_VALID_COUNTER; ++i) {
            if(buckets[pos].counter[i].key == key)
                return vector<uint32_t>(buckets[pos].counter[i].cnt, 
                                        buckets[pos].counter[i].cnt + INTERVAL_CNT);
        }

        return vector<uint32_t>(INTERVAL_CNT, 0);
    }

    VecHashMap query_all() {
        VecHashMap mp;
        for (int i = 0; i < bucket_num; i++) {
            for (int j = 0; j < MAX_VALID_COUNTER; j++) {
                if (!buckets[i].counter[j].key.empty()) {
                    mp[buckets[i].counter[j].key] = vector<uint32_t>(
                                                buckets[i].counter[j].cnt, 
                                                buckets[i].counter[j].cnt + INTERVAL_CNT);
                }
            }
        }
        return mp;
    }
};

#endif //_HEAVYPART_H_