// Standalone test for Graph (does not touch route.cpp / Intrusion.cpp)
#include "graph.h"
#include "route.h"
#include "Intrusion.h"
#include <iostream>
#include <vector>

void printMenu() {
    std::cout << "\n================ ROUTE SHIELD CLI ================\n";
    std::cout << "1. View Network Topology\n";
    std::cout << "2. Find Optimal Path (Source -> Destination)\n";
    std::cout << "3. Simulate Packet Traffic / Attack on Router\n";
    std::cout << "4. View Blacklisted Routers & IDS Report\n";
    std::cout << "5. Reset/Clear Traffic & Blacklist\n";
    std::cout << "6. Exit\n";
    std::cout << "Choose an option (1-6): ";
}

int main() {
    // 6-Router default network topology
    Graph net(6);
    net.addLink(0, 1, 2.0);
    net.addLink(0, 2, 4.0);
    net.addLink(1, 3, 3.0);
    net.addLink(2, 3, 1.0);
    net.addLink(3, 4, 2.0);
    net.addLink(4, 5, 5.0);

    // Intrusion detection threshold limit = 5 packets
    Intrusion ids(5);

    int choice;
    while (true) {
        printMenu();
        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(1000, '\n');
            continue;
        }

        if (choice == 1) {
            net.printTopology();
        }
        else if (choice == 2) {
            int src, dest;
            std::cout << "Enter Source Router ID (0 to 5): ";
            std::cin >> src;
            std::cout << "Enter Destination Router ID (0 to 5): ";
            std::cin >> dest;

            if (src < 0 || src >= net.getTotalNodes() || dest < 0 || dest >= net.getTotalNodes()) {
                std::cout << "[!] Invalid router IDs!\n";
                continue;
            }

            auto adj = net.toAdjList();
            PacketPath path = NetworkRouter::compute_optimal_path(net.getTotalNodes(), adj, src, dest, ids.getBlacklist());

            if (!path.path.empty()) {
                std::cout << "\n[+] Optimal Route: ";
                for (size_t i = 0; i < path.path.size(); ++i) {
                    std::cout << "R" << path.path[i] << (i + 1 < path.path.size() ? " -> " : "");
                }
                std::cout << "\n[+] Total Path Latency: " << path.total_latency << " ms\n";
            } else {
                std::cout << "\n[-] No safe path available! All paths blocked or endpoints compromised.\n";
            }
        }
        else if (choice == 3) {
            int target, packets;
            std::cout << "Enter Router ID to target (0 to 5): ";
            std::cin >> target;
            std::cout << "Enter number of incoming packets (Threshold is 5): ";
            std::cin >> packets;

            ids.addTraffic(target, packets);
            std::cout << "[*] Traffic injected successfully.\n";
            ids.printReport();
        }
        else if (choice == 4) {
            ids.printReport();
        }
        else if (choice == 5) {
            ids = Intrusion(5);
            std::cout << "[*] Intrusion monitor and blacklists reset.\n";
        }
        else if (choice == 6) {
            std::cout << "Exiting Route Shield CLI. Goodbye!\n";
            break;
        }
        else {
            std::cout << "[!] Invalid option, please enter 1-6.\n";
        }
    }

    return 0;
}