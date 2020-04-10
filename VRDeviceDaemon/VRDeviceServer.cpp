/***********************************************************************
VRDeviceServer - Class encapsulating the VR device protocol's server
side.
Copyright (c) 2002-2020 Oliver Kreylos

This file is part of the Vrui VR Device Driver Daemon (VRDeviceDaemon).

The Vrui VR Device Driver Daemon is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

The Vrui VR Device Driver Daemon is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License along
with the Vrui VR Device Driver Daemon; if not, write to the Free
Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
02111-1307 USA
***********************************************************************/

#include <VRDeviceDaemon/VRDeviceServer.h>

#include <stdio.h>
#include <stdexcept>
#include <Misc/SizedTypes.h>
#include <Misc/PrintInteger.h>
#include <Misc/StandardValueCoders.h>
#include <Misc/ConfigurationFile.h>
#include <Vrui/Internal/VRDeviceDescriptor.h>
#include <Vrui/Internal/HMDConfiguration.h>

#define VRDEVICEDAEMON_DEBUG_PROTOCOL 0

namespace {

/**************
Helper classes:
**************/

enum State
	{
	START,CONNECTED,ACTIVE,STREAMING
	};

}

/********************************************
Methods of class VRDeviceServer::ClientState:
********************************************/

VRDeviceServer::ClientState::ClientState(VRDeviceServer* sServer,Comm::ListeningTCPSocket& listenSocket)
	:server(sServer),
	 pipe(listenSocket),
	 state(START),protocolVersion(Vrui::VRDevicePipe::protocolVersionNumber),clientExpectsTimeStamps(true),
	 active(false),streaming(false)
	{
	#ifdef VERBOSE
	/* Assemble the client name: */
	clientName=pipe.getPeerHostName();
	clientName.push_back(':');
	char portId[10];
	clientName.append(Misc::print(pipe.getPeerPortId(),portId+sizeof(portId)-1));
	#endif
	}

/*******************************
Methods of class VRDeviceServer:
*******************************/

bool VRDeviceServer::newConnectionCallback(Threads::EventDispatcher::ListenerKey eventKey,int eventType,void* userData)
	{
	VRDeviceServer* thisPtr=static_cast<VRDeviceServer*>(userData);
	
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf("Creating new client state..."); fflush(stdout);
	#endif
	
	/* Create a new client state object and add it to the list: */
	ClientState* newClient=new ClientState(thisPtr,thisPtr->listenSocket);
	
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf(" done\n");
	#endif
	
	#ifdef VERBOSE
	printf("VRDeviceServer: Connecting new client %s\n",newClient->clientName.c_str());
	fflush(stdout);
	#endif
	
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf("Adding new client state to list\n");
	#endif
	
	thisPtr->clientStates.push_back(newClient);
	
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf("Adding listener for client's socket\n");
	#endif
	
	/* Add an event listener for incoming messages from the client: */
	newClient->listenerKey=thisPtr->dispatcher.addIOEventListener(newClient->pipe.getFd(),Threads::EventDispatcher::Read,thisPtr->clientMessageCallback,newClient);
	
	#if VRDEVICEDAEMON_DEBUG_PROTOCOL
	printf("Client connected\n");
	#endif
	
	return false;
	}

void VRDeviceServer::disconnectClient(VRDeviceServer::ClientState* client,bool removeListener,bool removeFromList)
	{
	if(removeListener)
		{
		/* Stop listening on the client's pipe: */
		dispatcher.removeIOEventListener(client->listenerKey);
		}
	
	/* Check if the client is still streaming or active: */
	if(client->streaming)
		--numStreamingClients;
	if(client->active)
		{
		--numActiveClients;
		
		/* Stop VR devices if there are no more active clients: */
		if(numActiveClients==0)
			deviceManager->stop();
		}
	
	/* Disconnect the client: */
	delete client;
	
	if(removeFromList)
		{
		/* Remove the dead client from the list: */
		for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
			if(*csIt==client)
				{
				/* Remove it and stop searching: */
				*csIt=clientStates.back();
				clientStates.pop_back();
				break;
				}
		}
	}

