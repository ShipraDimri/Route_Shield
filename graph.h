#ifndef GRAPH_H
#define GRAPH_H

#include <list>
#include <vector>
#include <utility>
#include <unordered_set>

// One outgoing link of a router: (neighbour router id, latency of the link)
struct Edge
{
    int to;
    double latency;
};

// Network topology stored as an adjacency list using std::list
class Graph
{
private:
    int totalNodes;
    std::vector<std::list<Edge>> adjList;

    bool validNode(int id) const;
    bool linkExists(int u, int v) const;

public:
    Graph(int n = 0);
    int addRouter();
    bool addLink(int u, int v, double latency);
    bool removeLink(int u, int v);
    bool updateLatency(int u, int v, double latency);

    int getTotalNodes() const;
    int getTotalLinks() const;
    const std::list<Edge>& getNeighbours(int id) const;
    bool hasLink(int u, int v) const;

    // Format used by NetworkRouter::compute_optimal_path in route.h
    std::vector<std::vector<std::pair<int, double>>> toAdjList() const;

    // Same as toAdjList() but without any link touching a blacklisted router
    std::vector<std::vector<std::pair<int, double>>> toAdjList(
        const std::unordered_set<int>& blacklisted) const;

    void printTopology() const;
};

#endif
