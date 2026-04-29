# BIGUASIM

## About
BiguaSim is an open-source simulation platform built primarily for underwater robotics and training reinforcement learning agents in Unreal Engine 5. It features bindings for Python to communicate seamlessly with agents, sensors, and environments.

Built upon the foundations of the original Holodeck/HoloOcean architecture, BiguaSim has been heavily customized and updated to support modern Unreal Engine features and advanced multi-domain sensor simulations (such as Hybrid GT Sidescan and Singlebeam Sonars).

## Requirements
* **Unreal Engine 5.3**
    * Download via the Epic Games Launcher.
    * Navigate to the Unreal Engine tab -> Library -> Add a new engine version -> Select 5.3 and install.
* **Visual Studio 2022** (Recommended for UE5)
    * Ensure you install the **"Game development with C++"** workload.
    * Check the **"Unreal Engine installer"** optional component.
    * *Reference:* [Setting Up Visual Studio for UE5](https://docs.unrealengine.com/5.0/en-US/setting-up-visual-studio-development-environment-for-cplusplus-projects-in-unreal-engine/)

## Installation
1. Clone the repository:
   `git clone https://bitbucket.org/frostlab/biguasim`
2. Navigate to the engine folder containing the `.uproject` file (`BiguaSim.uproject`).
3. Right-click the `.uproject` file -> **Switch Unreal Engine Version** -> Select your 5.3 installation.
4. Right-click the `.uproject` file -> **Generate Visual Studio project files**.
5. Open the generated `.sln` file in Visual Studio.
6. Build and run the project (if it asks to rebuild missing modules, click Yes).

## Usage
This repository contains everything you need to build and modify environments in Unreal Engine for BiguaSim. If you want to use worlds that have already been built and communicate with them via Python, please check our [Official Python Documentation](INSERIR_O_LINK_DO_GITHUB_PAGES_AQUI).

* BiguaSim can be run either directly from the Unreal Editor or as a Standalone Game.
* **Important:** The simulation will wait and nothing will happen until the Python client connects and begins sending tick commands.
    
## Building For Linux
For a great tutorial on cross-compiling, see this [wiki page](https://github.com/byu-pccl/holodeck-engine/wiki/Cross-Compiling-for-Linux).

## Legacy Architecture Reference (Developer Docs)
If you are here to work on the C++ engine (e.g., adding new sensors or agents) rather than just using the Python client, BiguaSim still relies on the core Shared Memory communication architecture developed by the BYU FRoStLab. You can consult their legacy wikis to understand the underlying C++ framework:
* [Core Architecture Onboarding](https://github.com/BYU-PCCL/holodeck/wiki/Holodeck-Onboarding)
* [Packaging and Using Custom Worlds](https://github.com/BYU-PCCL/holodeck-engine/wiki/Packaging-and-Using-Custom-Worlds)

## Troubleshooting
If you are having compilation problems or weird linking errors with BiguaSim:
1. Open the project in Visual Studio.
2. In the Solution Explorer, right-click the main project (e.g., `BiguaSim`).
3. Click **Clean**.
4. Close Visual Studio.
5. Delete the `Binaries`, `Intermediate`, and `Saved` folders in your project directory.
6. Right-click the `.uproject` file and **Generate Visual Studio project files** again.
7. Open the `.sln` and Build.