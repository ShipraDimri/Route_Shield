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
};

#endif