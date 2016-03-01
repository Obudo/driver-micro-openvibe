#include "ovasCDrivertestEEGAcquisition.h"
#include "ovasCConfigurationtestEEGAcquisition.h"
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

#include <toolkit/ovtk_all.h>

using namespace OpenViBEAcquisitionServer;
using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace std;

#define WRITE_SIZE 1
#define READ_SIZE 50

float32* dataBuffer;
HANDLE hSerialMicro;
COMMTIMEOUTS timeoutsMicro={0};

//___________________________________________________________________//
//                                                                   //

CDrivertestEEGAcquisition::CDrivertestEEGAcquisition(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_testEEGAcquisition", m_rDriverContext.getConfigurationManager())
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(0)
	,m_pSample(NULL)
	,m_ui32ComPort(1)
{
	m_oHeader.setSamplingFrequency(128);
	m_oHeader.setChannelCount(32);
	
	// The following class allows saving and loading driver settings from the acquisition server .conf file
	m_oSettings.add("Header", &m_oHeader);
	// To save your custom driver settings, register each variable to the SettingsHelper
	//m_oSettings.add("SettingName", &variable);
	m_oSettings.add("ComPort", &m_ui32ComPort);
	m_oSettings.load();	
}

CDrivertestEEGAcquisition::~CDrivertestEEGAcquisition(void)
{
}

const char* CDrivertestEEGAcquisition::getName(void)
{
	return "Test EEG Acquisition";
}

//___________________________________________________________________//
//                                                                   //

boolean CDrivertestEEGAcquisition::initialize(
	const uint32 ui32SampleCountPerSentBlock,
	IDriverCallback& rCallback)
{
	if(m_rDriverContext.isConnected()) return false;
	if(!m_oHeader.isChannelCountSet()||!m_oHeader.isSamplingFrequencySet()) return false;
	
	// Builds up a buffer to store
	// acquired samples. This buffer
	// will be sent to the acquisition
	// server later...
	m_pSample=new float32[m_oHeader.getChannelCount()*ui32SampleCountPerSentBlock];
	if(!m_pSample)
	{
		delete [] m_pSample;
		m_pSample=NULL;
		return false;
	}
	
	// ...
	// initialize hardware and get
	// available header information
	// from it
	// Using for example the connection ID provided by the configuration (m_ui32ConnectionID)
	// ...
	
	char com[10];
	sprintf(com, "COM%d", m_ui32ComPort);

	m_rDriverContext.getLogManager() << LogLevel_Info << "Attempting to Connect to Device at COM Port: " << com << "\n";
	
	hSerialMicro = CreateFile(com,
							GENERIC_READ | GENERIC_WRITE,
							0,
							0,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							0);

	m_rDriverContext.getLogManager() << LogLevel_Info << "Successfully Opened Port at COM: " << m_ui32ComPort << "\n";
						
	if (hSerialMicro==INVALID_HANDLE_VALUE) {
		if(GetLastError()==ERROR_FILE_NOT_FOUND) {
			m_rDriverContext.getLogManager() << LogLevel_Error << "Serial port does not exist" << "\n";
			return false;
		}
		m_rDriverContext.getLogManager() << LogLevel_Error << "Unknown error occurred while opening serial port " << m_ui32ComPort << "\n";
		return false;
	}
	
	DCB dcbSerialParams = {0};
		
	if (!GetCommState(hSerialMicro, &dcbSerialParams)) {
				m_rDriverContext.getLogManager() << LogLevel_Error << "Error getting state\n";
				return false;
	}
	
	dcbSerialParams.BaudRate = CBR_19200;
	dcbSerialParams.ByteSize = 8;
	dcbSerialParams.StopBits = ONESTOPBIT;
	dcbSerialParams.Parity = NOPARITY;
	
	if (!SetCommState(hSerialMicro, &dcbSerialParams)) {
		m_rDriverContext.getLogManager() << LogLevel_Error << "Error setting serial port state\n";
		return false;
	}
	
	//SET TIMEOUTS
		
	timeoutsMicro.ReadIntervalTimeout=50; //Intervallo tra un char e il successivo, prima di un return
	timeoutsMicro.ReadTotalTimeoutConstant=50; //Intervallo totale prima di un return
	timeoutsMicro.ReadTotalTimeoutMultiplier=10; //Intervallo tra un byte e il successivo
	timeoutsMicro.WriteTotalTimeoutConstant=50;
	timeoutsMicro.WriteTotalTimeoutMultiplier=10;
	
	if(!SetCommTimeouts(hSerialMicro, &timeoutsMicro)){
		m_rDriverContext.getLogManager() << LogLevel_Error << "Error setting timeouts\n";
		return false;
	}
	
	// Saves parameters
	m_pCallback=&rCallback;
	m_ui32SampleCountPerSentBlock=ui32SampleCountPerSentBlock;
	return true;
}

boolean CDrivertestEEGAcquisition::start(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(m_rDriverContext.isStarted()) return false;

	// ...
	// request hardware to start
	// sending data
	// ...
	
	//TODO: write start bit
	char bufferToWrite[WRITE_SIZE+1] ={0};
	DWORD bytesWritten=0;
	
	if(!WriteFile(hSerialMicro, bufferToWrite, WRITE_SIZE, &bytesWritten, NULL)){
		m_rDriverContext.getLogManager() << LogLevel_Error << "Error occurred during hardware start\n";
		return false;
	}

	return true;
}

boolean CDrivertestEEGAcquisition::loop(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(!m_rDriverContext.isStarted()) return true;

	OpenViBE::CStimulationSet l_oStimulationSet;

	// ...
	// receive samples from hardware
	// put them the correct way in the sample array
	// whether the buffer is full, send it to the acquisition server
	//...
	char bufferToRead[READ_SIZE+1] ={0};
	DWORD dwBytesRead=0;
	
	if(!ReadFile(hSerialMicro, bufferToRead, READ_SIZE, &dwBytesRead, NULL)){
		m_rDriverContext.getLogManager() << LogLevel_Error << "Error occurred during loop read\n";
	}

	m_pCallback->setSamples(m_pSample);
	
	// When your sample buffer is fully loaded, 
	// it is advised to ask the acquisition server 
	// to correct any drift in the acquisition automatically.
	m_rDriverContext.correctDriftSampleCount(m_rDriverContext.getSuggestedDriftCorrectionSampleCount());

	// ...
	// receive events from hardware
	// and put them the correct way in a CStimulationSet object
	//...
	m_pCallback->setStimulationSet(l_oStimulationSet);

	return true;
}

boolean CDrivertestEEGAcquisition::stop(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(!m_rDriverContext.isStarted()) return false;

	// ...
	// request the hardware to stop
	// sending data
	// ...
	char bufferToWrite[WRITE_SIZE+1] ={0};
	DWORD bytesWritten=0;
	
	if(!WriteFile(hSerialMicro, bufferToWrite, WRITE_SIZE, &bytesWritten, NULL)){
		m_rDriverContext.getLogManager() << LogLevel_Error << "Error occurred during hardware stop\n";
		return false;
	}

	return true;
}

boolean CDrivertestEEGAcquisition::uninitialize(void)
{
	if(!m_rDriverContext.isConnected()) return false;
	if(m_rDriverContext.isStarted()) return false;

	// ...
	// uninitialize hardware here
	// ...
	CloseHandle(hSerialMicro);
	
	delete [] m_pSample;
	m_pSample=NULL;
	m_pCallback=NULL;

	return true;
}

//___________________________________________________________________//
//                                                                   //
boolean CDrivertestEEGAcquisition::isConfigurable(void)
{
	return true; // change to false if your device is not configurable
}

boolean CDrivertestEEGAcquisition::configure(void)
{
	// Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
	CConfigurationtestEEGAcquisition m_oConfiguration(m_rDriverContext, OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-testEEGAcquisition.ui", m_ui32ComPort);
	
	if(!m_oConfiguration.configure(m_oHeader))
	{
		return false;
	}
	m_oSettings.save();
	
	return true;
}