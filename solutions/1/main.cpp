#include "dfs.h"

#include <iostream>

int main() {
    // Тестовый граф: 0 подключен к 1,2; 1 к 3,4; 2 к 5
    Graph graph = {
        {0, {1, 2}},
        {1, {0, 3, 4}},
        {2, {0, 5}},
        {3, {1}},
        {4, {1}},
        {5, {2}},
    };

    std::cout << "DFS from node 0:\n";
    auto gen = DFS(graph, 0);
    while (gen.Next()) {
        std::cout << "  visited: " << gen.Value() << "\n";
    }

    return 0;
}
