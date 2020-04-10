/***********************************************************************
InputDeviceManager - Class to manage physical and virtual input devices,
tools associated to input devices, and the input device update graph.
Copyright (c) 2004-2020 Oliver Kreylos

This file is part of the Virtual Reality User Interface Library (Vrui).

The Virtual Reality User Interface Library is free software; you can
redistribute it and/or modify it under the terms of the GNU General
Public License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

The Virtual Reality User Interface Library is distributed in the hope
that it will be useful, but WITHOUT ANY WARRANTY; without even the
implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Virtual Reality User Interface Library; if not, write to the
Free Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#ifndef VRUI_INPUTDEVICEMANAGER_INCLUDED
#define VRUI_INPUTDEVICEMANAGER_INCLUDED

#include <string>
#include <list>
#include <vector>
#include <Misc/StandardHashFunction.h>
#include <Misc/HashTable.h>
#include <Misc/CallbackData.h>
#include <Misc/CallbackList.h>
#include <Realtime/Time.h>
#include <Vrui/InputDevice.h>

/* Forward declarations: */
namespace Misc {
class ConfigurationFileSection;
}
class GLContextData;
namespace Vrui {
class InputDeviceFeature;
class InputGraphManager;
class TextEventDispatcher;
class InputDeviceAdapter;
}

namespace Vrui {

class InputDeviceManager
	{
	/* Embedded classes: */
	public:
	class CallbackData:public Misc::CallbackData // Base class for input device manager callbacks
		{
		/* Elements: */
		public:
		InputDeviceManager* manager; // Pointer to the input device manager that initiated the callback
		
		/* Constructors and destructors: */
		CallbackData(InputDeviceManager* sManager)
			:manager(sManager)
			{
			}
		};
	
	class InputDeviceCreationCallbackData:public CallbackData // Callback data sent when an input device is created
		{
		/* Elements: */
		public:
		InputDevice* inputDevice; // Pointer to newly created input device
		
		/* Constructors and destructors: */
		InputDeviceCreationCallbackData(InputDeviceManager* sManager,InputDevice* sInputDevice)
			:CallbackData(sManager),
			 inputDevice(sInputDevice)
			{
			}
		};
	
	class InputDeviceDestructionCallbackData:public CallbackData // Callback data sent when an input device is destroyed
		{
		/* Elements: */
		public:
		InputDevice* inputDevice; // Pointer to input device to be destroyed
		
		/* Constructors and destructors: */
		InputDeviceDestructionCallbackData(InputDeviceManager* sManager,InputDevice* sInputDevice)
			:CallbackData(sManager),
			 inputDevice(sInputDevice)
			{
			}
		};
	
	class InputDeviceUpdateCallbackData:public CallbackData // Callback data sent after the manager updated all physical input devices
		{
		/* Constructors and destructors: */
		public:
		InputDeviceUpdateCallbackData(InputDeviceManager* sManager)
			:CallbackData(sManager)
			{
			}
		};
	
	private:
	typedef std::list<InputDevice> InputDevices;
	
	struct HapticFeature // Structure relating an input device to a device adapter's haptic features
		{
		/* Elements: */
		public:
		InputDeviceAdapter* adapter; // Input device adapter managing the haptic feature
		unsigned int hapticFeatureIndex; // Index of the haptic feature in device adapter's namespace
		};
	
	typedef Misc::HashTable<InputDevice*,HapticFeature> HapticFeatureMap; // Hash table mapping input devices to haptic features
	
	/* Elements: */
	private:
	InputGraphManager* inputGraphManager; // Pointer to the input graph manager
	TextEventDispatcher* textEventDispatcher; // Pointer to object dispatching GLMotif text and text control events
	int numInputDeviceAdapters; // Number of input device adapters managed by the input device manager
	InputDeviceAdapter** inputDeviceAdapters; // Array of pointers to managed input device adapters
	InputDevices inputDevices; // List of all created input devices
	Misc::CallbackList inputDeviceCreationCallbacks; // List of callbacks to be called after a new input device has been created
	Misc::CallbackList inputDeviceDestructionCallbacks; // List of callbacks to be called before an input device will be destroyed
	Misc::CallbackList inputDeviceUpdateCallbacks; // List of callbacks to be called immediately after the input device manager updated all physical input devices
	HapticFeatureMap hapticFeatureMap; // Map from input devices to haptic features
	bool predictDeviceStates; // Flag to enable device state prediction for latency mitigation
	Realtime::TimePointMonotonic predictionTime; // Time point to which to predict the state of all input devices during update
	
