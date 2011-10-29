/*=Plus=header=begin======================================================
Program: Plus
Copyright (c) Laboratory for Percutaneous Surgery. All rights reserved.
See License.txt for details.
=========================================================Plus=header=end*/

#ifndef __vtkTracker_h
#define __vtkTracker_h

#include "PlusConfigure.h"
#include "vtkTrackerBuffer.h"
#include "vtkObject.h"
#include "vtkCriticalSection.h"
#include "vtkXMLDataElement.h"
#include <string>
#include <vector>
#include <map>

class vtkMatrix4x4;
class vtkMultiThreader;
class vtkTrackerTool;
class vtkSocketCommunicator;
class vtkCharArray;
class vtkDataArray;
class vtkDoubleArray;
class vtkHTMLGenerator; 
class vtkGnuplotExecuter;

/*! Flags for tool LEDs (specifically for the POLARIS) */
enum {
  TR_LED_OFF   = 0,
  TR_LED_ON    = 1,
  TR_LED_FLASH = 2
};

/*! Tracker tool types */
enum TRACKER_TOOL_TYPE
{
  TRACKER_TOOL_NONE=0, 
  TRACKER_TOOL_REFERENCE,
  TRACKER_TOOL_PROBE,
  TRACKER_TOOL_STYLUS, 
  TRACKER_TOOL_NEEDLE,
  TRACKER_TOOL_GENERAL
}; 

