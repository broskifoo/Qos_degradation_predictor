# Learning-Assisted QoS Degradation Prediction in Multi-Flow Packet-Switched Networks Simulator

This is a complete modular C++17 discrete-event simulation project that implements a time-driven packet-switched network simulator. It acts as a monitoring module that predicts expected Quality of Service (QoS) degradation occurrences inside a bottleneck link using purely computational Network Theory.

## Architecture and Components

The simulation architecture accurately mirrors classical computer networking theory and principles:

*   **Config module (`config`)**: Configurable simulation settings (Time, Link properties, flow properties, ML properties).
*   **Networking Models (`network`)**: Implements `Packet`, `Link` (computation of transmission schedules based on link bandwidth capacity), and a FIFO `Queue` to hold temporary arriving traffic. Both processing delay and propagation delays are supported.
*   **Traffic Generaturs (`simulator`)**: A discrete 1ms-granularity event loop orchestrates flows. Traffic sources include standard Constant Bit Rate (CBR/UDP) flows mimicking real-time streaming, and TCP Additive Increase Multiplicative Decrease (AIMD) flows calculating sending quotas based on RTT calculations.
*   **QoS Assessment & Logging (`qos`)**: Periodically tracks and aggregates RTT, jitter, queuing latency, packet drops, and transmission throughput metrics for all aggregated flows traversing the bottleneck.
*   **Predictor Engine (`ml`)**: Fully manual gradient-descent optimized binary classification Logistic Regression. The algorithm takes aggregated datasets matching periodic QoS epochs and determines QoS threshold failures.

## QoS Metrics Calculation (Network Theory Formulation)
Following classical queueing delays formulation:
1. $D_{\text{tx}} = \frac{\text{Packet Size (Bytes)} \times 8}{\text{Bottleneck Bandwidth (bps)}}$
2. $D_{\text{Total Path}} = D_{\text{tx}} + D_{\text{queue}} + D_{\text{prop}}$
3. $Jitter = | D_{i} - D_{i-1} | $

QoS is considered degraded (label `1`) when: Mean Epoch Delay $> 50$ms OR Mean Epoch Jitter $> 10$ms OR Frame Loss $> 5\%$.

## Compilation Steps (Windows CMake)

1.  Open your preferred command line/PowerShell terminal.
2.  Navigate to the `D:\CNlab\QoS_Network_Simulator\build` folder.
    ```bash
    cd D:\CNlab\QoS_Network_Simulator\build
    ```
3.  Execute CMake generator for your installed MSVC toolchain, then build the target.
    ```bash
    cmake ..
    cmake --build . --config Release
    ```

## Execution Steps

Run the generated executable directly from the `build` or `build/Release` folder (this is required to ensure it maps correctly to the data/results paths).

```bash
Release\QoS_Simulator.exe
```
Or if using single-config generators like MinGW:
```bash
.\QoS_Simulator.exe
```

## Outputs & Dataset

After execution, the following are updated:

*   `data/qos_dataset.csv`: Periodically generated features rows tracking traffic dynamics and QoS breakdown labels during simulation. Used later seamlessly by the Predictor.
*   `results/metrics.txt`: Model accuracy metrics representing predictive ML strength out of manual implementation.
*   `results/confusion_matrix.txt`: Diagnostic true positive/negative mapping outputs regarding prediction correctness over the generated testing folds.
