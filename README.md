# Route_Shield
Adaptive Network Routing Simulator with Real-Time Intrusion Detection &amp; Node Blacklisting in C/C++
In real-world networks, routing engines and security monitors usually operate independently. If a router is compromised or gets flooded with traffic, standard routing algorithms keep pushing packets through it.

Route-Shield combines both into a single system:
Finds optimal data paths using Dijkstra's Algorithm.
Continuously tracks traffic for abnormal spikes or malicious patterns via an Intrusion Monitor.
 Automatically blacklists compromised routers and computes safe alternate paths in real time.

---

### Key Features

Network Graph Representation: Network topology modeled using Adjacency Lists.
Dijkstra’s Routing Engine: Computes shortest paths using a Min-Heap Priority Queue.
Congestion Handling:Router buffers managed with FIFO and Priority Queues for high-priority packets.
Intrusion Detection: Tracks request rates per IP using Hash Maps (`std::unordered_map`) and blacklists nodes (`std::unordered_set`).
Dynamic Rerouting: Automatically bypasses blacklisted nodes without restarting the simulation.