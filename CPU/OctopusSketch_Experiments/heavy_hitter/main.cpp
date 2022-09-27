#include "octopus.h"
#include "trace.h"
#include <bits/stdc++.h>
using namespace std;

const int MEMORY = 500000;
const int BUCKET_CNT = MEMORY / 500;

pair<HashMap, vector<TUPLES>> calc_hh(const HashMap &mp, int key_type, int threshold) {
    HashMap a;
    vector<TUPLES> ans;
    for (auto [key, cnt]: mp) {
        TUPLES t = key;
        for (int i = 0; i < 5; i++) {
            if (key_mask[key_type][i] == 0)
                t.set(i, 0);
        }
        a[t] += cnt;
    }
    for (auto [key, cnt]: a) {
        if (cnt >= threshold)
            ans.push_back(key);
    }
    return make_pair(a, ans);
}

vector<TUPLES> calc_both(const vector<TUPLES> &a, const vector<TUPLES> &b) {
    unordered_map<TUPLES, bool> mp;
    for (auto x: a) {
        mp[x] = true;
    }
    vector<TUPLES> ans;
    for (auto x: b) {
        if (mp[x])
            ans.push_back(x);
    }
    return ans;
}

uint32_t abs_sub(uint32_t a, uint32_t b) {
    if (a > b)
        return a - b;
    return b - a;
}

void heavy_hitter() {
    OctopusSketch<BUCKET_CNT, MEMORY> octopus;
    vector<pair<TUPLES, uint32_t>> data = loadCAIDA18();

    HashMap gt;
    for (auto [key, tm]: data) {
        gt[key]++;
    }

    int threshold = data.size() / 2000;

    cout << "inserting..." << endl;

    for (auto [key, tim]: data) {
        octopus.insert(key);
    }

    cout << "evaluating..." << endl;
    
    HashMap res = octopus.query_all();
    int real_tot = 0, est_tot = 0, both_tot = 0;
    double are_tot = 0;
    double recall[6], accuracy[6], are[6], f1[6];
    for (int i = 0; i < 6; i++) {
        auto [real_map, real] = calc_hh(gt, i, threshold);
        auto [est_map, est] = calc_hh(res, i, threshold);
        auto both = calc_both(real, est);
        real_tot += real.size();
        est_tot += est.size();
        both_tot += both.size();
        for (auto key: both) {
            are_tot += (double)abs_sub(est_map[key], real_map[key]) / real_map[key];
        }
        recall[i] = (double)both_tot / real_tot;
        accuracy[i] = (double)both_tot / est_tot;
        are[i] = are_tot / both_tot;
        f1[i] = 2 * recall[i] * accuracy[i] / (recall[i] + accuracy[i]);
    }
    ofstream fout("heavy_hitter_result.csv");
    fout << "Number of Keys,Recall,Precision,F1 Score,ARE" << endl;
    for (int i = 0; i < 6; i++) {
        fout << i+1 << ',' << recall[i] << ',' << accuracy[i] << ',' << \
             f1[i] << ',' << are[i] << endl;
    }
    cout << "Result in heavy_hitter_result.csv" << endl;
}

int main() {
    heavy_hitter();
    return 0;
}
