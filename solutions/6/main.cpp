#include "process_pool.h"

#include <iostream>
#include <vector>

int square(int x) { return x * x; }

int main() {
    ProcessPool pool(3);

    std::vector<Future> futures;
    for (int i = 0; i < 8; ++i) {
        futures.push_back(pool.Submit(square, i));
    }

    for (int i = 0; i < 8; ++i) {
        std::cout << "Result[" << i << "] = " << futures[i].Get() << "\n";
    }

    return 0;
}
