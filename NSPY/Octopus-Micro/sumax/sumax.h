#include "../common/Util.h"
#include <bits/stdc++.h>
using namespace std;

template<int _memory, uint32_t _d>
class SuMaxSketch {
public:
    struct Counter {
        uint32_t cnt[INTERVAL_CNT];
    };

    int memory;
    int d, len;
    uint32_t **count;
    uint32_t **max_interval;
    uint32_t **max_wait;

    // SuMaxSketch(int _memory, int _d=3) {
    SuMaxSketch() {
        d = _d;
        memory = _memory;
        len = memory / d / 8;

        count = new uint32_t*[d];
        max_interval = new uint32_t*[d];
        max_wait = new uint32_t*[d];
        for (int i = 0; i < d; i++) {

            count[i] = new uint32_t[len];
            memset(count[i], 0, sizeof(uint32_t) * len);
            max_interval[i] = new uint32_t[len];
            memset(max_interval[i], 0, sizeof(uint32_t) * len);
            max_wait[i] = new uint32_t[len];
            memset(max_wait[i], 0, sizeof(uint32_t) * len);
        }
    }
    void clear(){
        for (int i = 0; i < d; i++){
            memset(count[i],0,       sizeof(uint32_t)*len);
            memset(max_interval[i],0,sizeof(uint32_t)*len);
            memset(max_wait[i],0,    sizeof(uint32_t)*len);
        }
    }
    ~SuMaxSketch() {
        for (int i = 0; i < d; i++){
            delete[] count[i];
            delete[] max_interval[i];
            delete[] max_wait[i];
        }
        delete[] count;
        delete[] max_interval;
        delete[] max_wait;
    }


    void insert(TUPLES flow_id, uint32_t delay,uint32_t waitime) {

        uint32_t mi = UINT32_MAX;
        for (int i = 0; i < d; i++) {
            int pos = BOBHash(flow_id, i) % len;
            mi = min(mi, count[i][pos] + 1);
            if (count[i][pos] < mi) {
                count[i][pos] += 1;
            }
            max_interval[i][pos] = max(max_interval[i][pos],delay);
            max_wait[i][pos] = max(max_wait[i][pos],waitime);
        }
    }
    void insert_pre(uint32_t src_ip,uint32_t dst_ip,uint16_t src_port,uint16_t dst_port,uint8_t _proto, uint32_t delay,uint32_t waitime){
        TUPLES item(src_ip,dst_ip,src_port,dst_port,_proto);
        insert(item,delay,waitime);
    }

    uint32_t query(TUPLES flow_id) {
        uint32_t res = UINT32_MAX;
        for (int i = 0; i < d; i++) {
            int pos = BOBHash(flow_id, i) % len;
            res = min(res, count[i][pos]);

        }
        return res;
    }

    uint32_t query_interval(TUPLES flow_id) {
        uint32_t res = UINT32_MAX;
        for (int i = 0; i < d; i++) {
            int pos = BOBHash(flow_id, i) % len;
            res = min(res, max_interval[i][pos]);

        }
        return res;
    }
    uint32_t query_wait(TUPLES flow_id) {
        uint32_t res = UINT32_MAX;

        for (int i = 0; i < d; i++) {
            int pos = BOBHash(flow_id, i) % len;
            res = min(res, max_wait[i][pos]);

        }
        return res;
    }
    uint32_t query_pre(uint32_t src_ip,uint32_t dst_ip,uint16_t src_port,uint16_t dst_port,uint8_t _proto){
        TUPLES item(src_ip,dst_ip,src_port,dst_port,_proto);
        return query(item);
    }
    uint32_t query_interval_pre(uint32_t src_ip,uint32_t dst_ip,uint16_t src_port,uint16_t dst_port,uint8_t _proto){
        TUPLES item(src_ip,dst_ip,src_port,dst_port,_proto);
        return query_interval(item);
    }
    uint32_t query_wait_pre(uint32_t src_ip,uint32_t dst_ip,uint16_t src_port,uint16_t dst_port,uint8_t _proto){
        TUPLES item(src_ip,dst_ip,src_port,dst_port,_proto);
        return query_wait(item);
    }

};