#ifndef DLEFT_H
#define DLEFT_H

#include "../common/Util.h"
#include <bits/stdc++.h>
// #include "../elastic/param.h"
template<int _D, uint32_t memory>
class DLeft {
public:
    struct Counter{
        TUPLES ID;
        uint32_t count;
        uint32_t last_time;
        uint32_t max_interval;
        uint32_t max_wait;
    };

    DLeft() {
        D = _D;
        L = memory / D / sizeof(Counter);
        ofstream fout("log.txt",ios::app);
        fout<<"DLeft_D:"<<D<<" L:"<<L<<" "<<std::flush;
        fout.close();
        counter = new Counter*[D];
        for (int i = 0; i < D; i++) {
            counter[i] = new Counter[L];
            memset(counter[i], 0, sizeof(Counter) * L);
        }
    }
    void clear() {
        for (int i = 0; i < D; i++) {
            memset(counter[i], 0, sizeof(Counter) * L);
        }
    }
    ~DLeft() {
        for (int i = 0; i < D; i++) {
            delete[] counter[i];
        }
        delete counter;
    }

    void insert(TUPLES flow_id, uint32_t arrive_time,uint32_t waitime) {
        for (int i = 0; i < D; i++) {
            int pos = BOBHash(flow_id, i) % L;
            if (counter[i][pos].ID.empty()) {
                counter[i][pos].ID = flow_id;
                counter[i][pos].last_time = arrive_time;
                counter[i][pos].max_wait = waitime;
            }
            if (counter[i][pos].ID == flow_id) {
                counter[i][pos].count++;
                if(arrive_time<counter[i][pos].last_time) cout<<"negative interval"<<std::endl;
                counter[i][pos].max_interval = std::max(counter[i][pos].max_interval,
                                                arrive_time - counter[i][pos].last_time);
                counter[i][pos].last_time = arrive_time;
                counter[i][pos].max_wait = std::max(counter[i][pos].max_wait,waitime);
                return;
            }
        }
    }

    void insert_pre(uint32_t src_ip,uint32_t dst_ip,uint16_t src_port,uint16_t dst_port,uint8_t _proto, uint32_t arrive_time,uint32_t waitime){
        TUPLES item(src_ip,dst_ip,src_port,dst_port,_proto);
        insert(item,arrive_time,waitime);
    }

    uint32_t query(TUPLES flow_id) {
        for (int i = 0; i < D; i++) {
            int pos = BOBHash(flow_id, i) % L;
            if (counter[i][pos].ID == flow_id) {
                return counter[i][pos].count;
            }
        }
        return 0;
    }
    uint32_t query_interval(TUPLES flow_id) {
        for (int i = 0; i < D; i++) {
            int pos = BOBHash(flow_id, i) % L;
            if (counter[i][pos].ID == flow_id) {
                return counter[i][pos].max_interval;
            }
        }
        return 0;
    }

    uint32_t query_wait(TUPLES flow_id) {
        for (int i = 0; i < D; i++) {
            int pos = BOBHash(flow_id, i) % L;
            if (counter[i][pos].ID == flow_id) {
                return counter[i][pos].max_wait;
            }
        }
        return 0;
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

    vector<vector<int>> PPrint(){
        vector<vector<int>> res;
        res.resize(D);
        for(int i=0;i<D;i++){
            for(int j=0;j<L;j++){
                if(counter[i][j].ID.empty())
                    res[i].push_back(0);
                else
                    res[i].push_back(1);
            }
        }
        return res;
    }

private:
    Counter **counter;
    int D, L;
};

#endif  // DLEFT_H
