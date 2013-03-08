/*
 *  GenericUSBXHCI.h
 *  GenericUSBXHCI
 *
 *  Created by Zenith432 on December 5th 2012.
 *  Copyright 2012-2013 Zenith432. All rights reserved.
 *
 */

#ifndef __GENERICUSBXHCI_H__
#define __GENERICUSBXHCI_H__

#define kMaxPorts 15U
#define kMaxStreamsAllowed 256U
#define kMaxActiveInterrupters 1

#include <IOKit/usb/IOUSBControllerV3.h>
#include "XHCIRegs.h"
#include "Private.h"

class IOInterruptEventSource;
class IOFilterInterruptEventSource;

class GenericUSBXHCI : public IOUSBControllerV3
{
	OSDeclareFinalStructors(GenericUSBXHCI);

	friend struct XHCIAsyncEndpoint;

private:
	/*
	 * offsets are for 64-bit, OS 10.8.2
	 */
	IOMemoryMap* _deviceBase;		// offset 0x2A8
	uint16_t _vendorID;				// offset 0x2B0
	uint16_t _deviceID;				// offset 0x2B2
	uint16_t _revisionID;			// offset 0x2B4
									// align 2-byte
	uint32_t _errataBits;			// various bits for chip erratas (0x2B8)

	/*
	 * XHCI Register pointers
	 */
									// align 4-byte
    XHCICapRegisters volatile* _pXHCICapRegisters;
									// Capabilities registers (0x2C0)
    XHCIOpRegisters volatile* _pXHCIOperationalRegisters;
									// offset 0x2C8
	XHCIRuntimeRegisters volatile* _pXHCIRuntimeRegisters;
									// offset 0x2D0
	XHCIXECPStruct volatile* _pXHCIExtendedCapRegisters;
									// offset 0x2D8
	uint32_t volatile* _pXHCIDoorbellRegisters;
									// offset 0x2E0
	uint32_t volatile* _pUSBLegSup;	// Added

	/*
	 * Misc
	 */
	uint32_t _maxPSASize;			// offset 0x2E8
	USBDeviceAddress _hub3Address;	// offset 0x2EC
	USBDeviceAddress _hub2Address;	// offset 0x2EE
	IOFilterInterruptEventSource* _filterInterruptSource;
									// offset 0x2F0
	uint8_t _numSlots;				// offset 0x2F8
	int32_t _baseInterruptIndex;	// Added

	/*
	 * DCBAA
	 */
									// align 6-byte
	struct {
		IOBufferMemoryDescriptor*md;// offset 0x300
		uint64_t* ptr;				// offset 0x308
		uint64_t physAddr;			// offset 0x310
	} _dcbaa;

	/*
	 * Command Ring
	 */
	struct {
		IOBufferMemoryDescriptor*md;// offset 0x320
		TRBStruct* ptr;				// offset 0x328
		uint64_t physAddr;			// offset 0x330
		uint16_t enqueueIndex;		// offset 0x338
		uint16_t dequeueIndex;		// offset 0x33A
		uint16_t numTRBs;			// offset 0x318 - reordered
		uint8_t cycleState;			// offset ox33C
		bool volatile stopPending;	// offset 0x23B18 - reordered
		TRBCallbackEntry* callbacks;// offset 0x340
	} _commandRing;

	/*
	 * Interrupt/EventRing
	 */
	uint16_t _erstMax;				// offset 0x348
	uint16_t _maxInterrupters;		// offset 0x34A
									// offset 0x34C
	EventRingStruct _eventRing[kMaxActiveInterrupters];
									// offset 0x380
									// offset 0x400
	XHCIInterruptRegisterSet _interruptBackup[kMaxActiveInterrupters];
									// offset 0x780
									// offset 0x7C0
	int32_t _debugflags;			// offset 0x980
	int32_t _errorCounters[4];		// offset 0x984

	/*
	 * Device Contexts
	 */
									// align 4-byte
	struct {
		IOBufferMemoryDescriptor*md;// offset 0x998
		ContextStruct* ptr;			// offset 0x9A0, 0x9A8
		uint64_t physAddr;			// offset 0x9B0
		uint16_t refCount;			// offset 0x23AD0 - reordered
	} _inputContext;

