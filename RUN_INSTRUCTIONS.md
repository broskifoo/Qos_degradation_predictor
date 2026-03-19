# QoS Network Simulator - Run Instructions

This document provides the sequential step-by-step commands to build and run the QoS Network Simulator project, including the 3D visualization, ML evaluation dashboard, and mathematical logic explainer.

## Prerequisites
- CMake installed and added to PATH
- C++17 compliant compiler (GCC/MinGW, MSVC, or Clang)
- PowerShell (built-in on Windows)

---

## 1. Compile the Project
Open a terminal in the root directory `QoS_Network_Simulator` and run the following CMake configuration and build commands:

```cmd
cmake -B build -S .
cmake --build build
```
*(If you have already compiled the project, you only need to run the second command whenever you make code changes.)*

---

## 2. Run the Main Simulation
The simulation **must** be executed from within the `build` directory so that it can locate the proper data and config subdirectories automatically. 

**Navigate to the build folder and start the simulation:**
```cmd
cd build
.\QoS_Simulator.exe
```

### What to expect:
1. A **3D Network Visualization** trace will pop up representing live network traffic between queue bottlenecks.
2. The simulation will run for ~300 seconds (or slightly faster depending on PC speed).
3. Once the packets have finished tracking, the engine trains the **Logistic Regression** predictor on the newly generated metrics dataset.
4. An interactive **ML Results Dashboard** will automatically open. The third panel in this screen visually plots the `Queue Occupancy against the Decision Boundary Predicton Score`. Check out the curve representing the model's intelligence! 
5. The terminal console running the simulation will also output the full math formulas evaluated by the framework.

*(Note: The simulation continuously blocks the terminal until you close the Dashboard window.)*

---

## 3. Explaining the AI Math Separately (Bonus)
If you just want an isolated terminal window demonstrating exactly the underlying equations and `Sigmoid (e^-z)` calculations powering the predictions:

**Open a new terminal window in the root directory and run:**
```cmd
.\show_math.bat
```
This batch file dynamically queries your trained model's freshly exported `results/model_weights.txt` weights and evaluates an interactive explanation inside an isolated Windows PowerShell screen!