/*!
\class vtkTracker 
\brief Generic interfaces with real-time 3D tracking systems

The vtkTracker is a generic VTK interface to real-time tracking
systems.  Derived classes should override the Connect(), Disconnect(), 
Probe(), InternalUpdate(), InternalStartTracking(), and InternalStopTracking() methods.
The InternalUpdate() method is called from within a separate
thread, therefore its contents must be thread safe.  Use the
vtkBrachyTracker or vtkNDICertusTracker as a framework for developing subclasses
for new tracking systems.

\ingroup PlusLibTracking
*/
class VTK_EXPORT vtkTracker : public vtkObject
{
public:
  static vtkTracker *New();
  vtkTypeMacro(vtkTracker,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  /*! 
  Probe to see to see if the tracking system is connected to the 
  computer.  This method should be overridden in subclasses. 
  */
  virtual PlusStatus Probe();

  /*! 
  Start the tracking system. The tracking system is brought from
  its ground state (i.e. on but not necessarily initialized) into
  full tracking mode.  This method calls InternalStartTracking()
  after doing a bit of housekeeping.
  */
  virtual PlusStatus StartTracking();

  /*! Stop the tracking system and bring it back to its ground state. This method calls InternalStopTracking(). */
  virtual PlusStatus StopTracking();

  /*! Test whether or not the system is tracking. */
  virtual int IsTracking() { return this->Tracking; };

  /*! Set recording start time for each tool */
  virtual void SetStartTime( double startTime ); 

  /*! Get recording start time */
  virtual double GetStartTime(); 

  /*! Read main configuration from xml data */
  virtual PlusStatus ReadConfiguration(vtkXMLDataElement* config); 

  /*! Write main configuration to xml data */
  virtual PlusStatus WriteConfiguration(vtkXMLDataElement* config); 

  /*! Convert tracker status to string */
  //TODO: the return value should be PLusStatus and the result should be got using parameter by reference
  static std::string ConvertTrackerStatusToString(TrackerStatus status); 

  /*! Get tool type enum from string */
  static PlusStatus ConvertStringToToolType(const char* typeString, TRACKER_TOOL_TYPE &type);

  /*! Get tool type string  from enum */
  static PlusStatus ConvertToolTypeToString(const TRACKER_TOOL_TYPE type, std::string &typeString);

  /*! Get the buffer element values of each tool in a string list by timestamp. */
  virtual PlusStatus GetTrackerToolBufferStringList(double timestamp, 
    std::map<std::string, std::string> &toolsBufferMatrices, 
    std::map<std::string, std::string> &toolsStatuses,
    bool calibratedTransform = false); 

  /*! Get the calibration matrices for all tools in a string */
  virtual PlusStatus GetTrackerToolCalibrationMatrixStringList(std::map<std::string, std::string> &toolsCalibrationMatrices); 

  /*! Add generated html report from tracking data acquisition to the existing html report. htmlReport and plotter arguments has to be defined by the caller function */
  virtual PlusStatus GenerateTrackingDataAcquisitionReport( vtkHTMLGenerator* htmlReport, vtkGnuplotExecuter* plotter, const char* gnuplotScriptsFolder); 

  /*! Get the internal update rate for this tracking system.  This is the number of transformations sent by the tracking system per second per tool. */
  double GetInternalUpdateRate() { return this->InternalUpdateRate; };

  /*! Get the tool object for the specified port.  The first tool is retrieved by GetTool(0). */
  vtkTrackerTool *GetTool(int port);

  /*! 
  Get the number of available tool ports. This is the maxiumum that a
  particular tracking system can support, not the number of tools
  that are actually connected to the system.  In order to determine
  how many tools are connected, you must call Update() and then
  check IsMissing() for each tool between 0 and NumberOfTools-1.
  */
  vtkGetMacro(NumberOfTools, int);

  /*! Get tool port by name */
  int GetToolPortByName(const char* toolName); 

  /*! Get tool ports by type */
  PlusStatus GetToolPortNumbersByType(TRACKER_TOOL_TYPE type, std::vector<int> &toolNumbersVector);

  /*! Get first active tool port number by type */
  int GetFirstPortNumberByType(TRACKER_TOOL_TYPE type);

  /*! Get port number of reference tool */
  int GetReferenceToolNumber();

  /*! Get port number of the first active tool */
  PlusStatus GetFirstActiveTool(int &tool);

  /*! Make the unit emit a string of audible beeps.  This is supported by the POLARIS. */
  void Beep(int n);

  /*! Turn one of the LEDs on the specified tool on or off.  This is supported by the POLARIS. */
  void SetToolLED(int tool, int led, int state);

  /*! 
  The subclass will do all the hardware-specific update stuff
  in this function. It should call ToolUpdate() for each tool.
  Note that vtkTracker.cxx starts up a separate thread after
  InternalStartTracking() is called, and that InternalUpdate() is
  called repeatedly from within that thread.  Therefore, any code
  within InternalUpdate() must be thread safe.  You can temporarily
  pause the thread by locking this->UpdateMutex->Lock() e.g. if you
  need to communicate with the device from outside of InternalUpdate().
  A call to this->UpdateMutex->Unlock() will resume the thread.
  */
  virtual PlusStatus InternalUpdate() { return PLUS_SUCCESS; };

  //BTX
  // These are used by static functions in vtkTracker.cxx, and since
  // VTK doesn't generally use 'friend' functions they are public
  // instead of protected.  Do not use them anywhere except inside
  // vtkTracker.cxx.
  vtkCriticalSection *UpdateMutex;
  vtkCriticalSection *RequestUpdateMutex;
  vtkTimeStamp UpdateTime;
  double InternalUpdateRate;  
  //ETX

  /*! Connects to device. Derived classes should override this */
  virtual PlusStatus Connect();

  /*! Disconnects from device. Derived classes should override this */
  virtual PlusStatus Disconnect();

  /*! Make this tracker into a copy of another tracker. */
  void DeepCopy(vtkTracker *tracker);

  /*! Clear all tool buffers */
  void ClearAllBuffers();

public:
  /*! Set the acquisition frequency */
  vtkSetMacro(Frequency, double);
  /*! Get the acquisition frequency */
  vtkGetMacro(Frequency, double);

  /*! Flag to store tracker calibrated state */
  vtkSetMacro(TrackerCalibrated, bool);
  /*! Flag to store tracker calibrated state */
  vtkGetMacro(TrackerCalibrated, bool);
  /*! Flag to store tracker calibrated state */
  vtkBooleanMacro(TrackerCalibrated, bool); 

  /*! 
  Set the transformation matrix between tracking-system coordinates
  and the desired world coordinate system.  You can use 
  vtkLandmarkTransform to create this matrix from a set of 
  registration points.  Warning: the matrix is copied,
  not referenced.
  */
  vtkSetObjectMacro(WorldCalibrationMatrix, vtkMatrix4x4); 
  /*! Get the transformation matrix between tracking-system coordinates and the desired world coordinate system. */
  vtkGetObjectMacro(WorldCalibrationMatrix, vtkMatrix4x4); 

protected:
  vtkTracker();
  ~vtkTracker();

  /*! 
  This function is called by InternalUpdate() so that the subclasses
  can communicate information back to the vtkTracker base class, which
  will in turn relay the information to the appropriate vtkTrackerTool.
  */
  PlusStatus ToolTimeStampedUpdate(int tool, vtkMatrix4x4 *matrix, TrackerStatus status, unsigned long frameNumber, double unfilteredtimestamp);

  /*! Set the number of tools for the tracker -- this method is only called once within the constructor for derived classes. */
  void SetNumberOfTools(int num);

  /*! Set the tool name */
  void SetToolName(int tool, const char* name);

  /*! Enable tool by tool number */
  void SetToolEnabled(int tool, bool enabled ); 

 /*! InternalStartTracking() initialize the tracking device, this methods should be overridden in derived classes */
  virtual PlusStatus InternalStartTracking() { return PLUS_SUCCESS; };

  /*! InternalStopTracking() free all resources associated with the device, this methods should be overridden in derived classes */
  virtual PlusStatus InternalStopTracking() { return PLUS_SUCCESS; };

  /*! This method should be overridden in derived classes that can make an audible beep. */
  virtual PlusStatus InternalBeep(int n) { return PLUS_SUCCESS; };

  /*! This method should be overridden for devices that have one or more LEDs on the tracked tools. */
  virtual PlusStatus InternalSetToolLED(int tool, int led, int state) { return PLUS_SUCCESS; };

protected:
  /*! Transformation matrix between tracking-system coordinates and the desired world coordinate system */
  vtkMatrix4x4 *WorldCalibrationMatrix;

  /*! Number of tools that this class can handle */
  int NumberOfTools;

  /*! Tracker tools */
  vtkTrackerTool **Tools;

  /*! Flag to strore tracking state of the class */
  int Tracking;

  /*! Last updated timestamp */
  unsigned long LastUpdateTime;

  /*! Thread used for data acquisition */
  vtkMultiThreader *Threader;
  
  /*! Tracking thread id */
  int ThreadId;

  /*! Tracking frequency */
  double Frequency; 

  /*! Flag used for identifying tracker calibration state */
  bool TrackerCalibrated; 

private:
  vtkTracker(const vtkTracker&);
  void operator=(const vtkTracker&);  
};

#endif