	/*
	 * ScratchPad
	 */
	struct {
		uint8_t max;				// offset 0x9B8
									// align 7-byte
		IOBufferMemoryDescriptor*md;// offset 0x9C0
		uint64_t* ptr;				// offset 0x9C8
		uint64_t physAddr;			// offset 0x9D0
		OSArray* mdGC;				// offset 0x9D8
	} _scratchpadBuffers;

	/*
	 * Slots
	 */
#if 0
	SlotStruct _slotArray[256];
									// offset 0x9E0
#else
	SlotStruct* _slotArray;
#endif
	struct {
		uint8_t HubAddress[kUSBMaxDevices];
									// offset 0x231E0
		uint8_t PortOnHub[kUSBMaxDevices];
									// offset 0x23260
		uint8_t Slot[kUSBMaxDevices];
									// offset 0x232E0
		bool Active[kUSBMaxDevices];
									// offset 0x23360
	} _addressMapper;

	/*
	 * Misc
	 */
	struct {
		uint16_t PortOnHub;			// offset 0x233E0
		USBDeviceAddress HubAddress;// offset 0x233E2
		bool isBeingAddressed;		// offset 0x233E4
	} _deviceZero;
									// align 1-byte
	int16_t _numEndpoints;			// offset 0x233E6
	int16_t _maxNumEndpoints;		// offset 0x233E8
	bool _uimInitialized;			// offset 0x233EA
#if 0
	bool _filterInterruptActive;	// offset 0x233EB
#endif
									// align 4-byte
	uint64_t volatile _millsecondCounter;
									// offset 0x233F0
	uint8_t _istKeepAwayFrames;		// offset 0x233F8
									// align 3-byte
	uint32_t _interruptCounters[4];	// offset 0x233FC

	/*
	 * Register Save Area
	 * Note: The following is layed out as XHCIOpRegisters
	 *   so it actually extends to _isSleeping
	 *   (size 0x410U = offset 0x400U + one XHCIPortRegisterSet)
	 */
									// align 4-byte
	XHCIOpRegistersUnpadded _sleepOpSave;
									// offset 0x23410

