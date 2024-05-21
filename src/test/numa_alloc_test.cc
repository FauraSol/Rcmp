/**
 * 背景测试：存在数据热点的情况下
 * 随机分配
 * 循环分配
 * 热点感知分配的差别
*/

#include <iostream>
#include <random>
#include <chrono>
#include <cstdlib>
#include <vector>
#include <numeric>
#include <numa.h>
#include <unistd.h>
#include <immintrin.h>
#include "cmdline.h"
#include "stats.hpp"
#include "log.hpp"

#define ALLOC_UNIT 8
#define MAX_ADDR 128 * 1024 * 1024 //128M * 8B = 1GB
#define HOT_ADDR_THRED (MAX_ADDR * 0.2) //8-2定律，20% 热数据
#define ITERATION 200000
#define NO_CACHE

using namespace std;

vector<void*> addr_map;

void test_init(){
    cout << "alloc test init begin..." << endl;
    //分配内存并且生成负载
    
    cout << "alloc test init end..." << endl;
}

inline double cal_avg(vector<int64_t>& vec) {
    auto sum = std::reduce(vec.begin(), vec.end());
    return static_cast<double>(sum) / vec.size();
}

inline void run_random_sample(zipf_distribution<> &zipf_distr, mt19937_64 &eng,vector<int64_t>& durations){
    for(int i=0;i<ITERATION;i++){
        int v_addr = zipf_distr(eng);
        bool local = eng()%2;
        auto start = std::chrono::high_resolution_clock::now();
        if(!addr_map[v_addr]){
            if(local == 0){
                addr_map[v_addr] = numa_alloc_onnode(ALLOC_UNIT, 0);
            }else{
                addr_map[v_addr] = numa_alloc_onnode(ALLOC_UNIT, 1);
            }
        }
        volatile uint8_t* ptr = reinterpret_cast<volatile uint8_t*>(addr_map[v_addr]);
        auto v = *ptr;
        // memset(addr_map[v_addr],i,ALLOC_UNIT);
#ifdef NO_CACHE
        _mm_clflush((void*)ptr);
        _mm_sfence();
#endif
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        durations.push_back(duration);
    }
    cout << "random sample time: " << cal_avg(durations) << "ns" << endl;
}

inline void run_cycle_sample(zipf_distribution<> &zipf_distr, mt19937_64 &eng,vector<int64_t>& durations){
    bool local = 0;
    for(int i=0;i<ITERATION;i++,local = !local){
        int v_addr = zipf_distr(eng);
        auto start = std::chrono::high_resolution_clock::now();
        if(!addr_map[v_addr]){
            if(local){
                addr_map[v_addr] = numa_alloc_onnode(ALLOC_UNIT, 0);
            }else{
                addr_map[v_addr] = numa_alloc_onnode(ALLOC_UNIT, 1);
            }
            local = !local;
        }
        volatile uint8_t* ptr = reinterpret_cast<volatile uint8_t*>(addr_map[v_addr]);
        auto v = *ptr;
        // memset(addr_map[v_addr],i,ALLOC_UNIT);
#ifdef NO_CACHE
        _mm_clflush((void*)ptr);
        _mm_sfence();
#endif
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        durations.push_back(duration);
    }
    cout << "cycle sample time: " << cal_avg(durations) << "ns" << endl;
}

inline void run_zipf_sample(zipf_distribution<> &zipf_distr, mt19937_64 &eng,vector<int64_t>& durations){
    for(int i=0;i<ITERATION;i++){
        int v_addr = zipf_distr(eng);
        if(v_addr <= HOT_ADDR_THRED){
            auto start = std::chrono::high_resolution_clock::now();
            if(!addr_map[v_addr]){
                addr_map[v_addr] = numa_alloc_onnode(ALLOC_UNIT, 0);
            }
            volatile uint8_t* ptr0 = reinterpret_cast<volatile uint8_t*>(addr_map[v_addr]);
            auto v = *ptr0;
            // memset(addr_map[v_addr],i,ALLOC_UNIT);
#ifdef NO_CACHE
            _mm_clflush((void*)ptr0);
            _mm_sfence();
#endif
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            durations.push_back(duration);
        }else{
            auto start = std::chrono::high_resolution_clock::now();
            if(!addr_map[v_addr]){
                addr_map[v_addr] = numa_alloc_onnode(ALLOC_UNIT, 1);
            }
            volatile uint8_t* ptr1 = reinterpret_cast<volatile uint8_t*>(addr_map[v_addr]);
            auto v = *ptr1;
            // memset(addr_map[v_addr],i,ALLOC_UNIT);
#ifdef NO_CACHE
            _mm_clflush((void*)ptr1);
            _mm_sfence();
#endif
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
            durations.push_back(duration);
        }
        
    }
    cout << "zipf sample time: " << cal_avg(durations) << "ns" << endl;
}

int main(){
    if (numa_available() < 0) {
        cerr << "NUMA support not available on this system." << endl;
        return 1;
    }
    mt19937_64 eng(0x9cfa2331b);
    zipf_distribution<> zipf_distr(MAX_ADDR,0.95);
    vector<int64_t> random_duration, cycle_duration, zipf_duration;
    addr_map.resize(MAX_ADDR,nullptr);
    run_random_sample(zipf_distr,eng,random_duration);
    for(int i=0;i<addr_map.size();i++){
        if(addr_map[i]){
            numa_free(addr_map[i],ALLOC_UNIT);
            addr_map[i] = nullptr;
        }
    }
    run_cycle_sample(zipf_distr ,eng,cycle_duration);
    for(int i=0;i<addr_map.size();i++){
        if(addr_map[i]){
            numa_free(addr_map[i],ALLOC_UNIT);
            addr_map[i] = nullptr;
        }
    }
    run_zipf_sample(zipf_distr, eng,zipf_duration);
    for(int i=0;i<addr_map.size();i++){
        if(addr_map[i]){
            numa_free(addr_map[i],ALLOC_UNIT);
            addr_map[i] = nullptr;
        }
    }
    return 0;
}

