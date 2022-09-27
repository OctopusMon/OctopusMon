#include "../common/Util.h"
#include <bits/stdc++.h>
using namespace std;
#define COUNTER_PER_BUCKET 8

struct Count{
    uint32_t count;
    uint32_t max_interval;
    uint32_t max_wait;
    void clear(){
        count=0;max_interval=0,max_wait=0;
    }
    void insert(TUPLES flow_id, uint32_t delay,uint32_t waitime) {
        count++;
        max_interval = max(max_interval,delay);
        max_wait = max(max_wait,waitime);
        return;
    }
};

class Buckets{
public:
    TUPLES key[COUNTER_PER_BUCKET];
    Count a[COUNTER_PER_BUCKET];
    uint32_t priori[COUNTER_PER_BUCKET];

    Buckets() {
        for(int i=0;i<COUNTER_PER_BUCKET;i++){
            a[i].clear();
            // key[i]=0;
            priori[i]=0;
        }
        memset(key,0,sizeof(TUPLES)*COUNTER_PER_BUCKET);
    }

};
int tmp=0;
template<uint32_t memory_limit>
class MARBLE {
private:
    int len[3];
    Buckets *b;

public:
    MARBLE() {
        int memory_per_line = memory_limit;
        len[0] = memory_per_line / sizeof(Buckets);

        b = new Buckets[len[0]];

    }

    ~MARBLE() {
        delete[] b;
        // for (int i = 0; i < 3; i++)
            // delete hash[i];
    }

    void clear(){
        memset(b,0,sizeof(Buckets)*len[0]);
    }

    int insert(TUPLES flow_id, uint32_t delay,uint32_t waitime) {
        /* find if there has matched bucket */
        int pos = BOBHash(flow_id, 1205) % len[0];
		int matched = -1, empty = -1, min_counter = 0;
		uint32_t min_counter_val = b[pos].priori[0];
		for(int i = 0; i < COUNTER_PER_BUCKET ; i++){
            b[pos].priori[i]--;
			if(b[pos].key[i] == flow_id){
				matched = i;
                // cout<<"matched"<<flush;
			}
			if(b[pos].key[i].empty() && empty == -1){
				empty = i;
                // cout<<"empty"<<flush;
            }
			if(min_counter_val > b[pos].priori[i]){
				min_counter = i;
				min_counter_val = b[pos].priori[i];
			}
		}

        /* if matched */
		if(matched != -1){	
            b[pos].a[matched].insert(flow_id,delay,waitime);
            b[pos].priori[matched] = 2000000000;
			return 0;
		}
		/* if there has empty bucket */
		if(empty != -1){
            b[pos].key[empty] = flow_id;
            b[pos].a[empty].insert(flow_id,delay,waitime);
            b[pos].priori[empty] = 2000000000;

			return 0;
		}
        
        // uint32_t guard_val = b[pos].against_votes;
        // guard_val = guard_val+1;

        // if(!((guard_val) > ((min_counter_val) << 3))){
        //     b[pos].against_votes = guard_val;
        //     return 2;
        // }

        // swap_val = b[pos].a[min_counter].to_vector_modified();
        // swap_key = b[pos].key[min_counter];


        // cout<<"kick"<<endl;
        b[pos].priori[min_counter] = 2000000000;
        b[pos].a[min_counter].clear();
        b[pos].a[min_counter].insert(flow_id,delay,waitime);
        b[pos].key[min_counter] = flow_id;
        
        return 1;
    }

    uint32_t query(TUPLES flow_id,int typ) {
        int pos = BOBHash(flow_id, 1205) % len[0];
        int matched=-1;
        for(int i = 0; i < COUNTER_PER_BUCKET ; i++){
			if(b[pos].key[i] == flow_id){
				matched = i;
				break;
			}
		}
        if(matched==-1)
            return 0;
        if(typ==0)
            return b[pos].a[matched].count;
        if(typ==1)
            return b[pos].a[matched].max_interval;
        if(typ==2)
            return b[pos].a[matched].max_wait;
    }
    void insert_pre(uint32_t src_ip,uint32_t dst_ip,uint16_t src_port,uint16_t dst_port,uint8_t _proto, uint32_t delay,uint32_t waitime){
        TUPLES item(src_ip,dst_ip,src_port,dst_port,_proto);
        insert(item,delay,waitime);
    }

    uint32_t query_pre(uint32_t src_ip,uint32_t dst_ip,uint16_t src_port,uint16_t dst_port,uint8_t _proto){
        TUPLES item(src_ip,dst_ip,src_port,dst_port,_proto);
        return query(item,0);
    }

    uint32_t query_interval_pre(uint32_t src_ip,uint32_t dst_ip,uint16_t src_port,uint16_t dst_port,uint8_t _proto){
        TUPLES item(src_ip,dst_ip,src_port,dst_port,_proto);
        return query(item,1);
    }
    uint32_t query_wait_pre(uint32_t src_ip,uint32_t dst_ip,uint16_t src_port,uint16_t dst_port,uint8_t _proto){
        TUPLES item(src_ip,dst_ip,src_port,dst_port,_proto);
        return query(item,2);
    }
};