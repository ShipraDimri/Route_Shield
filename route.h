#ifndef ROUTER_H
#define ROUTER_H

#include <vector>
#include <unordered_set>

struct PacketPath {
    std::vector<int> path;
    double total_latency;
    bool is_safe;
    bool exists;
};

class NetworkRouter {
public:
    static PacketPath compute_optimal_path(
        int total_nodes,
        const std::vector<std::vector<std::pair<int, double>>>& adj_list,
        int src,
        int dest,
        const std::unordered_set<int>& blacklisted_nodes
    );
};

#endif