#include "route.h"
#include <queue>
#include <limits>
#include <algorithm>

PacketPath NetworkRouter::compute_optimal_path(
    int total_nodes,
    const std::vector<std::vector<std::pair<int, double>>>& adj_list,
    int src,
    int dest,
    const std::unordered_set<int>& blacklisted_nodes
) {
    std::vector<double> min_latency(total_nodes, std::numeric_limits<double>::infinity());
    std::vector<int> parent(total_nodes, -1);
    
    if(blacklisted_nodes.count(src)||blacklisted_nodes.count(dest)){
        return {{},0.0,false,false};
    }

    // Min-heap priority queue: pair<latency, node_id>
    std::priority_queue<std::pair<double, int>, 
                        std::vector<std::pair<double, int>>, 
                        std::greater<std::pair<double, int>>> pq;

    if (src == dest) {
        return {{src}, 0.0, true, true};
    }

    min_latency[src] = 0.0;
    pq.push({0.0, src});

    while (!pq.empty()) {
        auto [current_latency, u] = pq.top();
        pq.pop();

        if (current_latency > min_latency[u]) continue;
        if (u == dest) break;

        for (const auto& edge : adj_list[u]) {
            int v = edge.first;
            double latency = edge.second;

            // Skip blacklisted nodes
            if (blacklisted_nodes.count(v)) continue;

            if (min_latency[u] + latency < min_latency[v]) {
                min_latency[v] = min_latency[u] + latency;
                parent[v] = u;
                pq.push({min_latency[v], v});
            }
        }
    }
PacketPath result;

    // Agar destination tak rasta hi nahi mila
    if (min_latency[dest] == std::numeric_limits<double>::infinity()) {
        result.exists = false;
        result.is_safe = false;
        result.total_latency = 0.0;
        return result;
    }

    result.exists = true;
    result.is_safe = true;
    result.total_latency = min_latency[dest];

    // Backtrack path reconstruction
    for (int curr = dest; curr != -1; curr = parent[curr]) {
        result.path.push_back(curr);
    }
    std::reverse(result.path.begin(), result.path.end());

    return result;
}