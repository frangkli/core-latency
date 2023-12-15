#include <pthread.h>
#include <string.h>

#include <atomic>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <thread>
#include <vector>

constexpr int NUM_SAMPLES = 1000;
constexpr int NUM_OPS = 100;

enum OP_TYPE { READ, WRITE };

// Gets a vector of available CPU cores
void getCores(std::vector<int>& cores);

// Pins thread to cpu core specified by parameter
void pinThread(int core);

// Print data in table format given data map and cores vector
void printData(
    const std::map<std::pair<int, int>, std::chrono::nanoseconds>& data,
    int coreNum, OP_TYPE op);

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

                for (int s = 0; s < NUM_SAMPLES; s++) {
                    switch (op) {
                        case READ: {
                            for (int n = 0; n < NUM_OPS; n++) {
                                while (n1.load(std::memory_order_acquire) != n)
                                    ;  // waiting for other thread
                                n2.store(n, std::memory_order_release);
                            }
                            break;
                        }
                        case WRITE: {
                            for (int n = 0; n < NUM_OPS; n++) {
                                int cmp;
                                do {
                                    cmp = 2 * n;
                                } while (
                                    !n1.compare_exchange_strong(cmp, cmp + 1));
                            }
                            break;
                        }
                    }
                }
            });

            std::chrono::nanoseconds rtt = std::chrono::nanoseconds::max();

            // Pin this thread to core j
            pinThread(j);

            for (int s = 0; s < NUM_SAMPLES; s++) {
                n1 = n2 = -1;
                switch (op) {
                    case READ: {
                        auto tp1 = std::chrono::high_resolution_clock::now();
                        for (int n = 0; n < NUM_OPS; n++) {
                            n1.store(n, std::memory_order_release);
                            while (n2.load(std::memory_order_acquire) != n)
                                ;  // waiting for other thread
                        }
                        auto tp2 = std::chrono::high_resolution_clock::now();
                        rtt = std::min(rtt, tp2 - tp1);
                        break;
                    }
                    case WRITE: {
                        auto tp1 = std::chrono::high_resolution_clock::now();
                        for (int n = 0; n < NUM_OPS; n++) {
                            int cmp;
                            do {
                                cmp = 2 * n - 1;
                            } while (!n1.compare_exchange_strong(cmp, cmp + 1));
                        }
                        while (n1.load(std::memory_order_acquire) != 199)
                            ;  // waiting for other thread
                        auto tp2 = std::chrono::high_resolution_clock::now();
                        rtt = std::min(rtt, tp2 - tp1);
                        break;
                    }
                }
            }

            t.join();

            auto result = rtt / 2 / NUM_OPS;
            data[{i, j}] = result;
            data[{j, i}] = result;
        }
    }

    // Print data table
    printData(data, cores.size(), op);

    return 0;
}

/*
 *  Helper Functions
 */

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

void pinThread(int core) {
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(core, &set);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &set) == -1) {
        perror("Error calling sched_setaffinity.");
        exit(1);
    }
}

void printData(
    const std::map<std::pair<int, int>, std::chrono::nanoseconds>& data,
    int coreNum, OP_TYPE op) {
    std::ofstream csvfile;
    std::string filename;
    switch (op) {
        case READ:
            filename = "data/cco_read_results.csv";
            break;
        case WRITE:
            filename = "data/cco_write_results.csv";
            break;
    }
    csvfile.open(filename, std::ofstream::trunc);
    csvfile << "Core 1,Core 2,Latency (Nanoseconds)" << std::endl;
    for (int i = 0; i < coreNum; i++) {
        for (int j = 0; j < coreNum; j++) {
            csvfile << i << "," << j << ",";
            if (i == j) {
                csvfile << 0;
            } else {
                csvfile << data.at({i, j}).count();
            }
            csvfile << std::endl;
        }
    }
}