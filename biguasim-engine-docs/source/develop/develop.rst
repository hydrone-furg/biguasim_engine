.. _develop:

===================
Developing BiguaSim
===================

.. note::
   
   BiguaSim is developed using Unreal Engine 5.3 . If you are developing worlds, agents, 
   sensors, etc. using Unreal, you will need to download Unreal Engine 5 to develop from the 
   current release and develop branches.

For users that would like to add to or modify BiguaSim, this documentation will help you get started.

The following guides go through how to set up your development environment, including Unreal 
Engine, and how to add new environments, agents, and sensors.

* :doc:`Getting Started <start>`
   * Cloning
   * Opening & Prepping Project
   * Setting up VSCode
   * Compiling
   * Launching Game Live
   * Logging
   * Code Formatting

* :doc:`Developing Environments <env>`
   * Creating a Custom Level
   * Packaging Environments
   * Adding Custom Semantic Labels

* :doc:`Developing Agents <agents>`
   * General Agents

* :doc:`Developing Sensors <sensors>`
   * C++
   * Python
   * Allowing Your Sensor to Be Used In BiguaSim
   * Semantic Camera Setup

* :doc:`Troubleshooting <troubleshooting>`
   * I can’t open BiguaSim in Unreal Engine on Windows
   * None of my changes to code are showing up when I run BiguaSim
   * I added new objects to my level but they don’t show up on sonar

.. toctree::
   :hidden:
   :maxdepth: 2
   
   start
   env
   agents
   sensors
   troubleshooting