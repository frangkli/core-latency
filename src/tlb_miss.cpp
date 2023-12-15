#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>

int main() {
    // Create giant array
    const int array_size = INT16_MAX;
    alignas(64) std::vector<int> data(array_size);

    // Use this to hide standard output
    std::cout.setstate(std::ios::failbit);

    // Initialize the array
    for (int i = 0; i < array_size; ++i) {
        data[i] = i;
    }

    auto time_miss = std::chrono::nanoseconds::min();
    auto time_hit = std::chrono::nanoseconds::max();

    std::vector<int64_t> times(array_size);

    for (int i = 0; i < array_size; ++i) {
        // Try to read data and time latency, need to "fake print" to cout (or
        // do something with the read value) to avoid compiler optimizing the
        // loop content away
        auto tp1 = std::chrono::high_resolution_clock::now();
        int value = data[i];
        std::cout << value << std::endl;
        auto tp2 = std::chrono::high_resolution_clock::now();

        // Store result
        time_miss = std::max(time_miss, tp2 - tp1);
        time_hit = std::min(time_miss, tp2 - tp1);
        times[i] = (tp2 - tp1).count();
    }

    // Unhide standard output
    std::cout.clear();

    // Print max and min
    std::cout << "Time taken with TLB misses: " << time_miss.count()
              << " nanoseconds." << std::endl;
    std::cout << "Time taken with TLB hits: " << time_hit.count()
              << " nanoseconds." << std::endl;

    // Store sorted latency data in csv file
    std::ofstream csvfile;
    csvfile.open("data/tlb_results.csv", std::ofstream::trunc);
    for (size_t i = 0; i < times.size(); i++) {
        csvfile << times[i] << std::endl;
    }

    return 0;
}
