#include "octopus.h"
#include "trace.h"
#include <bits/stdc++.h>
using namespace std;

const int MEMORY = 500000;
const int BUCKET_CNT = MEMORY / 500;

pair<unordered_map<TUPLES, int>, vector<TUPLES>> calc_hc(const unordered_map<TUPLES, int> &mp, int key_type, int threshold) {
    unordered_map<TUPLES, int> a;
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
        if (abs(cnt) >= threshold)
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

void heavy_change() {
    OctopusSketch<BUCKET_CNT, MEMORY> sketch1, sketch2;
    vector<pair<TUPLES, uint32_t>> data = loadCAIDA18();
    vector<pair<TUPLES, uint32_t>> data1(data.begin(), data.begin() + data.size() / 2);
    vector<pair<TUPLES, uint32_t>> data2(data.begin() + data.size() / 2 + 1, data.end());

    unordered_map<TUPLES, int> gt;
    for (auto [key, tm]: data1) {
        gt[key]++;
    }
    for (auto [key, tm]: data2) {
        gt[key]--;
    }
    
    int threshold = data.size() / 5000;

    cout << "inserting..." << endl;

    for (auto [key, tim]: data1)
        sketch1.insert(key);
    for (auto [key, tim]: data2)
        sketch2.insert(key);

    cout << "evaluating..." << endl;
    
    HashMap res1 = sketch1.query_all(), res2 = sketch2.query_all();
    unordered_map<TUPLES, int> res;
    for (auto [key, val]: res1) {
        res[key] += val;
    }
    for (auto [key, val]: res2) {
        res[key] -= val;
    }

    int real_tot = 0, est_tot = 0, both_tot = 0;
    double are_tot = 0;
    double recall[6], accuracy[6], f1[6];
    for (int i = 0; i < 6; i++) {
        auto [real_map, real] = calc_hc(gt, i, threshold);
        auto [est_map, est] = calc_hc(res, i, threshold);
        auto both = calc_both(real, est);
        real_tot += real.size();
        est_tot += est.size();
        both_tot += both.size();
        recall[i] = (double)both_tot / real_tot;
        accuracy[i] = (double)both_tot / est_tot;
        f1[i] = 2 * recall[i] * accuracy[i] / (recall[i] + accuracy[i]);
    }
    ofstream fout("heavy_change_result.csv");
    fout << "Number of Keys,Recall,Precision,F1 Score" << endl;
    for (int i = 0; i < 6; i++) {
        fout << i+1 << ',' << recall[i] << ',' << accuracy[i] << ',' << \
             f1[i] << endl;
    }
    cout << "Result in heavy_change_result.csv" << endl;
}

int main() {
    heavy_change();
    return 0;
}
