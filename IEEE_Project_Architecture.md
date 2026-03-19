# IEEE Project Architecture: Learning-Assisted Prediction of QoS Degradation

## 1. FULL SYSTEM ARCHITECTURE (Modular Design)
The upgraded simulator shifts from a basic Point-to-Point bottleneck into an **Adaptive Digital Twin Architecture**.
* **Traffic Subsystem:** Multi-class traffic generators (CBR Voice, VBR Video, TCP AIMD Bulk).
* **Routing/Topology Subsystem:** A multi-path topology graph containing several `Link` and `Queue` instances. It supports dynamic load balancing using Least-Delay pathing.
* **Smart Queuing Subsystem (AQM):** Queues are equipped with a `HybridAQM` module (RED + CoDel) dynamically tuned by predictive Logistic Regression outputs.
* **Network Anomaly Detector:** A statistical monitor riding on top of the Logger that evaluates $z$-scores against historical windows to identify micro-bursts or DDoS behavior.
* **Cross-Layer Optimizer (CLO):** A deterministic observer that orchestrates reactions across layers (e.g., instructing TCP sources to halve cwnd when Anomaly Detector signals stress).

## 2. CLASS DIAGRAM (C++ Responsibilities)
```cpp
// Core Components
class Simulator;           // Orchestrates physics timestep loops
class Topology;            // Manages routing logic, loads, and link state
class Node;                // Base class for Routers, Sources, Destinations
class Link;                // Represents physical medium (bandwidth, propagation)

// Traffic & Transport
class Flow; 
class TCPFlow : public Flow; // Implements cwnd, ssthresh, AIMD
class RealTimeFlow : public Flow; 

// The Brains
class Queue;                     // Upgraded to Hybrid AQM (RED + CoDel)
class AnomalyDetector;           // Statistical thresholding (Mean + 3*StdDev)
class CrossLayerOptimizer;       // Inter-layer signaling interface

// ML & Metrics
class LogisticRegression;  // Predictive engine 
class MetricsTracker;      // Sliding windows for RTT, Jitter, Throughput
```

## 3. DATA FLOW BETWEEN MODULES
1. **Generation:** `TCPFlow` generates `Packet` objects based on its `cwnd`.
2. **Routing Decision:** `Simulator` checks delay estimates and assigns the packet to an optimal parallel `Path`.
3. **Queuing:** The packet hits the Hybrid AQM `Queue`. 
4. **Active Queue Management:** AQM checks sojourn time (CoDel) and average queue size (RED). It decides whether to enqueue, drop, or mark the packet.
5. **Detection & Control:** Every `log_interval`, the network scans `MetricsTracker`. If an anomaly is found, Cross-Layer Optimization is pinged.
6. **Cross-Layer Feedback:** The system directly signals `TCPFlow` to throttle generation (reduce `cwnd`) to prevent a catastrophic queue drop cascading failure.

## 4. DETAILED ALGORITHMS
* **Hybrid AQM (RED + CoDel):** 
  * RED is used for **buffer occupancy control**; it computes $avg\_q$ via Exponential Weighted Moving Average (EWMA). If $avg\_q$ crosses $min\_th$, a probabilistic drop is applied.
  * CoDel battles **bufferbloat**. It limits *sojourn time*. If wait time > `TARGET` (e.g. 5ms) for longer than `INTERVAL` (e.g. 100ms), packets drop immediately on dequeue.
  * **ML Tuning:** If Logistic Regression suspects Degradation, `max_th` decreases.
* **Least-Delay Routing:** The router maintains estimate $D_p = D_{prop} + Q_{delay}$. It selects the path where $D_p$ is strictly minimized.
* **Anomaly Detection:** Tracks standard deviation $\sigma$ and mean $\mu$ of RTT. If current `target_rtt` > $\mu + 3\sigma$, triggers `CONGESTION_SPIKE`.

## 5. KEY C++ CODE SNIPPETS
**Multi-Path Routing (Least Delay Selection)**
```cpp
int best_path_idx = 0;
double min_delay = 999999.0;
for (int i = 0; i < paths.size(); i++) {
    double path_delay = paths[i].link->propagation_delay_ms + paths[i].queue->get_occupancy(); 
    if (path_delay < min_delay) {
        min_delay = path_delay;
        best_path_idx = i;
    }
}
```

## 6. INTEGRATION PLAN
* **Phase 1 (Completed):** Discard legacy bottleneck queue. Inject Hybrid AQM math (RED + CoDel).
* **Phase 2 (Completed):** Implement statistical Anomaly Detector tracking real-time sliding means. Implement Cross-Layer throttle triggers.
* **Phase 3:** Extract Topology out into a dynamically allocated vector of parallel `Link/Queue` pathways.
* **Phase 4:** Expand internal 3D Raylib rendering limits to support drawing matrices of network interfaces rather than straight single lines.

## 7. SIMULATION FLOW (Execution Loop)
1. Initialize Topology (Multiple Links/Load Balancer).
2. `Loop_Timestep:` (1ms increments)
3. **Generate Traffic:** TCP updates cwnd, fires packets.
4. **Route:** Select intelligent Least-Delay track.
5. **AQM Ingress:** Compute prob-drop. Buffer accepted.
6. **Cross-Layer Check:** Check RTT thresholds. Manipulate TCP state dynamically.
7. **Dequeue/Transmit:** Enforce propagation physics.
8. **Digital Twin Sync:** Pass statistical batches to Logistic Regression engine for learning.

## 8. PERFORMANCE METRICS TO EVALUATE
* **Bufferbloat Mitigation:** 99th percentile Delay mapping before vs. after CoDel integration.
* **Flow Fairness:** Jain's Fairness Index during AIMD congestion phases.
* **Network Stability Resilience:** Statistical recovery time mappings after induced anomalous traffic spikes.

## 9. COMPARISON
| Feature | Legacy Setup | Advanced Architecture |
| :--- | :--- | :--- |
| **Routing** | Static Single Link | Dynamic Least-Delay Auto-Balancing |
| **Queuing** | Tail-drop FIFO | Hybrid RED + CoDel (Active QM) |
| **Reaction** | TCP drops implicitly | Explicit Cross-Layer signaling / Detection |
| **Rendering** | Constant FPS Lock | Decoupled Real-Time Physics Accel |

## 10. IDEAS FOR GRAPH PLOTS
1. **Buffer Occupancy over Time (FIFO vs RED vs CoDel):** Plot showing FIFO oscillating wildly (sawtooth) while CoDel keeps the queue short and flat.
2. **Path Utilization Overlay:** A dual-line graph showing Traffic load transitioning seamlessly from Path A to Path B identically mirroring a delay spike anomaly on Path A.
3. **Probability Plot Correlation:** Scatter plot mapping how TCP congestion windows adapt identically with the ML Logistic Sigmoid boundaries.

## 11. IEEE-STYLE PROJECT TITLE SUGGESTIONS
* *Cross-Layer Optimization and Adaptive AQM in Multi-Path Networks using Predictive Modeling*
* *A Digital Twin Approach to QoS Enforcement using Multi-Path Self-Healing Routing*
* *Learning-Assisted Active Queue Management and Dynamic Routing in Packet-Switched Networks*
* *Intelligent Mitigation of Traffic Anomalies via Hybrid RED-CoDel and Cross-Layer Controls*
