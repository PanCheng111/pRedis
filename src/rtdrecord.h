#ifndef __RtdRecord_H__
#define __RtdRecord_H__

#include <unordered_map>
#include <cstdlib>
#include <cstring>
#include <assert.h>
#include <algorithm>
#include <queue>

class RtdRecord {
private:
    static const int MAXS = 1024+3;
    static const int LAMA_RTD_LENGTH = 10000+3;
    const int domain = 256; // compression ratio
    int rtd[LAMA_RTD_LENGTH]; // reuse time distribution, rtd[0] for cold miss
    int rtd_del[LAMA_RTD_LENGTH];
public:
    double mrc[MAXS], mrc_no_cold[MAXS];
    int aet[MAXS], aet_no_cold[MAXS];
    int total; // total access (include cold miss)

    int domain_value_to_index(int64_t value) {
        int index = 0,low2bit = value&(domain-1);
        while (value>=domain) {
            low2bit = value&(domain-1);
            value >>= 1;
            index += domain;
        }
        assert(index+low2bit<LAMA_RTD_LENGTH);
        return index+low2bit;
    }
    void insert(int64_t rt) { // insert one reuse time to rtd and penalty
        total++;
        if (rt<=0) rtd[0]++;
        else rtd[domain_value_to_index(rt)]++;
    }
    void insert_del(int64_t rt) {
        total++;
        rtd_del[domain_value_to_index(rt)]++;
    }
    void reset_mrc() {
        memset(rtd,0,sizeof(rtd));
        memset(rtd_del,0,sizeof(rtd_del));
        memset(mrc,0,sizeof(mrc));
        memset(aet,0,sizeof(aet));
        total = 0;
    }
    void next_step();
    void calc_mrc(int memory, int items);
    void calc_mrc_no_cold(int memory, int items);
};

void RtdRecord::calc_mrc(int memory, int items) {
    int32_t N = 0;
    int32_t read_N = 0;
    for (int32_t i = 0; i<LAMA_RTD_LENGTH; i++) {
        N += rtd[i]+rtd_del[i];
        read_N += rtd[i];
    }
    if (N==0) return;

    // ignore cold miss
    //read_N -= rtd[0];

    double sum = 0, tot = 0;
    double read_sum = 0;
    int32_t step = 1, T = 1;
    int32_t dom = -domain+1, dT = 1;
    for (int32_t c = 1; c<=memory; c++) {
        while (dT<LAMA_RTD_LENGTH) {
            double tmp = (N-sum)*step+(rtd[dT]-rtd_del[dT])*(step-1)/2.0;
            if ((tmp+tot)/N>=c*items) break;
            tot += tmp;
            T += step;
            if (++dom>domain) {
                dom = 1;
                step *= 2;
            }
            sum += rtd[dT]-rtd_del[dT];
            read_sum += rtd[dT];
            dT++;
        }
        if (dT==LAMA_RTD_LENGTH) {
            mrc[c] = 1-read_sum/read_N;
            continue;
        }
        int32_t bg = 0, ed = step, mid;
        while (bg+1<ed) {
            mid = (bg+ed)/2;
            double tmp = (N-sum)*mid+1.0*(rtd[dT]-rtd_del[dT])/step*mid*(mid-1)/2;
            if ((tmp+tot)/N>=c*items) ed = mid;
            else bg = mid;
        }
        aet[c] = T + ed;
        mrc[c] = 1-(read_sum+1.0*rtd[dT]/step*ed)/read_N;
    }
    mrc[0] = 1;
    aet[0] = 0;
}
/*************                  RtdRecord             ***************/

#endif // __RtdRecord_H__