bool VRDeviceServer::clientMessageCallback(Threads::EventDispatcher::ListenerKey eventKey,int eventType,void* userData)
	{
	ClientState* client=static_cast<ClientState*>(userData);
	VRDeviceServer* thisPtr=client->server;
	
	bool result=false;
	
	try
		{
		/* Read some data from the socket into the socket's read buffer and check if client hung up: */
		if(client->pipe.readSomeData()==0)
			throw std::runtime_error("Client terminated connection");
		
		/* Process messages as long as there is data in the read buffer: */
		while(!result&&client->pipe.canReadImmediately())
			{
			#if VRDEVICEDAEMON_DEBUG_PROTOCOL
			printf("Reading message..."); fflush(stdout);
			#endif
			
			/* Read the next message from the client: */
			Vrui::VRDevicePipe::MessageIdType message=client->pipe.readMessage();
			
			#if VRDEVICEDAEMON_DEBUG_PROTOCOL
			printf(" done, %u\n",(unsigned int)message);
			#endif
			
			/* Run the client state machine: */
			switch(client->state)
				{
				case START:
					if(message==Vrui::VRDevicePipe::CONNECT_REQUEST)
						{
						#if VRDEVICEDAEMON_DEBUG_PROTOCOL
						printf("Reading protocol version..."); fflush(stdout);
						#endif
						
						/* Read client's protocol version number: */
						client->protocolVersion=client->pipe.read<Misc::UInt32>();
						
						#if VRDEVICEDAEMON_DEBUG_PROTOCOL
						printf(" done, %u\n",client->protocolVersion);
						#endif
						
						#if VRDEVICEDAEMON_DEBUG_PROTOCOL
						printf("Sending connect reply..."); fflush(stdout);
						#endif
						
						/* Send connect reply message: */
						client->pipe.writeMessage(Vrui::VRDevicePipe::CONNECT_REPLY);
						if(client->protocolVersion>Vrui::VRDevicePipe::protocolVersionNumber)
							client->protocolVersion=Vrui::VRDevicePipe::protocolVersionNumber;
						client->pipe.write<Misc::UInt32>(client->protocolVersion);
						
						/* Send server layout: */
						thisPtr->state.writeLayout(client->pipe);
						
						/* Check if the client expects virtual device descriptors: */
						if(client->protocolVersion>=2U)
							{
							/* Send the layout of all virtual devices: */
							client->pipe.write<Misc::SInt32>(thisPtr->deviceManager->getNumVirtualDevices());
							for(int deviceIndex=0;deviceIndex<thisPtr->deviceManager->getNumVirtualDevices();++deviceIndex)
								thisPtr->deviceManager->getVirtualDevice(deviceIndex).write(client->pipe,client->protocolVersion);
							}
						
						/* Check if the client expects tracker state time stamps: */
						client->clientExpectsTimeStamps=client->protocolVersion>=3U;
						
						/* Check if the client expects device battery states: */
						if(client->protocolVersion>=5U)
							{
							/* Send all current device battery states: */
							Threads::Mutex::Lock batteryStateLock(thisPtr->batteryStateMutex);
							for(std::vector<Vrui::BatteryState>::const_iterator bsIt=thisPtr->batteryStates.begin();bsIt!=thisPtr->batteryStates.end();++bsIt)
								bsIt->write(client->pipe);
							}
						
						/* Check if the client expects HMD configurations: */
						if(client->protocolVersion>=4U)
							{
							/* Send all current HMD configurations to the new client: */
							Threads::Mutex::Lock hmdConfigurationLock(thisPtr->hmdConfigurationMutex);
							client->pipe.write<Misc::UInt32>(Misc::UInt32(thisPtr->hmdConfigurations.size()));
							for(std::vector<Vrui::HMDConfiguration*>::const_iterator hcIt=thisPtr->hmdConfigurations.begin();hcIt!=thisPtr->hmdConfigurations.end();++hcIt)
								{
								/* Send the full configuration to the client: */
								(*hcIt)->write(0U,0U,0U,client->pipe);
								}
							}
						
						/* Check if the client expects tracker valid flags: */
						client->clientExpectsValidFlags=client->protocolVersion>=5;
						
						/* Check if the client knows about power and haptic features: */
						if(client->protocolVersion>=6U)
							{
							/* Send the number of power and haptic features: */
							client->pipe.write<Misc::UInt32>(thisPtr->deviceManager->getNumPowerFeatures());
							client->pipe.write<Misc::UInt32>(thisPtr->deviceManager->getNumHapticFeatures());
							}
						
						/* Finish the reply message: */
						client->pipe.flush();
						
						#if VRDEVICEDAEMON_DEBUG_PROTOCOL
						printf(" done\n");
						#endif
						
						/* Go to connected state: */
						client->state=CONNECTED;
						}
					else
						throw std::runtime_error("Protocol error in START state");
					break;
				
				case CONNECTED:
					if(message==Vrui::VRDevicePipe::ACTIVATE_REQUEST)
						{
						/* Start VR devices if this is the first active client: */
						if(thisPtr->numActiveClients==0)
							thisPtr->deviceManager->start();
						++thisPtr->numActiveClients;
						
						/* Go to active state: */
						client->active=true;
						client->state=ACTIVE;
						}
					else if(message==Vrui::VRDevicePipe::DISCONNECT_REQUEST)
						{
						/* Cleanly disconnect this client: */
						#ifdef VERBOSE
						printf("VRDeviceServer: Disconnecting client %s\n",client->clientName.c_str());
						fflush(stdout);
						#endif
						thisPtr->disconnectClient(client,false,true);
						result=true;
						}
					else
						throw std::runtime_error("Protocol error in CONNECTED state");
					break;
				
				case ACTIVE:
					if(message==Vrui::VRDevicePipe::PACKET_REQUEST||message==Vrui::VRDevicePipe::STARTSTREAM_REQUEST)
						{
						#if VRDEVICEDAEMON_DEBUG_PROTOCOL
						printf("Sending packet reply..."); fflush(stdout);
						#endif
						
						/* Send a packet reply message: */
						client->pipe.writeMessage(Vrui::VRDevicePipe::PACKET_REPLY);
						
						/* Send the current server state to the client: */
						{
						Threads::Mutex::Lock stateLock(thisPtr->stateMutex);
						thisPtr->state.write(client->pipe,client->clientExpectsTimeStamps,client->clientExpectsValidFlags);
						}
						
						/* Finish the reply message: */
						client->pipe.flush();
						
						#if VRDEVICEDAEMON_DEBUG_PROTOCOL
						printf(" done\n");
						#endif
						
						if(message==Vrui::VRDevicePipe::STARTSTREAM_REQUEST)
							{
							/* Increase the number of streaming clients: */
							++thisPtr->numStreamingClients;
							
							/* Go to streaming state: */
							client->streaming=true;
							client->state=STREAMING;
							}
						}
					else if(message==Vrui::VRDevicePipe::POWEROFF_REQUEST)
						{
						/* Read the index of the power feature to power off: */
						unsigned int powerFeatureIndex=client->pipe.read<Misc::UInt16>();
						
						/* Power off the requested feature: */
						thisPtr->deviceManager->powerOff(powerFeatureIndex);
						}
					else if(message==Vrui::VRDevicePipe::HAPTICTICK_REQUEST)
						{
						/* Read the index of the haptic feature and the duration of the haptic tick: */
						unsigned int hapticFeatureIndex=client->pipe.read<Misc::UInt16>();
						unsigned int duration=client->pipe.read<Misc::UInt16>();
						unsigned int frequency=1U;
						unsigned int amplitude=255U;
						if(client->protocolVersion>=8U)
							{
							/* Read the haptic tick's frequency and amplitude: */
							frequency=client->pipe.read<Misc::UInt16>();
							amplitude=client->pipe.read<Misc::UInt8>();
							}
						
						/* Request a haptic tick on the requested feature: */
						thisPtr->deviceManager->hapticTick(hapticFeatureIndex,duration,frequency,amplitude);
						}
					else if(message==Vrui::VRDevicePipe::DEACTIVATE_REQUEST)
						{
						/* Stop VR devices if this was the last active clients: */
						--thisPtr->numActiveClients;
						if(thisPtr->numActiveClients==0)
							thisPtr->deviceManager->stop();
						
						/* Go to connected state: */
						client->active=false;
						client->state=CONNECTED;
						}
					else
						throw std::runtime_error("Protocol error in ACTIVE state");
					break;
				
				case STREAMING:
					if(message==Vrui::VRDevicePipe::POWEROFF_REQUEST)
						{
						/* Read the index of the power feature to power off: */
						unsigned int powerFeatureIndex=client->pipe.read<Misc::UInt16>();
						
						/* Power off the requested feature: */
						thisPtr->deviceManager->powerOff(powerFeatureIndex);
						}
					else if(message==Vrui::VRDevicePipe::HAPTICTICK_REQUEST)
						{
						/* Read the index of the haptic feature and the duration of the haptic tick: */
						unsigned int hapticFeatureIndex=client->pipe.read<Misc::UInt16>();
						unsigned int duration=client->pipe.read<Misc::UInt16>();
						unsigned int frequency=1U;
						unsigned int amplitude=255U;
						if(client->protocolVersion>=8U)
							{
							/* Read the haptic tick's frequency and amplitude: */
							frequency=client->pipe.read<Misc::UInt16>();
							amplitude=client->pipe.read<Misc::UInt8>();
							}
						
						/* Request a haptic tick on the requested feature: */
						thisPtr->deviceManager->hapticTick(hapticFeatureIndex,duration,frequency,amplitude);
						}
					else if(message==Vrui::VRDevicePipe::STOPSTREAM_REQUEST)
						{
						/* Send stopstream reply message: */
						client->pipe.writeMessage(Vrui::VRDevicePipe::STOPSTREAM_REPLY);
						client->pipe.flush();
						
						/* Decrease the number of streaming clients: */
						--thisPtr->numStreamingClients;
						
						/* Go to active state: */
						client->streaming=false;
						client->state=ACTIVE;
						}
					else if(message!=Vrui::VRDevicePipe::PACKET_REQUEST)
						throw std::runtime_error("Protocol error in STREAMING state");
					break;
				}
			}
		}
	catch(const std::runtime_error& err)
		{
		#ifdef VERBOSE
		printf("VRDeviceServer: Disconnecting client %s due to exception \"%s\"\n",client->clientName.c_str(),err.what());
		fflush(stdout);
		#endif
		thisPtr->disconnectClient(client,false,true);
		result=true;
		}
	
	return result;
	}

