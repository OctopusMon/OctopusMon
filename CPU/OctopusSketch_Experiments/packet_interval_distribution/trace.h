#ifndef _TRACE_H_
#define _TRACE_H_

#include "Util.h"
#include <bits/stdc++.h>
using namespace std;

vector<pair<TUPLES, uint32_t>> loadCAIDA18(int read_num = -1, 
const char* filename =
"/share/datasets/CAIDA2018/dataset/130000.dat"
){
    printf("Open %s \n", filename);
    FILE* pf = fopen(filename, "rb");
    if(!pf){
        printf("%s not found!\n", filename);
        exit(-1);
    }

    vector<pair<TUPLES, uint32_t>> vec;
    double ftime=-1;
    char trace[30];
    int i = 0;
    while(fread(trace, 1, 21, pf)/* && vec.size()<1e5*/){
        TUPLES u;
        memcpy(u.data, trace, 13);
        double ttime = *(double*) (trace+13);
        if(ftime<0)ftime=ttime;
        vec.push_back(pair<TUPLES, int>(u, uint32_t((ttime-ftime)*10000000)));
        // if( i % 10000 ==0 ) printf("%u %u\n", tkey, int((ttime-ftime)*10000000));
        // 流持续60s
        if(++i == read_num) break;
    }
    printf("load %d packets\n", i);
    fclose(pf);
    return vec;
}

vector<pair<int, TUPLES>> groundtruth(vector<pair<TUPLES, int>> &input, int read_num = -1){
    map<TUPLES, int> cnt;
    int i = 0;
    for(auto [tkey,ttime]: input){
        ++cnt[tkey];
        if(++i == read_num) break;
    }
    vector<pair<int, TUPLES>> ans;
    for(auto flow: cnt)
        ans.push_back({flow.second, flow.first});
    sort(ans.begin(), ans.end(), greater<pair<int, TUPLES>>());
    printf("there are %d flows\n", (int)ans.size());
    return ans;
}

#endif // _TRACE_H_

