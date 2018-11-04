#include "redis.h"


/**
 * 比较大小的函数
 * @author: cheng pan
 * @date: 2018.10.22
 */
long long maxer(long long a, long long b) {
    if (a > b) return a;
    return b;
} 

/**
 * 用来将一个长的reuse time进行压缩，以便存储
 * @author: cheng pan
 * @date: 2018.10.22
 */ 
long domain_value_to_index(long value) {
    long loc = 0,step = 1;
    int index = 0;
    while (loc+step*rth_domain<value) {
        loc += step*rth_domain;
        step *= 2;
        index += rth_domain;
    }
    while (loc<value) { index++; loc += step; }
    return index;
}

/**
 * 求一个压缩后的reuse time原来对应的大小（概数）
 * @author: cheng pan
 * @date: 2018.10.22
 */
long domain_index_to_value(long index) {
    long value = 0,step = 1;
    while (index>rth_domain) {
        value += step*rth_domain;
        step *= 2;
        index -= rth_domain;
    }
    while (index>0) {
        value += step;
        index--;
    }
    return value;
}

/**
 * 用于记录GET操作的效果，按照字节记录RTH
 * 返回值为该节点更新后的access time
 * @author: cheng pan
 * @date: 2018.10.22
 */ 
long long rthGet(rthRec *rth, robj *key, unsigned size, long long last_access_time) {
    int len = size;
    int isMiss = 0;
    if (last_access_time == 0) isMiss = 1;
    if (!isMiss) {
        int rt = (int)domain_value_to_index((rth->n - last_access_time));
        rth->rtd[rt] += len;
        rth->read_rtd[rt] += 1;
    }
    else {
        rth->rtd[0] += len;
        rth->read_rtd[0] += 1;
    }
    // 更新该key节点的last_acces_time
    last_access_time = rth->n;
    rth->n += len;
    return last_access_time;
}

/**
 * 用于记录UPDATE操作的效果，按照字节记录RTH
 * 返回值为该节点更新后的access time
 * @author: cheng pan
 * @date: 2018.10.22
 */
long long rthUpdate(rthRec *rth, robj *key, unsigned ori_size, unsigned cur_size, long long last_access_time) {
    unsigned b = ori_size, a = cur_size;
    int rt = (int)domain_value_to_index((rth->n - last_access_time));
    if (last_access_time != 0) { // 如果不是第一次写，则可以计算rt
        if (a>b) {
            rth->rtd[rt] += b;
            rth->rtd[0] += a-b;
        }
        else {
            rth->rtd[rt] += a;
            rth->rtd_del[rt] += b-a;
        }
    }
    else {
        // 第一次写操作，直接默认全部miss
        rth->rtd[0] += a;
    }
    // 更新该key节点的last_acces_time
    last_access_time = rth->n;
    rth->n += maxer(a, b);
    return last_access_time;
} 

/**
 * 计算MRC曲线的API，其中，tot_memory是总内存大小（bytes），PGAP是访问分配粒度（bytes），mrc是返回值
 * @author: cheng pan
 * @date: 2018.10.22
 */
void rthCalcMRC(rthRec *rth, long long tot_memory, unsigned int PGAP) {
    double sum = 0, tot = 0, N = 0;
    long step = 1; int dom = 1,dT = 1;
    long T = 0;

    double read_sum = 0;
    double read_N = 0;
    for (int i = 0; i < rth_RTD_LENGTH; i++) {
        read_N += rth->read_rtd[i];
        N += rth->rtd[i] + rth->rtd_del[i];
    }
    // System.out.println("N="+N + ", read_N="+read_N);
    rth->mrc = (double *)zmalloc((tot_memory / PGAP) * sizeof(double) + 10);
    memset(rth->mrc, 0, tot_memory / PGAP * sizeof(double) + 10);
    rth->mrc[0] = 1.0;
    if (read_N == 0) return;
    for (long c = PGAP; c <= tot_memory; c += PGAP) {
        while (dT < rth_RTD_LENGTH) {
    		double d = 1.0 * (rth->rtd[dT] + rth->rtd_del[dT]);
    		double tmp = step - (sum * step + d * (1 + step) / 2) / N;
    		if (tot + tmp > c) break;
            tot += tmp;
            T += step;
    		if (++dom>rth_domain) { dom = 1; step *= 2; }
    		sum += d;
            read_sum += 1.0 * rth->read_rtd[dT];
    		dT ++;
        }
    	long mid, be = 0, ed = step;
    	while (be < ed - 1) {
    		mid = (be + ed) >> 1;
    		double d = 1.0 * (rth->rtd[dT] + rth->rtd_del[dT]);
    		double tmp = mid - (sum * mid + d * (1 + mid) * mid / 2 / step) / N;
    		if (tot + tmp >= c) ed = mid; else be = mid;
        }
        double miss_ratio = (read_N - read_sum - 1.0 * rth->read_rtd[dT] / step * ed) / read_N;
       // if (c / PGAP == 4096) System.out.println("4mb, miss_ratio=" + miss_ratio);
        rth->mrc[(int)(c / PGAP)] = miss_ratio;
    }    
}

/**
 * 清空所有统计信息
 * @author: cheng pan
 * @date: 2018.10.22
 */
void rthClear(rthRec *rth) {
    memset(rth->rtd, 0, sizeof(rth->rtd));
    memset(rth->rtd_del, 0, sizeof(rth->rtd_del));
    memset(rth->read_rtd, 0, sizeof(rth->read_rtd));
    //memset(rth->mrc, 0, sizeof(rth->mrc));
    if (rth->mrc != NULL) zfree(rth->mrc);
    rth->mrc = NULL;
    rth->n = 0;
    rth->tot_penalty = 0;
} 
