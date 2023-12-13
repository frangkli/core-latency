#include <pthread.h>
#include <string.h>

#include <atomic>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <thread>
#include <vector>

constexpr int NUM_SAMPLES = 1000;
constexpr int NUM_OPS = 100;

enum OP_TYPE { READ, WRITE };

// Gets a vector of available CPU cores
void getCores(std::vector<int>& cores) {
    cpu_set_t set;
    CPU_ZERO(&set);
    if (sched_getaffinity(0, sizeof(cpu_set_t), &set) == -1) {
        perror("Error calling sched_setaffinity.");
        exit(1);
    }
    for (int i = 0; i < CPU_SETSIZE; i++) {
        if (CPU_ISSET(i, &set)) {
            cores.push_back(i);
        }
    }
}

// Pins thread to cpu core specified by parameter
void pinThread(int core) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &set) == -1) {
        perror("Error calling sched_setaffinity.");
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    // Validate command-line arguments
    if (argc != 2 || (strcmp(argv[1], "-r") && strcmp(argv[1], "-w"))) {
        std::cerr << "Please specify -r (read) or -w (write)." << std::endl;
        exit(1);
    }

    const OP_TYPE op = !strcmp(argv[1], "-r") ? READ : WRITE;

    // Containers
    std::vector<int> cores;
    std::map<std::pair<int, int>, std::chrono::nanoseconds> data;

    // Get available cores
    getCores(cores);

    // Loop through all core combinations
    for (size_t i = 0; i < cores.size(); i++) {
        for (size_t j = i + 1; j < cores.size(); j++) {
            // Atomic values
            alignas(64) std::atomic<int> n1 = {-1};
            alignas(64) std::atomic<int> n2 = {-1};

            std::thread t = std::thread([&] {
                // Pin new thread to core i
                pinThread(i);

                // Read operation
                for (int s = 0; s < NUM_SAMPLES; s++) {
                    for (int n = 0; n < NUM_OPS; n++) {
                        while (n1.load(std::memory_order_acquire) != n)
                            ;  // waiting for other thread
                        n2.store(n, std::memory_order_release);
                    }
                }
            });

            std::chrono::nanoseconds rtt = std::chrono::nanoseconds::max();

            // Pin this thread to core j
            pinThread(j);

            // Read operation
            for (int s = 0; s < NUM_SAMPLES; s++) {
                n1 = n2 = -1;
                auto tp1 = std::chrono::steady_clock::now();
                for (int n = 0; n < NUM_OPS; n++) {
                    n1.store(n, std::memory_order_release);
                    while (n2.load(std::memory_order_acquire) != n)
                        ;  // waiting for other thread
                }
                auto tp2 = std::chrono::steady_clock::now();
                rtt = std::min(rtt, tp2 - tp1);
            }

            t.join();

            auto result = rtt / 2 / NUM_OPS;
            data[{i, j}] = result;
            data[{j, i}] = result;
        }
    }

    std::cout << std::setw(4) << "CPU";
    for (size_t i = 0; i < cores.size(); ++i) {
        std::cout << " " << std::setw(4) << i;
    }
    std::cout << std::endl;
    for (size_t i = 0; i < cores.size(); i++) {
        std::cout << std::setw(4) << i;
        for (size_t j = 0; j < cores.size(); j++) {
            std::cout << " " << std::setw(4) << data[{i, j}].count();
        }
        std::cout << std::endl;
    }

    return 0;
}