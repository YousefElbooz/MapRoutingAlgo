# MapRoutingAlgo
## A Qt-based desktop application for visual route finding on maps.
This was our project in the **Analysis & Design of
Algorithms Course**, it supports loading map graphs, running shortest-path queries, theming (light/dark), and interactive visualization.

## 📑 Table of contents
1. [Project description](#project-description)
2. [Implementation](#implementation)
3. [Screenshots](#screenshots)  
4. [Application Setup and Usage](#application-setup-and-usage)
5. [Limitations](#limitations)  

---

## Project description
The goal of this project was to design and implement a system that:
- Loads a graph representing a map (nodes + edges).  
- Accepts queries (start point, end point, max walking distance).  
- Computes the **shortest path** using graph algorithms.  
- Displays results with interactive visualization.  

👉 Full problem statement can be found here: 
[MapRoutingProjectDescription](Docs/MapRoutingProjectDescription.pdf)

---

## Implementation
- **Language & Framework**: C++17 with **Qt6** (Qt Widgets)  
- **Core Components**:  
  - `MapGraph` → Handles graph storage, shortest-path algorithms, and queries  
  - `MapVisualizer` → Custom widget for rendering map, path, and selections  
  - `MainWindow` → UI logic, query navigation, and file handling  
- **Algorithms**: Multisource Bidirectional Dijkstra’s algorithm (optimized with a priority queue)  
- **UI Features**:  
  - Zoom / Pan map navigation  
  - Start/End point selection (mouse or manual input)  
  - Theming with `.qss` files (light & dark)  
  - Query navigation (`Prev`, `Next`, Jump to index)  

---

## Screenshots
- **Dark mode:** ![Dark theme](Docs/Images/Dark%20Mode.png)  

- **Light mode:** ![Light theme](Docs/Images/Light%20Mode.png)

---

## Application Setup and Usage
### Option 1: Use the Installer (Recommended)
You can directly install the application using the provided installer:  
[App Installer](App%20Installer/)  

(Go into the `App installer/` folder and run the setup executable *- works for windows only -*)

### Option 2: Build from Source
#### Requirements
- Qt 6.10.0 (Widgets module)  
- CMake 3.16+  
- C++17 compiler  

#### Build
```bash
git clone https://github.com/YousefElbooz/MapRoutingApp.git
cd MapRoutingApp
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

---

## Limitations
This was only made to satisfy the requirements of the project, but it's not a full app for all cases as it has some limitations:
- Currently supports only one map and one query file at a time
- File format is custom and not yet standardized (e.g., GPX/KML not supported)
- Exporting results is limited to text files

However, if you want to try the app you can use the test cases provided in this folder: [TEST CASES](TEST%20CASES)

---

### Feel free to fork the repo and contribute to the project *- If you'd ever think of that -*