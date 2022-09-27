#include "micro.h"
#include "trace.h"
#include <bits/stdc++.h>
using namespace std;

vector<double> calc_error(const multiset<uint32_t> &real, const vector<pair<int, int>> &est, double p0) {
    int sum = 0, tot = 0;
    for (auto [avg, cnt]: est) {
        tot += cnt;
    }
    vector<pair<double, double>> points;
    points.push_back(make_pair(0, 0));
    for (auto [avg, cnt]: est) {
        assert(cnt != 0);
        sum += cnt / 2;
        double q = (double)sum / tot;
        points.push_back(make_pair(log10(avg), q));
        sum += (cnt + 1) / 2;
    }
    points.push_back(make_pair(log10(1 << 23), 1));

    tot = real.size();
    vector<int> interval_vec;
    for (auto interval: real) {
        interval_vec.push_back(interval);
    }
    
    vector<double> res;
    for (double p = p0; p < 1; p += p0) {
        double qx = log10(interval_vec[real.size()*p]);
        for (unsigned i = 0; i < points.size(); i++) {
            if (i+1 == points.size() || points[i].first <= qx && qx <= points[i+1].first) {
                double est_y = i+1 == points.size() ? 1 : \
                               points[i].second + (qx - points[i].first)\
                               * (points[i+1].second - points[i].second)\
                               / (points[i+1].first - points[i].first);
                double error = est_y - p;
                res.push_back(fabs(error));
                break;
            }
        }
    }
    return res;
}

void distribution_estimation() {
    MicroSketch micro;
    vector<pair<TUPLES, uint32_t>> data = loadCAIDA18();
    unordered_map<TUPLES, int> flow_size;
    unordered_map<TUPLES, uint32_t> last_time;
    unordered_map<TUPLES, multiset<uint32_t>> real_interval;
    for (auto [key, tm]: data)
        flow_size[key]++;

    cout << "inserting..." << endl;
    
    for (auto [key, tm]: data) {
        if (last_time.count(key)) {
            int interval = tm - last_time[key];
            micro.insert(key, tm, interval);
            real_interval[key].insert(interval);
        }
        last_time[key] = tm;
    }

    cout << "evaluating..." << endl;

    vector<pair<int, TUPLES>> flow_list = micro.flow_list();
    sort(flow_list.begin(), flow_list.end(), greater<pair<int, TUPLES>>());

    const int point_cnt = 19;
    double p0 = 1.0 / (point_cnt + 1);
    vector<double> error(point_cnt, 0);
    double result[5][point_cnt];
    for (unsigned i = 0; i < 500; i++) {
        auto [size, key] = flow_list[i];
        vector<pair<int, int>> est = micro.query_delay(key);
        vector<double> cur_error = calc_error(real_interval[key], est, p0);
        for (int j = 0; j < point_cnt; j++)
            error[j] += cur_error[j];
        if (i % 100 == 99) {
            for (int j = 0; j < point_cnt; j++)
                result[i/100][j] = error[j] / (i+1);
        }
    }

    ofstream fout("distribution_result.csv");
    fout << "Quantile,Top-100 AE,Top-200 AE,Top-300 AE,Top-400 AE,Top-500 AE" << endl;
    double p = p0;
    for (int i = 0; i < point_cnt; i++, p += p0) {
        fout << p;
        for (int k = 0; k < 5; k++) {
            fout << ',' << result[k][i];
        }
        fout << endl;
    }
    cout << "Result in distribution_result.csv" << endl;
}

int main() {
    distribution_estimation();
    return 0;
}