	bool _isSleeping;				// offset 0x23820
	/*
	 * Root Hub Ports
	 */
	/*
	 * Note: IOUSBControllerV3 assumes _rootHubNumPorts <= kMaxPorts
	 *   which is a flaw - even assuming 15 port maximum
	 *   for USB2 or USB3 protocol each, an xHC can have 30 ports.
	 * The spec allows for up to 255 root-hub ports.
	 * Non-root hubs are limited to 15 due to the routeString.
	 */
	bool _rhPortEmulateCSC[kMaxPorts];
									// offset 0x23821
#if 0
	bool _rhPortInU3[kMaxPorts];
									// offset 0x23830
	bool _rhPortSuspendChange[kMaxPorts];
									// offset 0x2383F
#endif
	bool _rhPortBeingResumed[kMaxPorts];
									// offset 0x2384E
	bool _rhPortBeingReset[kMaxPorts];
									// offset 0x2385D
									// align 4-byte
	thread_call_t _rhResumePortTimerThread[kMaxPorts];
									// offset 0x23870
	uint16_t _rhPortStatusChangeBitmap;	// Added
	uint16_t _rhPortStatusChangeBitmapGated; // Added
#if 0
	thread_call_t _rhResetPortThread[kMaxPorts];
									// offset 0x238E8
	RHPortData _rhPortArea[kMaxPorts];
									// offset 0x23960
	bool _rhPortDebouncing[kMaxPorts];
									// offset 0x239D8
	bool _rhPortDebounceTarget[kMaxPorts];
									// offset 0x239E7
									// align 2-byte
	uint64_t _rhStamps[kMaxPorts];	// offset 0x239F8
	bool _rhPortBeingWarmReset[kMaxPorts];
									// offset 0x23A70
	bool _hasPCIPwrMgmt;			// offset 0x23A7F
#endif
									// offset 0x23A80
	uint32_t _unknown1;				// offset 0x23A88
	uint32_t _magic;				// offset 0x23A8C
#if 0
	uint16_t _rhPrevStatus[1U + kMaxPorts];
									// offset 0x23A90
	uint16_t _rhChangeBits[1U + kMaxPorts];
									// offset 0x23AB0
#endif
	uint16_t _unknown2;				// offset 0x23AD2
	uint8_t _HCCLow;				// replaces _is64bit, _csz in 0x23AD4 - 0x23AD5
									// align 2-byte
	IOACPIPlatformDevice* _providerACPIDevice;
									// offset 0x23AD8
	bool _muxedPortsExist;			// offset 0x23AE0
	bool _muxedPortsSearched;		// offset 0x23AE1
	bool _inTestMode;				// offset 0x23AE2
									// align 5-byte
#if 0
	uint8_t volatile* _unknown3;	// offset 0x23AE8
#endif
	IOSimpleLock* _wdhLock;			// offset 0x23AF0
	uint64_t _millsecondsTimers[4];	// offset 0x23AF8
									// offset 0x23B18 moved
	bool volatile m_invalid_regspace;
									// offset 0x23B19
	bool _HSEDetected;				// offset 0x23B1A
									// align 5-byte
#if 0
	struct {
		IOBufferMemoryDescriptor*md;// offset 0x23B20
		uint64_t physAddr;			// offset 0x23B28
		uint8_t cycleState;			// offset 0x23B30
	} _spareRing;
#endif
	char _muxName[kMaxPorts * 5U];	// offset 0x23B34
									// sizeof 0x23B80

public:
	/*
	 * Virtual from IOUSBControllerV3
	 */
	/*
	 * Pure
	 */
	IOReturn ResetControllerState(void);
	IOReturn RestartControllerFromReset(void);
	IOReturn SaveControllerStateForSleep(void);
	IOReturn RestoreControllerStateFromSleep(void);
	IOReturn DozeController(void);
	IOReturn WakeControllerFromDoze(void);
	IOReturn UIMEnableAddressEndpoints(USBDeviceAddress address, bool enable);
	IOReturn UIMEnableAllEndpoints(bool enable) { return kIOReturnInternalError; }
	IOReturn EnableInterruptsFromController(bool enable);
	/*
	 * Overrides
	 */
	IOReturn GetRootHubBOSDescriptor(OSData* desc);
	IOReturn GetRootHub3Descriptor(IOUSB3HubDescriptor* desc);
	IOReturn UIMDeviceToBeReset(short functionAddress);
	IOReturn UIMAbortStream(UInt32 streamID, short functionNumber, short endpointNumber, short direction);
	UInt32 UIMMaxSupportedStream(void);
	USBDeviceAddress UIMGetActualDeviceAddress(USBDeviceAddress current);
	IOReturn UIMCreateSSBulkEndpoint(UInt8 functionNumber, UInt8 endpointNumber, UInt8 direction, UInt8 speed,
									 UInt16 maxPacketSize, UInt32 maxStream, UInt32 maxBurst);
	IOReturn UIMCreateSSInterruptEndpoint(short functionAddress, short endpointNumber, UInt8 direction, short speed,
										  UInt16 maxPacketSize, short pollingRate, UInt32 maxBurst);
	IOReturn UIMCreateSSIsochEndpoint(short functionAddress, short endpointNumber, UInt32 maxPacketSize, UInt8 direction,
									  UInt8 interval, UInt32 maxBurst);
	IOReturn UIMCreateStreams(UInt8 functionNumber, UInt8 endpointNumber, UInt8 direction, UInt32 maxStream);
	IOReturn GetRootHubPortErrorCount(UInt16 port, UInt16* count);
	IOReturn GetBandwidthAvailableForDevice(IOUSBDevice* forDevice, UInt32* pBandwidthAvailable);

