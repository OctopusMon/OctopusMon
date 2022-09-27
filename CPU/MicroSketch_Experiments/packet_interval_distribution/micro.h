#ifndef MICRO_H
#define MICRO_H

#include "Util.h"
#include <bits/stdc++.h>
using namespace std;

class CellSketch {
public:
    static constexpr int COUNTER_CNT = 8;
    static constexpr int MAX_INTERVAL = 1 << 23;
    static constexpr bool UNBIASED = true;
    static constexpr int HASH_CNT = 3;
    static constexpr int LINE_CNT = 4;
    static constexpr int DIVIDE_RANGE = COUNTER_CNT;
    const int divide_cnt[LINE_CNT-1] = {500, 1000, 2000};
    const int cell_cnt[LINE_CNT] = {80000, 20000, 10000, 5000};

    int update_cnt;
    int insert_cnt;

    int *cells[LINE_CNT];
    set<uint32_t> *cell_flow[LINE_CNT];

    struct CounterID {
        TUPLES flow_id;
        int level;
        int cell;
        int counter;

        CounterID(TUPLES _flow_id, int _level, int _cell, int _counter) {
            memset(this, 0, sizeof(CounterID));
            flow_id = _flow_id;
            level = _level;
            cell = _cell;
            counter = _counter;
        }
    };

    static int harmonic_mean(int l, int r) {
        return 2.0 * l * r / (l+r);
    }

    static int arithmetic_mean(int l, int r) {
        return (l + r) / 2;
    }

    static int mean(int l, int r) {
        return (r-l+1 >= (1 << COUNTER_CNT) ? harmonic_mean(l, r) \
                                            : arithmetic_mean(l, r));
    }

    static vector<pair<int, int>> get_subinterval_uniform(int l, int r) {
        double range = (double)(r-l+1) / COUNTER_CNT;
        int ul = l, ur = l + range - 1;
        vector<pair<int, int>> res;
        for (int i = 0; i < COUNTER_CNT; i++) {
            res.push_back(make_pair(ul, ur));
            ul = ur + 1;
            ur = (i+1 == COUNTER_CNT-1 ? r : ul + range - 1);
        }
        return res;
    }

    static vector<pair<int, int>> get_subinterval_exp(int l, int r) {
        int range = (r-l+1) / ((1 << COUNTER_CNT) - 1);
        int ul, ur = l-1, weight = 1;
        vector<pair<int, int>> res;
        for (int i = 0; i < COUNTER_CNT; i++) {
            ul = ur + 1;
            ur = (i == COUNTER_CNT-1 ? r : ul + range * weight  - 1);
            weight *= 2;
            res.push_back(make_pair(ul, ur));
        }
        return res;
    }

    static vector<pair<int, int>> get_subinterval(int l, int r) {
        return (r-l+1 >= (1 << COUNTER_CNT) ? get_subinterval_exp(l, r) \
                                            : get_subinterval_uniform(l, r));
    }
    
    CellSketch() {
        for (int i = 0; i < LINE_CNT; i++) {
            cells[i] = new int[cell_cnt[i] * COUNTER_CNT];
            memset(cells[i], 0, cell_cnt[i] * COUNTER_CNT * sizeof(int));

            cell_flow[i] = new set<uint32_t>[cell_cnt[i]];
        }
    }

    ~CellSketch() {
        for (int i = 0; i < LINE_CNT; i++) {
            delete[] cells[i];
        }
    }

    int get_sign(TUPLES flow_id, int hash_id) {
        if (!UNBIASED)
            return 1;
        int hash_val = BOBHash(flow_id, HASH_CNT + hash_id);
        return hash_val % 2 ? 1 : -1;
    }

    int* get_cell(int level, int cell_id) {
        assert(cell_id < cell_cnt[level]);
        return cells[level] + cell_id * COUNTER_CNT;
    }

    int get_lower_cell_id(TUPLES flow_id, int level, int cell_id, int counter_id) {
        if (level == LINE_CNT - 1)
            return -1;
        CounterID id(flow_id, level, cell_id, counter_id);
        return BOBHash(id) % cell_cnt[level+1];
    }

    void cell_add(TUPLES flow_id, int cell_id, int level, int l, int r, 
                  int delay, int val, int sign) {
        assert(delay >= l);
        if (delay > MAX_INTERVAL)
            delay = MAX_INTERVAL;

        cell_flow[level][cell_id].insert(flow_id.srcIP());

        int *cell = get_cell(level, cell_id);
        int counter_id = -1;
        vector<pair<int, int>> subinterval = get_subinterval(l, r);
        int sl, sr;
        for (int j = 0; j < COUNTER_CNT; j++) {
            auto [ul, ur] = subinterval[j];
            if (ul <= delay && delay <= ur) {
                counter_id = j;
                sl = ul;
                sr = ur;
            }
        }
        assert(counter_id != -1);
        int &counter = cell[counter_id];
        int lower_cell_id = get_lower_cell_id(flow_id, level, cell_id, counter_id);

        update_cnt++;
        
        if (counter != INT32_MAX) {
            counter += val * sign;
            if (level+1 < LINE_CNT && sr - sl + 1 >= DIVIDE_RANGE \
                && abs(counter) >= divide_cnt[level] && (counter > 0) == (sign == 1)) {
                int divide_val = abs(counter) / COUNTER_CNT;
                vector<pair<int, int>> subinterval = get_subinterval(sl, sr);
                for (int j = 0; j < COUNTER_CNT; j++) {
                    auto [ul, ur] = subinterval[j];
                    if (j == COUNTER_CNT - 1)
                        divide_val += abs(counter) % COUNTER_CNT;
                    cell_add(flow_id, lower_cell_id, level+1, sl, sr, 
                                arithmetic_mean(ul, ur), divide_val, sign);
                }
                counter = INT32_MAX;
            }
        } else {
            cell_add(flow_id, lower_cell_id, level+1, sl, sr, delay, val, sign);
        }
    }