void VRDeviceServer::disconnectClientOnError(VRDeviceServer::ClientStateList::iterator csIt,const std::runtime_error& err)
	{
	/* Print error message to stderr: */
	fprintf(stderr,"VRDeviceServer: Disconnecting client %s due to exception %s\n",(*csIt)->clientName.c_str(),err.what());
	fflush(stderr);
	
	/* Disconnect the client: */
	disconnectClient(*csIt,true,false);
	
	/* Remove the dead client from the list: */
	delete *csIt;
	*csIt=clientStates.back();
	clientStates.pop_back();
	}

bool VRDeviceServer::writeStateUpdates(VRDeviceServer::ClientStateList::iterator csIt)
	{
	/* Bail out if the client is not streaming or does not understand incremental state updates: */
	ClientState* client=*csIt;
	if(!client->streaming||client->protocolVersion<7U)
		return true;
	
	/* Send state updates to client: */
	try
		{
		/* Send tracker state updates: */
		for(std::vector<int>::iterator utIt=updatedTrackers.begin();utIt!=updatedTrackers.end();++utIt)
			{
			/* Send tracker update message: */
			client->pipe.writeMessage(Vrui::VRDevicePipe::TRACKER_UPDATE);
			client->pipe.write<Misc::UInt16>(Misc::UInt16(*utIt));
			Misc::Marshaller<Vrui::VRDeviceState::TrackerState>::write(state.getTrackerState(*utIt),client->pipe);
			client->pipe.write<Vrui::VRDeviceState::TimeStamp>(state.getTrackerTimeStamp(*utIt));
			client->pipe.write<Misc::UInt8>(state.getTrackerValid(*utIt)?1U:0U);
			}
		
		/* Send button updates: */
		for(std::vector<int>::iterator ubIt=updatedButtons.begin();ubIt!=updatedButtons.end();++ubIt)
			{
			/* Send button update message: */
			client->pipe.writeMessage(Vrui::VRDevicePipe::BUTTON_UPDATE);
			client->pipe.write<Misc::UInt16>(Misc::UInt16(*ubIt));
			client->pipe.write<Misc::UInt8>(state.getButtonState(*ubIt)?1U:0U);
			}
		
		/* Send valuator updates: */
		for(std::vector<int>::iterator uvIt=updatedValuators.begin();uvIt!=updatedValuators.end();++uvIt)
			{
			/* Send valuator update message: */
			client->pipe.writeMessage(Vrui::VRDevicePipe::VALUATOR_UPDATE);
			client->pipe.write<Misc::UInt16>(Misc::UInt16(*uvIt));
			client->pipe.write<Vrui::VRDeviceState::ValuatorState>(state.getValuatorState(*uvIt));
			}
		
		/* Finish the message set: */
		client->pipe.flush();
		}
	catch(const std::runtime_error& err)
		{
		/* Disconnect the client and signal an error: */
		disconnectClientOnError(csIt,err);
		return false;
		}
	
	return true;
	}

