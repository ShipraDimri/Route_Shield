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