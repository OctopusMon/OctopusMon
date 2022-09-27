#include "Util.h"
#include "octopus.h"
#include "trace.h"
#include <bits/stdc++.h>
using namespace std;

const int MEMORY = 500000;
const int BUCKET_CNT = MEMORY / 1000;

vector<pair<TUPLES, uint32_t>> get_interval(const vector<pair<TUPLES, uint32_t>> &data) {
    unordered_map<TUPLES, uint32_t> last_time;
    vector<pair<TUPLES, uint32_t>> res;
    for (auto [key, tm]: data) {
        if (last_time.count(key)) {
            res.push_back(make_pair(key, tm - last_time[key]));
        }
        last_time[key] = tm;
    }
    return res;
}

int get_interval_id(uint32_t interval) {
    int interval_id;
    for (interval_id = 0; interval_id < INTERVAL_CNT - 1; interval_id++) {
        if (interval <= RANGE[interval_id])
            break;
    }
    return interval_id;
}

VecHashMap calc_distribution(const VecHashMap &mp, int key_type) {
    VecHashMap dist_map;
    for (auto [key, cnt]: mp) {
        TUPLES t = key;
        for (int i = 0; i < 5; i++) {
            if (key_mask[key_type][i] == 0)
                t.set(i, 0);
        }
        if (!dist_map.count(t)) {
            dist_map[t] = vector<uint32_t>(4, 0);
        }
        for (int i = 0; i < 4; i++)
            dist_map[t][i] += cnt[i];
    }
    return dist_map;
}

void interval_distribution() {
    OctopusSketch<BUCKET_CNT, MEMORY> octopus;

    vector<pair<TUPLES, uint32_t>> data = loadCAIDA18();
    vector<pair<TUPLES, uint32_t>> interval_data = get_interval(data);

    VecHashMap gt;
    for (auto [key, tm]: interval_data) {
        int interval_id = get_interval_id(tm);
        if (!gt.count(key)) {
            gt[key] = vector<uint32_t>(4, 0);
        }
        gt[key][interval_id]++;
    }

    int threshold = data.size() / 2000;
    cout << "inserting..." << endl;
    
    VecHashMap res;
    for (auto [key, interval]: interval_data) {
        octopus.insert(key, interval);
    }

    cout << "evaluating..." << endl;

    res = octopus.query_all();

    int err_tot = 0, sum = 0;
    double wmre[6];
    for (int i = 0; i < 6; i++) {
        auto real_map = calc_distribution(gt, i);
        auto est_map = calc_distribution(res, i);
        for (auto [key, cnt]: est_map) {
            if (!real_map.count(key))
                continue;
            int est_sum = 0, real_sum = 0;
            for (auto x: cnt)
                est_sum += x;
            for (auto x: real_map[key])
                real_sum += x;
            if (est_sum < threshold || real_sum == 0)
                continue;
            sum += est_sum + real_sum;
            for (int j = 0; j < 4; j++) {
                int tmp = cnt[j] - real_map[key][j];
                if (tmp < 0)
                    tmp = -tmp;
                err_tot += tmp;
            }
        }
        wmre[i] = (double)err_tot * 2 / sum;
    }
    ofstream fout("distribution_result.csv");
    fout << "Number of Keys,WMRE" << endl;
    for (int i = 0; i < 6; i++) {
        fout << i+1 << ',' << wmre[i] << endl;
    }
    cout << "Result in distribution_result.csv" << endl;
}

int main() {
    interval_distribution();
    return 0;
}