bool VRDeviceServer::writeServerState(VRDeviceServer::ClientStateList::iterator csIt)
	{
	/* Bail out if the client is not streaming or understands incremental state updates: */
	ClientState* client=*csIt;
	if(client->protocolVersion>=7U||!client->streaming)
		return true;
	
	/* Send state to client: */
	try
		{
		/* Send packet reply message: */
		client->pipe.writeMessage(Vrui::VRDevicePipe::PACKET_REPLY);
		
		/* Send server state: */
		state.write(client->pipe,client->clientExpectsTimeStamps,client->clientExpectsValidFlags);
		client->pipe.flush();
		}
	catch(const std::runtime_error& err)
		{
		/* Disconnect the client and signal an error: */
		disconnectClientOnError(csIt,err);
		return false;
		}
	
	return true;
	}

bool VRDeviceServer::writeBatteryState(VRDeviceServer::ClientStateList::iterator csIt,unsigned int deviceIndex)
	{
	/* Bail out if the client is not streaming or can't handle battery states: */
	ClientState* client=*csIt;
	if(!client->streaming||client->protocolVersion<5U)
		return true;
	
	/* Send battery state to client: */
	try
		{
		/* Send battery state update message: */
		client->pipe.writeMessage(Vrui::VRDevicePipe::BATTERYSTATE_UPDATE);
		
		/* Send virtual device index: */
		client->pipe.write<Misc::UInt16>(deviceIndex);
		
		/* Send battery state: */
		batteryStates[deviceIndex].write(client->pipe);
		client->pipe.flush();
		}
	catch(const std::runtime_error& err)
		{
		/* Disconnect the client and signal an error: */
		disconnectClientOnError(csIt,err);
		return false;
		}
	
	return true;
	}