    void query_dfs(TUPLES flow_id, vector<int> cell_ids, int level, int l, int r, 
                    vector<pair<int, int>> &res) {
        vector<int*> cell_list;
        for (int cell_id: cell_ids)
            cell_list.push_back(get_cell(level, cell_id));
        vector<pair<int, int>> subinterval = get_subinterval(l, r);
        for (int counter_id = 0; counter_id < COUNTER_CNT; counter_id++) {
            auto [ul, ur] = subinterval[counter_id];
            bool divided = true;
            vector<int> ans;
            for (int* cell: cell_list) {
                int counter = cell[counter_id];
                if (counter != INT_MAX) {
                    divided = false;
                    ans.push_back(abs(counter));
                }
            }
            if (divided) {
                vector<int> lower_cell_ids;
                for (int cell_id: cell_ids) {
                    int lower_cell_id = get_lower_cell_id(flow_id, level, cell_id, counter_id);
                    lower_cell_ids.push_back(lower_cell_id);
                }
                query_dfs(flow_id, lower_cell_ids, level+1, ul, ur, res);
            } else {
                sort(ans.begin(), ans.end());
                int val;
                if (UNBIASED) {
                    val = ans[ans.size() / 2];
                } else {
                    val = ans[0];
                }
                if (val > 0) {
                    res.push_back(make_pair(ur, val));
                }
            }
        }
    }

    void insert(TUPLES flow_id, int delay, int val=1) {
        insert_cnt++;
        for (int i = 0; i < HASH_CNT; i++) {
            int cell_id = BOBHash(flow_id, i) % cell_cnt[0];
            cell_add(flow_id, cell_id, 0, 0, MAX_INTERVAL, delay, val, 
                     get_sign(flow_id, i));
        }
    }

    vector<pair<int, int>> query(TUPLES flow_id) {
        vector<pair<int, int>> res;
        vector<int> cell_ids;
        for (int i = 0; i < HASH_CNT; i++) {
            int cell_id = BOBHash(flow_id, i) % cell_cnt[0];
            cell_ids.push_back(cell_id);
        }
        query_dfs(flow_id, cell_ids, 0, 0, MAX_INTERVAL, res);
        return res;
    }
};

class MicroSketch {
public:
    static const int DLEFT_MEMORY = 500000;
    static const int D = 3;
    static const bool RECORD_DELAY = true;

    struct Counter{
        TUPLES ID;
        uint32_t count;
        uint32_t last_time;
        uint32_t max_interval;
    };

    MicroSketch() {
        L = DLEFT_MEMORY / D / sizeof(Counter);

        counter = new Counter*[D];
        for (int i = 0; i < D; i++) {
            counter[i] = new Counter[L];
            memset(counter[i], 0, sizeof(Counter) * L);
        }

        if (RECORD_DELAY)
            cell_sketch = new CellSketch();
    }

    ~MicroSketch() {
        for (int i = 0; i < D; i++) {
            delete[] counter[i];
        }
        delete counter;
        
        if (RECORD_DELAY)
            delete cell_sketch;
    }

    void insert(TUPLES flow_id, int arrive_time, int delay=-1) {
        for (int i = 0; i < D; i++) {
            int pos = BOBHash(flow_id, i) % L;
            if (counter[i][pos].ID.empty()) {
                counter[i][pos].ID = flow_id;
                counter[i][pos].last_time = arrive_time;
            }
            if (counter[i][pos].ID == flow_id) {
                counter[i][pos].count++;
                counter[i][pos].max_interval = std::max(counter[i][pos].max_interval,
                                                arrive_time - counter[i][pos].last_time);
                counter[i][pos].last_time = arrive_time;
                if (RECORD_DELAY)
                    cell_sketch->insert(flow_id, delay);
                return;
            }
        }
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

    vector<pair<int, int>> query_delay(TUPLES flow_id) {
        assert(RECORD_DELAY);
        for (int i = 0; i < D; i++) {
            int pos = BOBHash(flow_id, i) % L;
            if (counter[i][pos].ID == flow_id) {
                return cell_sketch->query(flow_id);
            }
        }
        return vector<pair<int, int>>();
    }

    vector<pair<int, TUPLES>> flow_list() {
        vector<pair<int, TUPLES>> res;
        for (int i = 0; i < D; i++) {
            for (int j = 0; j < L; j++) {
                if (!counter[i][j].ID.empty()) {
                    res.push_back(make_pair(counter[i][j].count, counter[i][j].ID));
                }
            }
        }
        return res;
    }

private:
    Counter **counter;
    int L;
    CellSketch *cell_sketch;
};

#endif  // MICRO_H
