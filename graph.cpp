#include "graph.h"
#include <iostream>
using namespace std;

Graph::Graph(int n)
{
    totalNodes = (n < 0) ? 0 : n;
    adjList.resize(totalNodes);
}

bool Graph::validNode(int id) const
{
    return id >= 0 && id < totalNodes;
}

bool Graph::linkExists(int u, int v) const
{
    for (const Edge& e : adjList[u])
    {
        if (e.to == v)
            return true;
    }
    return false;
}

int Graph::addRouter()
{
    adjList.emplace_back();
    return totalNodes++;
}

bool Graph::addLink(int u, int v, double latency)
{
    if (!validNode(u) || !validNode(v) || u == v || latency <= 0)
    {
        cout << "Invalid link R" << u << " - R" << v << endl;
        return false;
    }
    if (linkExists(u, v))
    {
        cout << "Link R" << u << " - R" << v << " already exists.\n";
        return false;
    }

    adjList[u].push_back({v, latency});
    adjList[v].push_back({u, latency});
    return true;
}

bool Graph::removeLink(int u, int v)
{
    if (!validNode(u) || !validNode(v) || !linkExists(u, v))
        return false;

    adjList[u].remove_if([v](const Edge& e) { return e.to == v; });
    adjList[v].remove_if([u](const Edge& e) { return e.to == u; });
    return true;
}

bool Graph::updateLatency(int u, int v, double latency)
{
    if (!validNode(u) || !validNode(v) || latency <= 0 || !linkExists(u, v))
        return false;

    for (Edge& e : adjList[u])
        if (e.to == v) e.latency = latency;
    for (Edge& e : adjList[v])
        if (e.to == u) e.latency = latency;
    return true;
}

int Graph::getTotalNodes() const
{
    return totalNodes;
}

int Graph::getTotalLinks() const
{
    int count = 0;
    for (const auto& l : adjList)
        count += l.size();
    return count / 2;   // every link is stored twice
}

const list<Edge>& Graph::getNeighbours(int id) const
{
    static const list<Edge> empty;
    if (!validNode(id))
        return empty;
    return adjList[id];
}

bool Graph::hasLink(int u, int v) const
{
    if (!validNode(u) || !validNode(v))
        return false;
    return linkExists(u, v);
}

vector<vector<pair<int, double>>> Graph::toAdjList() const
{
    vector<vector<pair<int, double>>> result(totalNodes);
    for (int u = 0; u < totalNodes; u++)
        for (const Edge& e : adjList[u])
            result[u].push_back({e.to, e.latency});
    return result;
}

vector<vector<pair<int, double>>> Graph::toAdjList(
    const unordered_set<int>& blacklisted) const
{
    vector<vector<pair<int, double>>> result(totalNodes);
    for (int u = 0; u < totalNodes; u++)
    {
        if (blacklisted.count(u))
            continue;
        for (const Edge& e : adjList[u])
            if (!blacklisted.count(e.to))
                result[u].push_back({e.to, e.latency});
    }
    return result;
}

void Graph::printTopology() const
{
    cout << "\n========== NETWORK TOPOLOGY ==========\n";
    cout << "Routers : " << totalNodes << "   Links : " << getTotalLinks() << "\n\n";
    for (int u = 0; u < totalNodes; u++)
    {
        cout << "R" << u << " -> ";
        if (adjList[u].empty())
            cout << "(no links)";
        for (const Edge& e : adjList[u])
            cout << "R" << e.to << "(" << e.latency << ") ";
        cout << endl;
    }
}
