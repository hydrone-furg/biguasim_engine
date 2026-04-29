.. _develop-sensor:

==================
Developing Sensors
==================

Similar to agents, sensors are built on both the C++ and Python sides. In this section we will give
an example of developing a sensor and explain how to set it up in both C++ and Python.

The example sensor we will be using is a simple sensor that returns the float ``2.0`` at each tick.
This is a very simple sensor, but it will give you the basic structure of how to set up a sensor.

C++
===
Each sensor will need a '.h' and '.cpp' file, as is standard practice for C++. 

These will both be placed in ``engine/Source/Holodeck/Sensors``, with the ``.h`` in Public and the 
``.cpp`` in Private.

.h file
-------
Start by including the following in your ``.h`` file:

.. code:: c++

    #pragma once
    #include "Holodeck.h"
    #include "HolodeckSensor.h"

Next, set up the class for the sensor:

.. code:: c++

    #include "ExampleSensor.generated.h"

    UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
    class HOLODECK_API UExampleSensor : public UHolodeckSensor {
        GENERATED_BODY()
        public:
            ...
        protected:
            ...
        private:
            ...
    };
.. note::

    * The name of the sensor needs to have the character "U" before it.
    * ``#include "ExampleSensor.generated.h"`` is necessary for Unreal Engine to generate the proper 
      code for the sensor. This is a requirement for all classes that are derived from UObjects.

Now let's go over a few of the main necessary functions to put into the .h file. First, make sure 
your sensor has a constructor and an InitializeSensor() override like this one (note that these 
should be under the ``public`` section of the class):

.. code:: c++

    UExampleSensor();
    virtual void InitializeSensor() override;

Next, in the ``protected`` section, you will need a tick function. This function defines the behavior 
of the sensor every time the simulation ticks.

.. code:: c++

    void TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

Finally, the last item that is essential for a sensor is the pointer to the parent agent in the 
``private`` section. This simply stores a reference to whatever object the sensor is attached to:

.. code:: c++
    
    AActor* Parent;

You may also want to include some helper functions and some class variables. We suggest a function 
that defines your sensor model. 

.cpp file
---------

Now that you are working in the .cpp file, make sure to include the matching header file along with 
``holodeck.h`` and any other libraries you need. Next we will define our necessary functions.

Start with the constructor. It should look something like the following:

.. code:: c++

    UExampleSensor::UExampleSensor() {
        PrimaryComponentTick.bCanEverTick = true;
        SensorName = "ExampleSensor";
    }

Initialize the sensor with any variables that it needs to function. For example, make sure to attach 
the sensor to its parent:

.. code:: c++

    void UExampleSensor::InitializeSensor() {
        Super::InitializeSensor();
        //You need to get the pointer to the object the sensor is attached to. 
        Parent = this->GetAttachmentRootActor();
    }

Next, set up the ``.tick()`` function. This returns your sensor's output, which is sent to the client 
through the shared buffer. This is where you would call your sensor model that implements the sensor. 
For our example, we will have our sensor return the float ``2.0`` at each tick:

.. code:: c++

    void UExampleSensor::TickSensorComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) { 
        float* FloatBuffer = static_cast<float*>(Buffer);
        FloatBuffer[0] = 2.0;
    }

These are all of the necessary functions. Fill in your other functions from the .h file as needed.

Allowing Your Sensor to Be Used In BiguaSim
============================================

At this point the main structure of the sensor is implemented. The following steps make your sensor 
available to the BiguaSim.
    
Additionally, if your sensor is computationally expensive and you expect the ``Hz`` parameter to be set, and affect
the run speed, you will need to add it to the list of _heavy_sensors or _sonar_sensors in the ``SensorDefinition`` class.

Next, in ``engine/Source/Holodeck/ClientCommands/Public/AddSensorCommand.h``, add an include statement 
for your sensor's .h file:

.. code:: c++

    #include "ExampleSensor.h"

Lastly, in the corresponding ``AddSensorCommand.cpp``, add an entry for your sensor in the SensorMap 
dictionary:

.. code:: c++
    
    { "ExampleSensor", UExampleSensor::StaticClass() },



Your sensor should now be available to use BiguaSim!

Semantic Camera Setup
=====================

To get clean and accurate semantic segmentation classes for specific objects in your custom world, you cannot simply drag and drop raw Static Meshes into the level. You need to wrap them in Blueprints using a specific naming convention.

Here is the fastest workflow to set this up:

1. **Import your Mesh:** Import your 3D model (Static Mesh) into the Content Browser.

2. **Create the Blueprint:** Right-click the imported mesh, navigate to **Asset Actions**, and select **Create Blueprint Using This...**. 
   
   .. note::
      
      .. image:: ../../_static/create-blueprint.png
         :align: center
         :alt: Asset Actions Create Blueprint

3. **Name it after your Tag:** Name the newly created Blueprint exactly what you want the semantic tag to be (e.g., ``Pipe``, ``Valve``, or ``Obstacle``).

4. **Rename the Internal Mesh:** Double-click to open the Blueprint you just created. In the Components panel (usually on the top left), find the Static Mesh component and rename it to match your tag (it must be the exact same name as the Blueprint).
   
   .. note::
      
      .. image:: ../../_static/rename.png
         :align: center
         :alt: Renaming the Static Mesh Component

5. **Populate your World:** Compile and save the Blueprint. Finally, drag and drop this Blueprint into your level instead of the raw mesh.

That's it! When the Semantic Camera renders the scene, it will automatically read these names and assign the correct semantic classes to your objects in the Python client.