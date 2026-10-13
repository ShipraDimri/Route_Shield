#ifndef INTRUSION_H
#define INTRUSION_H

#include <unordered_map>
#include <unordered_set>

class Intrusion
{
private:
    std::unordered_map<int, int> trafficCount;
    std::unordered_set<int> blacklist;

    int threshold;

public:
    // Constructor
    Intrusion(int limit = 200);

    // Add one request
    void addTraffic(int routerID);

    // Add multiple requests
    void addTraffic(int routerID, int requests);

    // Check if router crossed threshold
    void detectIntrusion(int routerID);

    // Check blacklist
    bool isBlacklisted(int routerID) const;

    // Reset traffic count
    void resetTraffic(int routerID);

    // Print blacklist
    void printBlacklist() const;

    // Print intrusion report
    void printReport() const;

    // Return blacklist
    const std::unordered_set<int>& getBlacklist() const;
};

#endif