	/*
	 * Virtual from IOUSBControllerV2
	 */
	/*
	 * Pure
	 */
	IOReturn UIMCreateControlEndpoint(UInt8 functionNumber, UInt8 endpointNumber, UInt16 maxPacketSize, UInt8 speed,
									  USBDeviceAddress highSpeedHub, int highSpeedPort);
	IOReturn UIMCreateBulkEndpoint(UInt8 functionNumber, UInt8 endpointNumber, UInt8 direction, UInt8 speed,
								   UInt16 maxPacketSize, USBDeviceAddress highSpeedHub, int highSpeedPort);
	IOReturn UIMCreateInterruptEndpoint(short functionAddress, short endpointNumber,UInt8 direction, short speed,
										UInt16 maxPacketSize, short pollingRate, USBDeviceAddress highSpeedHub,
										int highSpeedPort);
	IOReturn UIMCreateIsochEndpoint(short functionAddress, short endpointNumber, UInt32 maxPacketSize, UInt8 direction,
									USBDeviceAddress highSpeedHub, int highSpeedPort) { return kIOReturnInternalError; }
	/*
	 * Overrides
	 */
	IOReturn ConfigureDeviceZero(UInt8 maxPacketSize, UInt8 speed, USBDeviceAddress hub, int port);
	IOReturn UIMHubMaintenance(USBDeviceAddress highSpeedHub, UInt32 highSpeedPort, UInt32 command, UInt32 flags);
	IOReturn UIMSetTestMode(UInt32 mode, UInt32 port);
	UInt64 GetMicroFrameNumber(void);
	IOReturn UIMCreateIsochEndpoint(short functionAddress, short endpointNumber, UInt32 maxPacketSize, UInt8 direction,
									USBDeviceAddress highSpeedHub, int highSpeedPort, UInt8 interval);
	IOUSBControllerIsochEndpoint* AllocateIsochEP(void);
	IODMACommand* GetNewDMACommand();
	IOReturn GetFrameNumberWithTime(UInt64* frameNumber, AbsoluteTime* theTime);

	/*
	 * Virtual from IOUSBController
	 */
	/*
	 * Pure
	 */
	IOReturn UIMInitialize(IOService* provider);
	IOReturn UIMFinalize();
	IOReturn UIMCreateControlEndpoint(UInt8 functionNumber, UInt8 endpointNumber, UInt16 maxPacketSize,
									  UInt8 speed) { return kIOReturnInternalError; }
	IOReturn UIMCreateControlTransfer(short functionNumber, short endpointNumber, IOUSBCompletion completion,
									  void *CBP, bool bufferRounding, UInt32 bufferSize,
									  short direction) { return kIOReturnInternalError; }
	IOReturn UIMCreateControlTransfer(short functionNumber, short endpointNumber, IOUSBCompletion completion,
									  IOMemoryDescriptor* CBP, bool bufferRounding, UInt32 bufferSize,
									  short direction) { return kIOReturnInternalError; }
	IOReturn UIMCreateBulkEndpoint(UInt8 functionNumber, UInt8 endpointNumber, UInt8 direction,
								   UInt8 speed, UInt8 maxPacketSize) { return kIOReturnInternalError; }
	IOReturn UIMCreateBulkTransfer(short functionNumber, short endpointNumber, IOUSBCompletion completion,
								   IOMemoryDescriptor* CBP, bool bufferRounding, UInt32 bufferSize,
								   short direction) { return kIOReturnInternalError; }
	IOReturn UIMCreateInterruptEndpoint(short functionAddress, short endpointNumber, UInt8 direction,
										short speed, UInt16 maxPacketSize,
										short pollingRate) { return kIOReturnInternalError; }
	IOReturn UIMCreateInterruptTransfer(short functionNumber, short endpointNumber, IOUSBCompletion completion,
										IOMemoryDescriptor* CBP, bool bufferRounding, UInt32 bufferSize,
										short direction) { return kIOReturnInternalError; }
	IOReturn UIMCreateIsochEndpoint(short functionAddress, short endpointNumber, UInt32 maxPacketSize,
									UInt8 direction) { return kIOReturnInternalError; }
	IOReturn UIMCreateIsochTransfer(short functionAddress, short endpointNumber, IOUSBIsocCompletion completion,
									UInt8 direction, UInt64 frameStart, IOMemoryDescriptor* pBuffer,
									UInt32 frameCount, IOUSBIsocFrame* pFrames) { return kIOReturnInternalError; }
	IOReturn UIMAbortEndpoint(short functionNumber, short endpointNumber, short direction);
	IOReturn UIMDeleteEndpoint(short functionNumber, short endpointNumber, short direction);
	IOReturn UIMClearEndpointStall(short functionNumber, short endpointNumber, short direction);
	void UIMRootHubStatusChange(void);
	IOReturn GetRootHubDeviceDescriptor(IOUSBDeviceDescriptor* desc);
	IOReturn GetRootHubDescriptor(IOUSBHubDescriptor* desc);
	IOReturn SetRootHubDescriptor(OSData* buffer) { return kIOReturnInternalError; }
	IOReturn GetRootHubConfDescriptor(OSData* desc);
	IOReturn GetRootHubStatus(IOUSBHubStatus* status);
	IOReturn SetRootHubFeature(UInt16 wValue) { return kIOReturnSuccess; }
	IOReturn ClearRootHubFeature(UInt16 wValue) { return kIOReturnInternalError; }
	IOReturn GetRootHubPortStatus(IOUSBHubPortStatus* status, UInt16 port);
	IOReturn SetRootHubPortFeature(UInt16 wValue, UInt16 port);
	IOReturn ClearRootHubPortFeature(UInt16 wValue, UInt16 port);
	IOReturn GetRootHubPortState(UInt8* state, UInt16 port) { return kIOReturnInternalError; }
	IOReturn SetHubAddress(UInt16 wValue);
	UInt32 GetBandwidthAvailable(void) { return 0U; }
	UInt64 GetFrameNumber(void);
	UInt32 GetFrameNumber32(void);
	void PollInterrupts(IOUSBCompletionAction safeAction);
	void UIMRootHubStatusChange(bool abort) {}
	IOReturn GetRootHubStringDescriptor(UInt8 index, OSData* desc);
	/*
	 * Overrides
	 */
	UInt32 GetErrataBits(UInt16 vendorID, UInt16 deviceID, UInt16 revisionID);
	void UIMCheckForTimeouts(void);
	IOReturn UIMCreateControlTransfer(short functionNumber, short endpointNumber, IOUSBCommand* command,
									  void* CBP, bool bufferRounding, UInt32 bufferSize,
									  short direction) { return kIOReturnInternalError; }
	IOReturn UIMCreateControlTransfer(short functionNumber, short endpointNumber, IOUSBCommand* command,
									  IOMemoryDescriptor* CBP, bool bufferRounding, UInt32 bufferSize,
									  short direction);
	IOReturn UIMCreateBulkTransfer(IOUSBCommand* command);
	IOReturn UIMCreateInterruptTransfer(IOUSBCommand* command);
	IOReturn UIMCreateIsochTransfer(IOUSBIsocCommand *command);

	/*
	 * Virtual from IOService
	 */
	bool willTerminate(IOService* provider, IOOptionBits options);
	bool terminate(IOOptionBits options);
	IOReturn message(UInt32 type, IOService* provider, void* argument);
	IOReturn newUserClient(task_t owningTask, void* securityID, UInt32 type, IOUserClient ** handler);

	/*
	 * Virtual from IORegistryEntry
	 */
	bool init(OSDictionary* dictionary);
	void free(void);

