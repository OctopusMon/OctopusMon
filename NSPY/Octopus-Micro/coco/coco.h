#ifndef COCO_H
#define COCO_H

#include <unordered_map>
#include <string.h>
#include "../common/Util.h"

class CocoSketch {
public:
    struct Counter{
        TUPLES ID;
        uint32_t count;
    };

    CocoSketch(uint32_t _MEMORY, uint32_t _HASH_NUM = 2){
        // std::cout << "coco memory " << _MEMORY << std::endl;
        HASH_NUM = _HASH_NUM;
        LENGTH = _MEMORY / _HASH_NUM / sizeof(Counter);
        ofstream fout("log.txt",ios::app);
        fout << "coco " << LENGTH << " "<<std::flush;
        fout.close();
        counter = new Counter* [HASH_NUM];
        for(uint32_t i = 0;i < HASH_NUM;++i){
            counter[i] = new Counter [LENGTH];
            memset(counter[i], 0, sizeof(Counter) * LENGTH);
        }
    }

    ~CocoSketch(){
        for(uint32_t i = 0;i < HASH_NUM;++i){
            delete [] counter[i];
        }
        delete [] counter;
    }

    void clear() {
        for(uint32_t i = 0;i < HASH_NUM;++i){
            memset(counter[i], 0, sizeof(Counter) * LENGTH);
        }
    }

    void insert(const TUPLES& item, int w=1){
        if (w == 0)
            return;
        uint32_t minimum = 0x7fffffff;
        uint32_t minPos, minHash;

        for(uint32_t i = 0;i < HASH_NUM;++i){
            uint32_t position = BOBHash(item, i) % LENGTH;
            if(counter[i][position].ID == item){
                counter[i][position].count += w;
                return;
            }
            if(counter[i][position].count < minimum){
                minPos = position;
                minHash = i;
                minimum = counter[i][position].count;
            }
        }

        counter[minHash][minPos].count += w;
        if(randomGenerator() % counter[minHash][minPos].count < w){
            counter[minHash][minPos].ID = item;
        }
    }

    uint32_t query(const TUPLES& item) {
        for(uint32_t i = 0;i < HASH_NUM;++i){
            uint32_t position = BOBHash(item, i) % LENGTH;
            if(counter[i][position].ID == item){
                return counter[i][position].count;
            }
        }
        return 0;
    }

    HashMap query_all(){
        HashMap ret;

        for(uint32_t i = 0;i < HASH_NUM;++i){
            for(uint32_t j = 0;j < LENGTH;++j){
                if (!counter[i][j].ID.empty())
                    ret[counter[i][j].ID] = counter[i][j].count;
            }
        }

        return ret;
    }

private:
    uint32_t LENGTH;
    uint32_t HASH_NUM;

    Counter** counter;
};

#endif
