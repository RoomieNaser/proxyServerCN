# Computer Networks Mini-Project: HTTP Proxy Server

## Team Members
* Rumman Naser (2405394)- Core Socket Logic, Concurrency (Multithreading), HTTP Parsing, and Caching Architecture
* Shreekant Singh (24051740)- Access Control (Domain Blacklisting), Traffic Logging, Performance Measurements, and Documentation

## Project Overview
We have chosen **Project 1: Designing a Proxy Server**. The goal of this system is to build an intermediary that sits between clients and the internet. Our proxy will handle concurrent client connections, parse and forward HTTP requests, implement a caching mechanism, and enforce domain-based access control. 

## AI Usage Statement
To maintain transparency, our team is utilizing Generative AI tools in the following specific capacities for this project:
*   **Debugging:** We are using AI to assist in troubleshooting thread synchronization issues, memory management, and general C/C++ syntax errors within our Linux WSL development environment.
*   **Documentation:** AI is used to generate, format, and structure documentation, including this README file and our architectural write-up.
*   **Conceptual Learning:** We utilize AI to direct us toward documentation and resources that help us understand core networking concepts, such as socket programming, concurrency models, and HTTP caching strategies. 
*   **Originality:** All core architectural decisions, code implementation, and testing are conducted manually by the team.

## System Architecture & Design
*   **Core Logic & Concurrency:** The proxy will be written in C/C++ using POSIX sockets. To accept connections from multiple simultaneous clients without one client's request blocking another, the server will utilize a multithreaded architecture where each incoming connection is dispatched to a worker thread.
*   **Networking Protocol:** The system will correctly parse and forward requests for the HTTP protocol. It will extract necessary headers to resolve the intended destination, forward the request, and route the remote server's response back to the client.
*   **Caching Strategy:** Responses to HTTP `GET` requests will be cached in an in-memory data structure with a sensible policy for when cached content is served vs. re-fetched. 
*   **Access Control & Logging:** A blacklist configuration will be maintained to block specific domains/URLs. All passing traffic will be logged in a way that would be useful to a network administrator.

## Trade-offs and Key Decisions
*   **Concurrency: Thread-per-Connection vs. Event-Driven (`epoll`)**
    *   *Approach:* We are utilizing a thread-pool or thread-per-connection model.
    *   *Trade-off:* While an event-driven loop handles high concurrency with less memory overhead, a multithreaded model significantly simplifies the synchronous parsing of HTTP headers and blocking socket I/O operations.
*   **Caching: In-Memory vs. Disk Storage**
    *   *Approach:* The cache will be stored locally in memory.
    *   *Trade-off:* In-memory caching provides the lowest possible latency for cache hits. The trade-off is volatility (the cache clears on server restart) and strict memory limits.
*   **Edge Case Handling**
    *   *Approach:* The proxy will handle malformed requests and unreachable destinations gracefully rather than crashing. It will generate and return standard HTTP error codes (e.g., `400 Bad Request` or `502 Bad Gateway`) to the client.

## Setup and Run Instructions
1. Clone the repository: `git clone https://github.com/RoomieNaser/proxyServerCN`
2. Compile the source: `gcc proxy.c -lpthread -o proxy`
3. Run the application: `./proxy --port 8080`

## Live Demonstration Plan
During the live evaluation, our demonstration will showcase:
1. Normal request forwarding through a browser.
2. A cache hit vs. a cache miss. We will measure and report the performance overhead our proxy introduces compared to a direct connection.
3. At least one access-control rule in action actively blocking a blacklisted domain.
4. The proxy gracefully handling an unreachable host or malformed request without crashing.