bool VRDeviceServer::writeHmdConfiguration(VRDeviceServer::ClientStateList::iterator csIt,VRDeviceServer::HMDConfigurationVersions& hmdConfigurationVersions)
	{
	/* Bail out if the client is not streaming or can't handle HMD configurations: */
	ClientState* client=*csIt;
	if(!client->streaming||client->protocolVersion<4U)
		return true;
	
	try
		{
		/* Send HMD configuration to client: */
		hmdConfigurationVersions.hmdConfiguration->write(hmdConfigurationVersions.eyePosVersion,hmdConfigurationVersions.eyeVersion,hmdConfigurationVersions.distortionMeshVersion,client->pipe);
		client->pipe.flush();
		}
	catch(const std::runtime_error& err)
		{
		/* Disconnect the client and signal an error: */
		disconnectClientOnError(csIt,err);
		return false;
		}
	
	return true;
	}

VRDeviceServer::VRDeviceServer(VRDeviceManager* sDeviceManager,const Misc::ConfigurationFile& configFile)
	:VRDeviceManager::VRStreamer(sDeviceManager),
	 listenSocket(configFile.retrieveValue<int>("./serverPort",-1),5),
	 numActiveClients(0),numStreamingClients(0),
	 haveUpdates(false),
	 managerTrackerStateVersion(0U),streamingTrackerStateVersion(0U),
	 managerBatteryStateVersion(0U),streamingBatteryStateVersion(0U),batteryStateVersions(0),
	 managerHmdConfigurationVersion(0U),streamingHmdConfigurationVersion(0U),
	 numHmdConfigurations(hmdConfigurations.size()),hmdConfigurationVersions(0)
	{
	/* Add an event listener for incoming connections on the listening socket: */
	dispatcher.addIOEventListener(listenSocket.getFd(),Threads::EventDispatcher::Read,newConnectionCallback,this);
	
	/* Initialize the array of battery state version numbers: */
	batteryStateVersions=new BatteryStateVersions[deviceManager->getNumVirtualDevices()];
	
	/* Initialize the array of HMD configuration version numbers: */
	hmdConfigurationVersions=new HMDConfigurationVersions[numHmdConfigurations];
	for(unsigned int i=0;i<hmdConfigurations.size();++i)
		hmdConfigurationVersions[i].hmdConfiguration=hmdConfigurations[i];
	}

