#include <iostream>
#include <random>
#include <chrono>
#include <cstdlib>
#include <vector>
#include <numeric>
#include <numa.h>
#include <unistd.h>

constexpr size_t REGION_SIZE = 1024 * 1024 * 64; // 64MB per region
constexpr int NUM_REGIONS = 2;
constexpr int ITERATION = 100000;

using std::vector,std::cout,std::cerr,std::endl;

double cal_avg(vector<int64_t>& vec) {
    auto sum = std::reduce(vec.begin(), vec.end());
    return static_cast<double>(sum) / vec.size();
}

int main() {
    if (numa_available() < 0) {
        cerr << "NUMA support not available on this system." << endl;
        return 1;
    }


    void* region0 = numa_alloc_onnode(REGION_SIZE, 0);
    void* region1 = numa_alloc_onnode(REGION_SIZE, 1);

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dis(0, REGION_SIZE - 1);

    cout << "Testing same-NUMA access (node 0):" << endl;

    vector<int64_t> same_numa;
    vector<int64_t> cross_numa;

    //warm
    for (int i = 0; i < ITERATION; i++) {
        size_t offset0 = dis(gen);
        size_t offset1 = dis(gen);
        volatile uint8_t* ptr0 = static_cast<uint8_t*>(region0) + offset0;
        volatile uint8_t* ptr1 = static_cast<uint8_t*>(region1) + offset1;
        uint8_t tmp;
        tmp = *ptr0;
        *ptr0 = *ptr1;
        *ptr1 = tmp;
    }

    for (int i = 0; i < ITERATION; i++) {
        size_t offset0 = dis(gen);
        size_t offset1 = dis(gen);
        volatile uint8_t* ptr0 = static_cast<uint8_t*>(region0) + offset0;
        volatile uint8_t* ptr1 = static_cast<uint8_t*>(region0) + offset1;
        auto start = std::chrono::high_resolution_clock::now();
        *ptr0 = *ptr0 ^ *ptr1;
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        same_numa.push_back(duration);
    }

    cout << "Average time for same-NUMA access (node 0): " << cal_avg(same_numa) << "ns" << endl;

    cout << "Testing cross-NUMA access (node 0 to node 1):" << endl;
    for (int i = 0; i < ITERATION; i++) {
        size_t offset0 = dis(gen);
        size_t offset1 = dis(gen);
        volatile uint8_t* ptr0 = static_cast<uint8_t*>(region1) + offset0;
        volatile uint8_t* ptr1 = static_cast<uint8_t*>(region1) + offset1;
        auto start = std::chrono::high_resolution_clock::now();
        *ptr0 = *ptr0 ^ *ptr1;
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        cross_numa.push_back(duration);
    }

    cout << "Average time for cross-NUMA access (node 0 to node 1): " << cal_avg(cross_numa) << "ns" << endl;

    numa_free(region0, REGION_SIZE);
    numa_free(region1, REGION_SIZE);

    return 0;
}