=============================
Sensors API
=============================

BiguaSim features a comprehensive suite of high-fidelity simulated sensors that can be attached to agents. Below is the summary of sensors divided by application domain.

Sensor Overview
===============

.. list-table:: 
   :widths: 25 75
   :header-rows: 1

   * - Sensor
     - Description
   * - **Acoustic / Sonars**
     - 
   * - :cpp:class:`UImagingSonar`
     - Imaging sonar (Forward Looking). Simulates complex acoustic reflections, multipath, and shadowing.
   * - :cpp:class:`UProfilingSonar`
     - Profiling sonar. Inherits the physics of the imaging sonar but forces an extremely narrow elevation beam (1 degree) for cross-sectional slices.
   * - :cpp:class:`USidescanSonar`
     - Sidescan sonar. Returns a 1D array containing port and starboard echoes to generate seabed mosaics.
   * - :cpp:class:`USinglebeamSonar`
     - Echo sounder (Singlebeam Sonar). Emits a single acoustic cone to measure distance/intensity.
   * - **Vision / Cameras**
     - 
   * - :cpp:class:`URGBCamera`
     - Standard optical camera rendering the visible spectrum.
   * - :cpp:class:`UDepthCamera`
     - High-fidelity Z-buffer depth camera. The matrix returns the exact distance where **each pixel represents a ray** projected in a straight line.
   * - :cpp:class:`URGBDCamera`
     - 2-in-1 camera. Simultaneously returns color and depth in the same render pass. Ideal for Visual SLAM.
   * - :cpp:class:`UAnnotationComponent`
     - Semantic Segmentation Camera. Tags and paints scene objects based on customizable IDs.
   * - **Dynamics / Navigation**
     - 
   * - :cpp:class:`UDVLSensor`
     - Doppler Velocity Log. Measures three-dimensional linear velocity using a 4-beam acoustic array (Janus configuration).
   * - :cpp:class:`UIMUSensor`
     - Inertial Measurement Unit. Simulates linear acceleration and angular velocity with Gaussian noise and Random Walk Bias.
   * - :cpp:class:`UGPSSensor`
     - Returns the global coordinate (X,Y,Z). Simulates signal attenuation (returns NaN if submerged).
   * - :cpp:class:`UMagnetometerSensor`
     - 3D Compass. Measures the global magnetic vector in the local reference frame to correct rotation drift (Yaw).
   * - :cpp:class:`UCollisionSensor`
     - Returns a boolean (True/False) indicating whether the agent collided with something.
   * - **Scanners / Range**
     - 
   * - :cpp:class:`URangeFinderSensor`
     - Raycast-based distance sensor. Can act as a 1-beam rangefinder or a planar 2D Lidar (360 degrees).
   * - :cpp:class:`URaycastLidar`
     - High-fidelity rotary 3D Lidar. Simulates atmospheric attenuation and point loss due to low reflectivity (Drop-off).
   * - :cpp:class:`URaycastSemanticLidar`
     - Semantic 3D Lidar. Returns the point cloud (XYZ) and extracts the Semantic ID of each hit object.
   * - **Utilities / Ground Truth**
     - 
   * - :cpp:class:`ULocationSensor`
     - Absolute Ground Truth of the agent's global position (X,Y,Z) with no signal loss.
   * - :cpp:class:`URotationSensor`
     - Ground Truth of the rotation in Euler Angles (Roll, Pitch, Yaw).
   * - :cpp:class:`UOrientationSensor`
     - Ground Truth of the orientation in a 3x3 Rotation Matrix format (Forward, Left, and Up vectors).
   * - :cpp:class:`UPoseSensor`
     - Returns position and orientation packed into a 4x4 Homogeneous Transformation Matrix (16 floats).
   * - :cpp:class:`UDynamicsSensor`
     - Full kinematic sensor (Ground Truth). Array containing Acceleration, Velocity, Position, and Orientation.
   * - :cpp:class:`UVelocitySensor`
     - Ground Truth of the absolute linear velocity (X,Y,Z).

---

Detailed API Reference
======================

Imaging Sonar
-------------
.. doxygenclass:: UImagingSonar
   :members:
   :protected-members:
   :private-members:

Profiling Sonar
---------------
.. doxygenclass:: UProfilingSonar
   :members:
   :protected-members:
   :private-members:

Sidescan Sonar
--------------
.. doxygenclass:: USidescanSonar
   :members:
   :protected-members:
   :private-members:

Singlebeam Sonar
----------------
.. doxygenclass:: USinglebeamSonar
   :members:
   :protected-members:
   :private-members:

RGB Camera
----------
.. doxygenclass:: URGBCamera
   :members:
   :protected-members:
   :private-members:

Depth Camera
------------
.. doxygenclass:: UDepthCamera
   :members:
   :protected-members:
   :private-members:

RGB-D Camera
------------
.. doxygenclass:: URGBDCamera
   :members:
   :protected-members:
   :private-members:

Annotation Component (Semantic Camera)
--------------------------------------
.. doxygenclass:: UAnnotationComponent
   :members:
   :protected-members:
   :private-members:

DVL Sensor
----------
.. doxygenclass:: UDVLSensor
   :members:
   :protected-members:
   :private-members:

IMU Sensor
----------
.. doxygenclass:: UIMUSensor
   :members:
   :protected-members:
   :private-members:

GPS Sensor
----------
.. doxygenclass:: UGPSSensor
   :members:
   :protected-members:
   :private-members:

Magnetometer Sensor
-------------------
.. doxygenclass:: UMagnetometerSensor
   :members:
   :protected-members:
   :private-members:

Collision Sensor
----------------
.. doxygenclass:: UCollisionSensor
   :members:
   :protected-members:
   :private-members:

Range Finder Sensor
-------------------
.. doxygenclass:: URangeFinderSensor
   :members:
   :protected-members:
   :private-members:

Raycast Lidar
-------------
.. doxygenclass:: URaycastLidar
   :members:
   :protected-members:
   :private-members:

Raycast Semantic Lidar
----------------------
.. doxygenclass:: URaycastSemanticLidar
   :members:
   :protected-members:
   :private-members:

Location Sensor
---------------
.. doxygenclass:: ULocationSensor
   :members:
   :protected-members:
   :private-members:

Rotation Sensor
---------------
.. doxygenclass:: URotationSensor
   :members:
   :protected-members:
   :private-members:

Orientation Sensor
------------------
.. doxygenclass:: UOrientationSensor
   :members:
   :protected-members:
   :private-members:

Pose Sensor
-----------
.. doxygenclass:: UPoseSensor
   :members:
   :protected-members:
   :private-members:

Dynamics Sensor
---------------
.. doxygenclass:: UDynamicsSensor
   :members:
   :protected-members:
   :private-members:

Velocity Sensor
---------------
.. doxygenclass:: UVelocitySensor
   :members:
   :protected-members:
   :private-members: