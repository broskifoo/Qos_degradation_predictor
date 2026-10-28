# Learning-Assisted QoS Degradation Prediction in Multi-Flow Packet-Switched Networks

A complete modular **C++17** discrete-event network simulator that predicts Quality of Service (QoS) degradation in real time using a hand-built Logistic Regression model, rendered through a live **Raylib 3D dashboard**.

---
![WhatsApp Image 2026-03-21 at 1 13 23 AM](https://github.com/user-attachments/assets/db9a322f-ce2a-4ba9-beab-6344bfbcdf5c)

## Features<img width="1460" height="741" alt="Screenshot 2026-03-21 010618" src="https://github.com/user-attachments/assets/da8f652b-3f1e-47b2-bfb3-66d41f03f7a7" />

<img width="1197" height="731" alt="Screenshot 2026-03-21 011919" src="https://github.com/user-attachments/assets/16c906ad-387c-4be2-b1d4-b6e3315c0ef1" />

| Module | Description |
|--------|-------------|
| **Multi-Flow Traffic** | CBR/UDP real-time flows + TCP AIMD flows with RTT-based congestion control |
| **Multi-Path Self-Healing Routing** | 3 parallel paths with least-delay routing; automatic path switching on congestion |
| **AQM (RED)** | Adaptive queue management — early drop probability rises with queue fill |
| **Anomaly Detection** | Z-score sliding-window anomaly detector (CONGESTION_SPIKE / BURST_TRAFFIC) |
| **Cross-Layer TCP Feedback** | Anomaly events directly throttle TCP CWND via cross-layer signals |
| **Logistic Regression ML** | Manual gradient-descent binary classifier predicts QoS threshold violations |
| **Live 3D Dashboard** | Raylib-powered real-time topology view + QoS graph + side-panel metrics |
| **ML Results Screen** | Post-simulation window: accuracy bars, confusion matrix, prediction curve |

---

## Architecture

```
src/
├── main.cpp                  # Entry point — simulation → ML training → results display
├── config/                   # Config loader (JSON)
├── network/                  # Packet, Link, Queue (FIFO + RED AQM)
├── simulator/                # Event loop, TCPFlow, RealTimeFlow, multi-path routing
├── qos/                      # MetricsTracker, Logger (CSV dataset writer)
├── ml/                       # LogisticRegression, feature correlation analysis
└── visualization/            # Dashboard (Raylib 3D + 2D panels, ML results screen)
config/
└── simulation_config.json    # Tunable: sim time, bandwidth, flows, ML lr/epochs
data/
└── qos_dataset.csv           # Auto-generated during simulation
results/
├── metrics.txt               # Accuracy, Precision, Recall, F1
├── confusion_matrix.txt      # TP / FP / FN / TN
└── model_weights.txt         # Trained logistic regression weights
```

---

## QoS Math

Following classical queuing theory:

$$D_{\text{Total}} = D_{\text{tx}} + D_{\text{queue}} + D_{\text{prop}}$$

$$D_{\text{tx}} = \frac{\text{Packet Size (bytes)} \times 8}{\text{Bandwidth (bps)}} \qquad \text{Jitter} = |D_i - D_{i-1}|$$

QoS is labelled **degraded** (`1`) when:
- Mean Epoch Delay **> 50 ms**, OR
- Mean Epoch Jitter **> 10 ms**, OR
- Frame Loss **> 5%**

---

## Build (Windows — CMake + MinGW or MSVC)

```powershell
cd D:\CNlab\QoS_Network_Simulator\build
cmake ..
cmake --build .
```

---

## Run

> **Important:** always run from inside `build/` so relative paths (`../data/`, `../config/`) resolve correctly.

```powershell
cd D:\CNlab\QoS_Network_Simulator\build
.\QoS_Simulator.exe
```

### Execution Flow

| Phase | What happens |
|-------|-------------|
| **Simulation (0 – 300 s)** | Live 3D dashboard: topology, QoS graph, side-panel metrics. Toggle Baseline ↔ Enhanced mode with the button. |
| **Transition** | Animated "please wait" screens while dataset is loaded and ML model is trained |
| **ML Results** | Same window shows: Performance Metrics (Accuracy / Precision / Recall / F1), Confusion Matrix, Predictive Model Curve |
| **Exit** | Click **X** to close — results saved to `results/` |

---

## Outputs

| File | Contents |
|------|----------|
| `data/qos_dataset.csv` | Per-epoch feature rows: RTT, jitter, loss, throughput, queue occupancy, label |
| `results/metrics.txt` | Accuracy, Precision, Recall, F1 Score |
| `results/confusion_matrix.txt` | TP / FP / FN / TN counts |
| `results/model_weights.txt` | Final trained weights + bias of logistic regression |

---

## Configuration (`config/simulation_config.json`)

Key tunable parameters:

```json
{
  "simulation_time_sec": 300,
  "bandwidth_mbps": 10,
  "queue_size_packets": 100,
  "tcp_flows": 3,
  "learning_rate": 0.01,
  "epochs": 1000
}
```
