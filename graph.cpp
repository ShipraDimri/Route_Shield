#include "Graph.h"
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