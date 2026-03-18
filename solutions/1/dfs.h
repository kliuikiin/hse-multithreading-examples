#pragma once

#include "generator.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>

using Graph = std::unordered_map<int, std::vector<int>>;

Generator<int> DFS(const Graph& graph, int start) {
    std::unordered_set<int> visited;
    std::vector<int> stack = {start};

    while (!stack.empty()) {
        int node = stack.back();
        stack.pop_back();

        if (visited.count(node)) continue;
        visited.insert(node);

        co_yield node;

        if (graph.count(node)) {
            for (int neighbor : graph.at(node)) {
                if (!visited.count(neighbor)) {
                    stack.push_back(neighbor);
                }
            }
        }
    }
}
