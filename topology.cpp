// Standalone test for Graph (does not touch route.cpp / Intrusion.cpp)
#include "graph.h"
#include <iostream>
using namespace std;

int main()
{
    Graph net(6);

    net.addLink(0, 1, 2);
    net.addLink(0, 2, 4);
    net.addLink(1, 3, 3);
    net.addLink(2, 3, 1);
    net.addLink(3, 4, 2);
    net.addLink(4, 5, 5);

    net.printTopology();

    return 0;
}