VRDeviceServer::~VRDeviceServer(void)
	{
	/* Stop VR devices if there are still active clients: */
	if(numActiveClients>0)
		deviceManager->stop();
	
	/* Forcefully disconnect all clients: */
	for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
		delete *csIt;
	
	/* Clean up: */
	delete[] batteryStateVersions;
	delete[] hmdConfigurationVersions;
	}

void VRDeviceServer::trackerUpdated(int trackerIndex)
	{
	/* Remember the updated tracker's index and wake up the run loop: */
	haveUpdates=true;
	updatedTrackers.push_back(trackerIndex);
	dispatcher.interrupt();
	}

void VRDeviceServer::buttonUpdated(int buttonIndex)
	{
	/* Remember the updated button's index and wake up the run loop: */
	haveUpdates=true;
	updatedButtons.push_back(buttonIndex);
	dispatcher.interrupt();
	}

void VRDeviceServer::valuatorUpdated(int valuatorIndex)
	{
	/* Remember the updated valuator's index and wake up the run loop: */
	haveUpdates=true;
	updatedValuators.push_back(valuatorIndex);
	dispatcher.interrupt();
	}

void VRDeviceServer::updateCompleted(void)
	{
	/* Update the version number of the device manager's tracking state and wake up the run loop: */
	++managerTrackerStateVersion;
	dispatcher.interrupt();
	}

void VRDeviceServer::batteryStateUpdated(unsigned int deviceIndex)
	{
	/* Update the version number of the device manager's device battery state and wake up the run loop: */
	++batteryStateVersions[deviceIndex].managerVersion;
	++managerBatteryStateVersion;
	dispatcher.interrupt();
	}

void VRDeviceServer::hmdConfigurationUpdated(const Vrui::HMDConfiguration* hmdConfiguration)
	{
	/* Update the version number of the device manager's HMD configuration state and wake up the run loop: */
	++managerHmdConfigurationVersion;
	dispatcher.interrupt();
	}

