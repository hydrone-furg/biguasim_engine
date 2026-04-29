==================
Core Framework API
==================

This section covers the core simulation and communication logic of the BiguaSim engine. These classes manage the Unreal Engine game loop and the shared memory communication with the Python client.

Game Mode & State
-----------------

The main GameMode handles the initialization of the world, spawns agents, and registers the server for communication. The GameInstance persists throughout the simulation.

.. doxygenclass:: AHolodeckGameMode
   :members:
   :protected-members:
   :undoc-members:

.. doxygenclass:: UHolodeckGameInstance
   :members:
   :protected-members:
   :undoc-members:

Server & Communication
----------------------

The server classes are responsible for packing and unpacking data via shared memory.

.. doxygenclass:: UHolodeckServer
   :members:
   :protected-members:
   :undoc-members:

.. doxygenclass:: HolodeckSharedMemory
   :members:
   :protected-members:
   :undoc-members: