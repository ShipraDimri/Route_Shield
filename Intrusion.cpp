#include "Intrusion.h"
#include <iostream>

using namespace std;

// Constructor
Intrusion::Intrusion(int limit)
{
    threshold = limit;
}

// Add one request
void Intrusion::addTraffic(int routerID)
{
    trafficCount[routerID]++;
    detectIntrusion(routerID);
}

// Add multiple requests
void Intrusion::addTraffic(int routerID, int requests)
{
    trafficCount[routerID] += requests;
    detectIntrusion(routerID);
}

// Detect intrusion
void Intrusion::detectIntrusion(int routerID)
{
    if (trafficCount[routerID] > threshold)
    {
        if (blacklist.find(routerID) == blacklist.end())
        {
            blacklist.insert(routerID);

            cout << "\n=====================================\n";
            cout << " INTRUSION ALERT\n";
            cout << "=====================================\n";
            cout << "Router R" << routerID
                 << " has exceeded the traffic threshold.\n";
            cout << "Router added to blacklist.\n";
        }
    }
}

// Check blacklist
bool Intrusion::isBlacklisted(int routerID) const
{
    return blacklist.find(routerID) != blacklist.end();
}

// Reset traffic
void Intrusion::resetTraffic(int routerID)
{
    trafficCount[routerID] = 0;
}

// Print blacklist
void Intrusion::printBlacklist() const
{
    cout << "\n========== BLACKLIST ==========\n";

    if (blacklist.empty())
    {
        cout << "No blacklisted routers.\n";
        return;
    }

    for (int router : blacklist)
    {
        cout << "Router R" << router << endl;
    }
}

// Print report
void Intrusion::printReport() const
{
    cout << "\n========== INTRUSION REPORT ==========\n";

    if (blacklist.empty())
    {
        cout << "No intrusion detected.\n";
        return;
    }

    for (int router : blacklist)
    {
        auto it = trafficCount.find(router);

        int requests = 0;

        if (it != trafficCount.end())
            requests = it->second;

        cout << "Router      : R" << router << endl;
        cout << "Traffic     : " << requests << " requests" << endl;
        cout << "Threshold   : " << threshold << " requests" << endl;
        cout << "Status      : BLACKLISTED" << endl;
        cout << "--------------------------------------\n";
    }
}

// Return blacklist
const unordered_set<int>& Intrusion::getBlacklist() const
{
    return blacklist;
}