	/*
	 * Self
	 */
	void SetVendorInfo(void);
	/*
	 * Accessors
	 */
	uint8_t Read8Reg(uint8_t volatile const*);
	uint16_t Read16Reg(uint16_t volatile const*);
	uint32_t Read32Reg(uint32_t volatile const*);
	uint64_t Read64Reg(uint64_t volatile const*);
	void Write32Reg(uint32_t volatile*, uint32_t);
	void Write64Reg(uint64_t volatile*, uint64_t, __unused bool);
	static int32_t DiffTRBIndex(uint64_t a, uint64_t b) { return static_cast<int32_t>(a - b) >> 4; }
	static void SetTRBAddr64(TRBStruct*, uint64_t);
	static void SetDCBAAAddr64(uint64_t*, uint64_t);
	static void ClearTRB(TRBStruct*, bool);
	uint8_t GetSlotID(int32_t);
	/*
	 * Diagnostics
	 */
	void PrintCapRegs(PrintSink* = 0);
	void PrintRuntimeRegs(PrintSink* = 0);
	void PrintSlots(PrintSink* = 0);
	void PrintEndpoints(uint8_t, PrintSink* = 0);
	void PrintRootHubPortBandwidth(PrintSink* = 0);
	static void PrintContext(ContextStruct*) {}
	static void PrintEventTRB(TRBStruct*, int32_t, bool, ringStruct*) {}
	/*
	 * Root Hub
	 */
	static void RHResumePortTimerEntry(OSObject*, void*);
	IOReturn RHResetPort(uint8_t, uint16_t);
	void RHResumePortTimer(uint32_t);
	static IOReturn RHResumePortCompletionEntry(OSObject*, void*, void*, void*, void*);
	IOReturn RHResumePortCompletion(uint32_t);
	IOReturn RHCompleteResumeOnAllPorts(void);
	bool RHCheckForPortResume(uint16_t, uint8_t, uint32_t);
	void RHCheckForPortResumes(void);
	IOReturn InitializePorts(void);
	IOReturn AllocRHThreadCalls(void);
	void FinalizeRHThreadCalls(void);
	void RHPortStatusChangeBitmapSet(uint16_t v) { static_cast<void>(__sync_fetch_and_or(&_rhPortStatusChangeBitmap, v)); }
	uint16_t RHPortStatusChangeBitmapGrab(void) { return __sync_lock_test_and_set(&_rhPortStatusChangeBitmap, 0U); }
	void RHPortStatusChangeBitmapInit(void) { _rhPortStatusChangeBitmap = 0U; _rhPortStatusChangeBitmapGated = 0U; }
	/*
	 * Assorted
	 */
	IOReturn MakeBuffer(uint32_t, size_t, uint64_t, IOBufferMemoryDescriptor **, void **, uint64_t*);
	IOReturn MakeBufferUnmapped(uint32_t, size_t, uint64_t, IOBufferMemoryDescriptor **, uint64_t*);
	void CheckSleepCapability(void);
	void SetPropsForBookkeeping(void);
	IOReturn AllocScratchpadBuffers(void);
	void FinalizeScratchpadBuffers(void);
	uint16_t PortNumberCanonicalToProtocol(uint16_t, uint8_t*);
	uint16_t PortNumberProtocolToCanonical(uint16_t, uint8_t);
	IOUSBHubPolicyMaker* GetHubForProtocol(uint8_t protocol);
	bool IsStillConnectedAndEnabled(int32_t);
	void CheckSlotForTimeouts(int32_t, uint32_t);
#if 0
	int32_t FindSlotFromPort(uint16_t);
	void HandlePortDebouncing(uint16_t*, uint16_t*, uint16_t, uint16_t, uint8_t);
#endif
	IOReturn TranslateXHCIStatus(int32_t, bool, uint8_t, bool);
	static IOReturn TranslateCommandCompletion(int32_t);
	static IOReturn GatedGetFrameNumberWithTime(OSObject*, void*, void*, void*, void*);
#if 0
	IOReturn CheckPeriodicBandwidth(int32_t, int32_t, uint16_t, int16_t, int32_t, uint32_t, uint32_t);
#endif
	IOReturn AllocStreamsContextArray(ringStruct*, uint32_t);
	IOReturn configureHub(uint32_t, uint32_t);
	SlotStruct* SlotPtr(uint8_t slot) { return &_slotArray[slot - 1U]; }
	IOReturn ResetDevice(int32_t);
	int32_t NegotiateBandwidth(int32_t);
	int32_t SetLTV(uint32_t);
	IOReturn GetPortBandwidth(uint8_t, uint8_t, uint8_t*, size_t*);
	void NukeSlot(uint8_t);
	void NotifyRootHubsOfPowerLoss(void);
	void SantizePortsAfterPowerLoss(void);
	static void SleepWithGateReleased(IOCommandGate*, uint32_t);
	void CheckedSleep(uint32_t);
	/*
	 * Contexts
	 */
	void GetInputContext(void);
	void ReleaseInputContext(void);
	uint32_t GetInputContextSize(void);
	uint32_t GetDeviceContextSize(void);
	ContextStruct* GetSlotContext(int32_t, int32_t = 0);
	ContextStruct* GetInputContextPtr(int32_t = 0);
	static uint8_t GetSlCtxSpeed(ContextStruct const*);
	static void SetSlCtxSpeed(ContextStruct*, uint32_t);
	/*
	 * Endpoints
	 */
	IOReturn CreateBulkEndpoint(uint8_t, uint8_t, uint8_t, uint8_t, uint16_t, uint16_t, int32_t, uint32_t, uint32_t);
	IOReturn CreateInterruptEndpoint(int16_t, int16_t, uint8_t, int16_t, uint16_t, int16_t, uint16_t, int32_t, uint32_t);
	IOReturn CreateIsochEndpoint(int16_t, int16_t, uint32_t, uint8_t, uint8_t, uint32_t);
	void ClearEndpoint(int32_t, int32_t);
	IOReturn QuiesceAllEndpoints(void);
	IOReturn CreateEndpoint(int32_t, int32_t, uint16_t, int16_t, int32_t, uint32_t, uint32_t, void*);
	IOReturn StartEndpoint(int32_t, int32_t, uint16_t);
	bool checkEPForTimeOuts(int32_t, int32_t, uint32_t, uint32_t);
	uint32_t QuiesceEndpoint(int32_t, int32_t);
	IOReturn StopEndpoint(int32_t, int32_t, bool = false);
	void ResetEndpoint(int32_t, int32_t, bool = false);
	bool IsIsocEP(int32_t, int32_t);
	void DeleteIsochEP(void /* XHCIIsochEndpoint */*);
	static uint8_t TranslateEndpoint(int16_t, int16_t);
	/*
	 * Streams
	 */
	bool IsStreamsEndpoint(int32_t slot, int32_t endpoint) { return SlotPtr(slot)->maxStreamForEndpoint[endpoint] > 1U; }
	uint16_t GetLastStreamForEndpoint(int32_t slot, int32_t endpoint) { return SlotPtr(slot)->lastStreamForEndpoint[endpoint]; }
	void RestartStreams(int32_t, int32_t, uint32_t);
	IOReturn CreateStream(int32_t, int32_t, uint32_t);
	ringStruct* FindStream(int32_t, int32_t, uint64_t, int32_t*, bool);
	void DeleteStreams(int32_t, int32_t);
	/*
	 * Transfers
	 */
	IOReturn CreateTransfer(IOUSBCommand*, uint32_t);
	void ClearStopTDs(int32_t, int32_t);
	static IOReturn AddDummyCommand(ringStruct*, IOUSBCommand*);
	IOReturn _createTransfer(void*, bool, uint32_t, uint32_t, size_t, bool, bool, int32_t*,
							 uint32_t*, bool, int16_t*);
	static TRBStruct* GetNextTRB(ringStruct*, void*, TRBStruct**, bool);
	static void CloseFragment(ringStruct*, TRBStruct*, uint32_t);
	static IOReturn GenerateNextPhysicalSegment(TRBStruct*, uint32_t*, size_t, IODMACommand*);
	static void PutBackTRB(ringStruct*, TRBStruct*);
	/*
	 * Rings
	 */
	ringStruct* CreateRing(int32_t, int32_t, uint32_t);
	ringStruct* GetRing(int32_t, int32_t, uint32_t);
	IOReturn AllocRing(ringStruct*, int32_t);
	static void DeallocRing(ringStruct*);
	static int32_t CountRingToED(ringStruct*, int32_t, uint32_t*, bool);
	void ParkRing(ringStruct*);
	IOReturn ReturnAllTransfersAndReinitRing(int32_t, int32_t, uint32_t);
	IOReturn ReinitTransferRing(int32_t, int32_t, uint32_t);
	int32_t SetTRDQPtr(int32_t, int32_t, uint32_t, int32_t);
	static bool CanTDFragmentFit(ringStruct*, uint32_t);
	/*
	 * Non-standard XHCI Extensions
	 */
	void EnableXHCIPorts(void);
	void EnableComplianceMode(void) {}
	void DisableComplianceMode(void) {}
	bool DiscoverMuxedPorts(void);
	IOReturn HCSelect(uint8_t, uint8_t);
	IOReturn HCSelectWithMethod(char const*);
	bool GetIntelFlag(uint8_t slot) { return SlotPtr(slot)->_intelFlag; }
	void SetIntelFlag(uint8_t slot, bool value) { SlotPtr(slot)->_intelFlag = value; }
	/*
	 * XHCI Normatives
	 */
	IOReturn ResetController(void);
	uint32_t GetPortSCForWriting(int16_t);	// original virtual
	void DecodeExtendedCapability(uint32_t);
	void DecodeSupportedProtocol(XHCIXECPStruct volatile*);
	void TakeOwnershipFromBios(void);
	IOReturn StopUSBBus(void);
	void RestartUSBBus(void);
	IOReturn WaitForUSBSts(uint32_t, uint32_t);
	IOReturn XHCIHandshake(uint32_t volatile*, uint32_t, uint32_t, int);
	IOReturn AddressDevice(uint32_t,uint16_t,bool,uint8_t,int32_t,int32_t);
	IOReturn EnterTestMode(void);
	IOReturn LeaveTestMode(void);
	IOReturn PlacePortInMode(uint32_t, uint32_t);
	/*
	 * Command Ring
	 */
	void InitCMDRing(void);
	IOReturn RestoreCRCr(void);
	IOReturn CommandStop(void);
	IOReturn CommandAbort(void);
	int32_t WaitForCMD(TRBStruct*, int32_t, TRBCallback);
	IOReturn EnqueCMD(TRBStruct*, int32_t, TRBCallback, void*);
	bool DoCMDCompletion(TRBStruct);
	static void _CompleteSlotCommand(GenericUSBXHCI*, TRBStruct*, void*);
	static void CompleteSlotCommand(TRBStruct*, void*);
#if 0
	static void CompleteRenesasVendorCommand(TRBStruct*, void*);
#endif
	/*
	 * Event Handling
	 */
	static void InterruptHandler(OSObject*, __unused IOInterruptEventSource*, __unused int);
	static bool PrimaryInterruptFilter(OSObject*, IOFilterInterruptEventSource*);
	void FilterInterrupt(IOFilterInterruptEventSource*);
	bool preFilterEventRing(IOFilterInterruptEventSource*, int32_t);
	void postFilterEventRing(int32_t);
	bool FilterEventRing(int32_t, bool*);
	bool PollEventRing2(int32_t);
	void PollForCMDCompletions(int32_t);
	void DoStopCompletion(TRBStruct*);
	void processTransferEvent(TRBStruct*);
	bool processTransferEvent2(TRBStruct*, int32_t);
	IOReturn InitAnEventRing(int32_t);
	void InitEventRing(int32_t, bool);
	void FinalizeAnEventRing(int32_t);
	void SaveAnInterrupter(int32_t);
	void RestoreAnInterrupter(int32_t);
	static int findInterruptIndex(IOService*, bool);
	/*
	 * Feature Methods
	 */
	IOReturn XHCIRootHubPowerPort(uint16_t, bool);
	IOReturn XHCIRootHubEnablePort(uint16_t, bool);
	IOReturn XHCIRootHubSetLinkStatePort(uint8_t, uint16_t);
	IOReturn XHCIRootHubWarmResetPort(uint16_t);
	IOReturn XHCIRootHubResetPort(uint8_t, uint16_t);
	IOReturn XHCIRootHubSuspendPort(uint8_t, uint16_t, bool);
	IOReturn XHCIRootHubClearPortConnectionChange(uint16_t);
	IOReturn XHCIRootHubClearPortChangeBit(uint16_t, uint32_t);
};

#endif