	/* Constructors and destructors: */
	public:
	InputDeviceManager(InputGraphManager* sInputGraphManager,TextEventDispatcher* sTextEventDispatcher);
	private:
	InputDeviceManager(const InputDeviceManager& source); // Prohibit copy constructor
	InputDeviceManager& operator=(const InputDeviceManager& source); // Prohibit assignment operator
	public:
	~InputDeviceManager(void);
	
	/* Methods: */
	void initialize(const Misc::ConfigurationFileSection& configFileSection); // Creates all input device adapters listed in the configuration file section
	void addAdapter(InputDeviceAdapter* newAdapter); // Adds an input device adapter to the input device manager
	int getNumInputDeviceAdapters(void) const // Returns number of input device adapters
		{
		return numInputDeviceAdapters;
		}
	InputDeviceAdapter* getInputDeviceAdapter(int inputDeviceAdapterIndex) // Returns pointer to an input device adapter
		{
		return inputDeviceAdapters[inputDeviceAdapterIndex];
		}
	InputDeviceAdapter* findInputDeviceAdapter(const InputDevice* device) const; // Returns pointer to the input device adapter owning the given device (or 0)
	InputGraphManager* getInputGraphManager(void) const
		{
		return inputGraphManager;
		}
	TextEventDispatcher* getTextEventDispatcher(void) const
		{
		return textEventDispatcher;
		}
	InputDevice* createInputDevice(const char* deviceName,int trackType,int numButtons,int numValuators,bool physicalDevice =false);
	int getNumInputDevices(void) const
		{
		return inputDevices.size();
		}
	InputDevice* getInputDevice(int deviceIndex);
	InputDevice* findInputDevice(const char* deviceName);
	void destroyInputDevice(InputDevice* device);
	std::string getFeatureName(const InputDeviceFeature& feature) const; // Returns the name of the given input device feature
	int getFeatureIndex(InputDevice* device,const char* featureName) const; // Returns the index of the feature of the given name on the given input device, or -1 if feature does not exist
	void addHapticFeature(InputDevice* device,InputDeviceAdapter* adapter,unsigned int hapticFeatureIndex); // Registers a haptic feature with the given input device
	bool hasHapticFeature(InputDevice* device) const // Returns true if the given input device has a haptic feature
		{
		return hapticFeatureMap.isEntry(device);
		}
	bool isPredictionEnabled(void) const // Returns true if device state prediction is currently enabled
		{
		return predictDeviceStates;
		}
	void disablePrediction(void); // Disables device state prediction
	void prepareMainLoop(void); // Notifies all input device adapters that Vrui main loop is about to start
	void setPredictionTime(const Realtime::TimePointMonotonic& newPredictionTime); // Enables device state prediction and sets the prediction time point for the current frame
	const Realtime::TimePointMonotonic& getPredictionTime(void) const // Returns the current device state prediction time point
		{
		return predictionTime;
		}
	void updateInputDevices(void);
	Misc::CallbackList& getInputDeviceCreationCallbacks(void) // Returns list of input device creation callbacks
		{
		return inputDeviceCreationCallbacks;
		}
	Misc::CallbackList& getInputDeviceDestructionCallbacks(void) // Returns list of input device creation callbacks
		{
		return inputDeviceDestructionCallbacks;
		}
	Misc::CallbackList& getInputDeviceUpdateCallbacks(void) // Returns list of input device update callbacks
		{
		return inputDeviceUpdateCallbacks;
		}
	void glRenderAction(GLContextData& contextData) const; // Renders the input device manager's state
	void hapticTick(InputDevice* device,unsigned int duration,unsigned int frequency,unsigned int amplitude); // Requests a haptic tick of the given duration in milliseconds, frequency in Hertz, and relative amplitude in [0, 256) for the given input device; does nothing if device does not have a haptic feature
	};

}

#endif