void VRDeviceServer::run(void)
	{
	#ifdef VERBOSE
	printf("VRDeviceServer: Listening for incoming connections on TCP port %d\n",listenSocket.getPortId());
	fflush(stdout);
	#endif
	
	/* Enable update notifications: */
	deviceManager->setStreamer(this);
	
	/* Run the main loop and dispatch events until stopped: */
	while(dispatcher.dispatchNextEvent())
		{
		/* Check if any streaming update needs to be sent: */
		if(numStreamingClients>0&&(haveUpdates||streamingTrackerStateVersion!=managerTrackerStateVersion))
			{
			Threads::Mutex::Lock stateLock(stateMutex);
			
			/* Check if any incremental device state updates need to be sent: */
			if(haveUpdates)
				{
				/* Send incremental updates to all clients in streaming mode: */
				for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
					if(!writeStateUpdates(csIt))
						--csIt;
				
				/* Reset the update arrays: */
				haveUpdates=false;
				updatedTrackers.clear();
				updatedButtons.clear();
				updatedValuators.clear();
				}
			
			/* Check if a full state update needs to be sent: */
			if(streamingTrackerStateVersion!=managerTrackerStateVersion)
				{
				/* Send a full state update to all clients in streaming mode: */
				for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
					if(!writeServerState(csIt))
						--csIt;
				
				/* Mark streaming state as up-to-date: */
				streamingTrackerStateVersion=managerTrackerStateVersion;
				}
			}
		
		/* Check if any device battery states need to be sent: */
		if(streamingBatteryStateVersion!=managerBatteryStateVersion)
			{
			Threads::Mutex::Lock batteryStateLock(batteryStateMutex);
			
			for(int i=0;i<deviceManager->getNumVirtualDevices();++i)
				{
				if(batteryStateVersions[i].streamingVersion!=batteryStateVersions[i].managerVersion)
					{
					#ifdef VERBOSE
					printf("VRDeviceServer: Sending updated battery state %d to clients\n",i);
					fflush(stdout);
					#endif
					
					/* Send the new HMD configuration to all clients that are streaming and can handle it: */
					for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
						if(!writeBatteryState(csIt,i))
							--csIt;
					
					/* Mark the battery state as up-to-date: */
					batteryStateVersions[i].streamingVersion=batteryStateVersions[i].managerVersion;
					}
				}
			
			/* Mark device battery state as up-to-date: */
			streamingBatteryStateVersion=managerBatteryStateVersion;
			}
		
		/* Check if any HMD configuration updates need to be sent: */
		if(streamingHmdConfigurationVersion!=managerHmdConfigurationVersion)
			{
			Threads::Mutex::Lock hmdConfigurationLock(hmdConfigurationMutex);
			
			for(unsigned int i=0;i<numHmdConfigurations;++i)
				{
				/* Check if this HMD configuration has been updated: */
				Vrui::HMDConfiguration& hc=*hmdConfigurationVersions[i].hmdConfiguration;
				if(hmdConfigurationVersions[i].eyePosVersion!=hc.getEyePosVersion()||hmdConfigurationVersions[i].eyeVersion!=hc.getEyeVersion()||hmdConfigurationVersions[i].distortionMeshVersion!=hc.getDistortionMeshVersion())
					{
					#ifdef VERBOSE
					printf("VRDeviceServer: Sending updated HMD configuration %u to clients\n",i);
					fflush(stdout);
					#endif
					
					/* Send the new HMD configuration to all clients that are streaming and can handle it: */
					for(ClientStateList::iterator csIt=clientStates.begin();csIt!=clientStates.end();++csIt)
						if(!writeHmdConfiguration(csIt,hmdConfigurationVersions[i]))
							--csIt;
					
					/* Mark the HMD configuration as up-to-date: */
					hmdConfigurationVersions[i].eyePosVersion=hc.getEyePosVersion();
					hmdConfigurationVersions[i].eyeVersion=hc.getEyeVersion();
					hmdConfigurationVersions[i].distortionMeshVersion=hc.getDistortionMeshVersion();
					}
				}
			
			/* Mark HMD configuration state as up-to-date: */
			streamingHmdConfigurationVersion=managerHmdConfigurationVersion;
			}
		}
	
	/* Disable update notifications: */
	deviceManager->setStreamer(0);
	}
