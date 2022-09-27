#ifndef UTIL_H
#define UTIL_H

#include <x86intrin.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <algorithm>
#include <functional>
#include <unordered_map>
using namespace std;

#include "hash.h"

// #pragma pack(1)

#define MAX_TRAFFIC 6

enum KeyType{
    five_tuples = 0,
    srcIP_dstIP = 1,
    srcIP_srcPort = 2,
    dstIP_dstPort = 3,
    srcIP = 4,
    dstIP = 5,
};

const int key_mask[6][5] = {
    {1, 1, 1, 1, 1},
    {1, 1, 0, 0, 0},
    {1, 0, 1, 0, 0},
    {0, 1, 0, 1, 0},
    {1, 0, 0, 0, 0},
    {0, 1, 0, 0, 0}
};

#define TUPLES_LEN 13

struct TUPLES{
    uint8_t data[TUPLES_LEN];

    TUPLES() {
        memset(data, 0, sizeof data);
    }

    TUPLES(uint32_t _srcIP, uint32_t _dstIP, uint16_t _srcPort, uint16_t _dstPort, uint8_t _proto) {
        *((uint32_t*)(data)) = _srcIP;
        *((uint32_t*)(data+4)) = _dstIP;
        *((uint16_t*)(data+8)) = _srcPort;
        *((uint16_t*)(data+10)) = _dstPort;
        *((uint8_t*)(data+12)) = _proto;
    }

    inline bool empty() const{
        for (int i = 0; i < 13; i++)
            if (data[i])
                return false;
        return true;
    }

    inline uint32_t srcIP() const{
        return *((uint32_t*)(data));
    }

    inline uint32_t dstIP() const{
        return *((uint32_t*)(&data[4]));
    }

    inline uint16_t srcPort() const{
        return *((uint16_t*)(&data[8]));
    }

    inline uint16_t dstPort() const{
        return *((uint16_t*)(&data[10]));
    }

    inline uint8_t proto() const{
        return *((uint8_t*)(&data[12]));
    }

    inline uint64_t srcIP_dstIP() const{
        return *((uint64_t*)(data));
    }

    inline uint64_t srcIP_srcPort() const{
        uint64_t ip = srcIP();
        uint64_t port = srcPort();
        return ((ip << 32) | port);
    }

    inline uint64_t dstIP_dstPort() const{
        uint64_t ip = dstIP();
        uint64_t port = dstPort();
        return ((ip << 32) | port);
    }
};
ostream& operator<<(ostream &out, const TUPLES &t) {
    out << '(' << t.srcIP() << ',' << t.dstIP() << ',' << t.srcPort() << ',' 
            << t.dstPort() << ',' << (unsigned)t.proto() << ')';
    return out;
}

bool operator == (const TUPLES& a, const TUPLES& b){
    return memcmp(a.data, b.data, sizeof(TUPLES)) == 0;
}

namespace std{
    template<>
    struct hash<TUPLES>{
        size_t operator()(const TUPLES& item) const noexcept
        {
            return Hash::BOBHash32((uint8_t*)&item, sizeof(TUPLES), 0);
        }
    };
}

typedef std::chrono::high_resolution_clock::time_point TP;

typedef std::unordered_map<TUPLES, uint32_t> HashMap;
typedef std::unordered_map<TUPLES, vector<uint32_t>> VecHashMap;

inline TP now(){
    return std::chrono::high_resolution_clock::now();
}

inline double durationms(TP finish, TP start){
    return std::chrono::duration_cast<std::chrono::duration<double,std::ratio<1,1000000>>>(finish - start).count();
}

template<typename T>
T Median(std::vector<T> vec, uint32_t len){
    std::sort(vec.begin(), vec.end());
    return (len & 1) ? vec[len >> 1] : (vec[len >> 1] + vec[(len >> 1) - 1]) / 2.0;
}

#define INTERVAL_CNT 4

// const int RANGE[INTERVAL_CNT-1] = {100, 1000, 10000};

#endif
