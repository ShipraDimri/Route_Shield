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

    // Record one traffic request
    void addTraffic(int routerID);

    // Record multiple traffic requests
    void addTraffic(int routerID, int requests);

    // Detect intrusion
    void detectIntrusion(int routerID);

    // Check whether router is blacklisted
    bool isBlacklisted(int routerID) const;

    // Remove router from blacklist
    void removeFromBlacklist(int routerID);

    // Reset router traffic
    void resetTraffic(int routerID);

    // Print blacklist
    void printBlacklist() const;

    // Print intrusion report
    void printReport() const;

    // Return blacklist
    const std::unordered_set<int>& getBlacklist() const;
};

#endif