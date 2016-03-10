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

#define WRITE_SIZE 4
#define READ_SIZE 128
#define RAW_DATA_SIZE 24
#define MARKER_1 0x01
#define MARKER_2 0x02
#define MARKER_3 0x03
#define DEBUG_MODE 0

float32* dataBuffer;
uint32 totalSampleCount;
uint32 discardedSampleCount;
HANDLE hSerialMicro;
COMMTIMEOUTS timeoutsMicro={0};

CDrivertestEEGAcquisition::CDrivertestEEGAcquisition(IDriverContext& rDriverContext)
	:IDriver(rDriverContext)
	,m_oSettings("AcquisitionServer_Driver_testEEGAcquisition", m_rDriverContext.getConfigurationManager())
	,m_pCallback(NULL)
	,m_ui32SampleCountPerSentBlock(0)
	,m_pSample(NULL)
	,m_ui32ComPort(1)
	
{
	m_oHeader.setSamplingFrequency(250);
	m_oHeader.setChannelCount(8);
	
	// The following class allows saving and loading driver settings from the acquisition server .conf file
	m_oSettings.add("Header", &m_oHeader);
	// To save your custom driver settings, register each variable to the SettingsHelper
	//m_oSettings.add("SettingName", &variable);
	m_oSettings.add("ComPort", &m_ui32ComPort);
	m_oSettings.load();	
}

CDrivertestEEGAcquisition::~CDrivertestEEGAcquisition(void){
}

const char* CDrivertestEEGAcquisition::getName(void){
	return "Test EEG Acquisition";
}                                                                  //

boolean CDrivertestEEGAcquisition::initialize(const uint32 ui32SampleCountPerSentBlock,	IDriverCallback& rCallback){
	m_rDriverContext.getLogManager() << LogLevel_Trace << "CDriverGenericOscillator::initialize\n";
	
	if(m_rDriverContext.isConnected()) { 
		return false; 
	}
	if(!m_oHeader.isChannelCountSet()||!m_oHeader.isSamplingFrequencySet())	{
		return false;
	}

	for(uint32 i=0;i<m_oHeader.getChannelCount();i++) {
		m_oHeader.setChannelUnits(i, OVTK_UNIT_Volts, OVTK_FACTOR_Base);
	}
	
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
						
	if (hSerialMicro==INVALID_HANDLE_VALUE) {
		if(GetLastError()==ERROR_FILE_NOT_FOUND) {
			m_rDriverContext.getLogManager() << LogLevel_Error << "Serial port does not exist" << "\n";
			return false;
		}
		m_rDriverContext.getLogManager() << LogLevel_Error << "Unknown error occurred while opening serial port " << m_ui32ComPort << "\n";
		return false;
	}
	
	m_rDriverContext.getLogManager() << LogLevel_Info << "Successfully Opened Port at COM: " << m_ui32ComPort << "\n";
		
	DCB dcbSerialParams = {0};
		
	if (!GetCommState(hSerialMicro, &dcbSerialParams)) {
				m_rDriverContext.getLogManager() << LogLevel_Error << "Error getting state\n";
				return false;
	}
	
	dcbSerialParams.BaudRate = 512000;
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

boolean CDrivertestEEGAcquisition::start(void){
	if(!m_rDriverContext.isConnected()) return false;
	if(m_rDriverContext.isStarted()) return false;

	// ...
	// request hardware to start
	// sending data
	// ...
	
	char bufferToWrite[WRITE_SIZE+1] ={1,2,0x10,3};
	DWORD bytesWritten=0;
	
	if(!WriteFile(hSerialMicro, bufferToWrite, WRITE_SIZE, &bytesWritten, NULL)||(bytesWritten!=WRITE_SIZE)){
		m_rDriverContext.getLogManager() << LogLevel_Error << "Error occurred during hardware start\n";
		return false;
	}
	totalSampleCount=0;
	discardedSampleCount=0;	
	
	return true;
}

boolean CDrivertestEEGAcquisition::loop(void){
	if(!m_rDriverContext.isConnected()) return false;
	if(!m_rDriverContext.isStarted()) return true;

	OpenViBE::CStimulationSet l_oStimulationSet;

	// ...
	// receive samples from hardware
	// put them the correct way in the sample array
	// whether the buffer is full, send it to the acquisition server
	//...
	if (DEBUG_MODE) {
		debugIncomingArray();
	} else {
		statesMachine();		
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

boolean CDrivertestEEGAcquisition::stop(void){
	if(!m_rDriverContext.isConnected()) return false;
	if(!m_rDriverContext.isStarted()) return false;

	// ...
	// request the hardware to stop
	// sending data
	// ...
	char bufferToWrite[WRITE_SIZE+1] ={1,2,0x20,3};
	DWORD bytesWritten=0;
	
	if(!WriteFile(hSerialMicro, bufferToWrite, WRITE_SIZE, &bytesWritten, NULL)){
		m_rDriverContext.getLogManager() << LogLevel_Error << "Error occurred during hardware stop\n";
		return false;
	}

	m_rDriverContext.getLogManager() << LogLevel_Info << "Total sample read per channel: " << totalSampleCount << "\n";
	m_rDriverContext.getLogManager() << LogLevel_Info << "Total bytes discarded: " << discardedSampleCount << "\n";
	
	return true;
}

boolean CDrivertestEEGAcquisition::uninitialize(void){
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
                                                     //
boolean CDrivertestEEGAcquisition::isConfigurable(void){
	return true; // change to false if your device is not configurable
}

boolean CDrivertestEEGAcquisition::configure(void){
	// Change this line if you need to specify some references to your driver attribute that need configuration, e.g. the connection ID.
	CConfigurationtestEEGAcquisition m_oConfiguration(m_rDriverContext, OpenViBE::Directories::getDataDir() + "/applications/acquisition-server/interface-testEEGAcquisition.ui", m_ui32ComPort);
	
	if(!m_oConfiguration.configure(m_oHeader))
	{
		return false;
	}
	m_oSettings.save();
	
	return true;
}

boolean CDrivertestEEGAcquisition::saveDataToSampleBuffer (uint32 phaseCounter, unsigned __int8* dataRaw) {
	for (uint32 j=0; j<m_oHeader.getChannelCount(); j++){
		uint32 msb, lsb1, lsb2;
		msb = dataRaw[3*j];
		lsb1 = dataRaw[3*j+1];
		lsb2 = dataRaw[3*j+2];

		int reconstructedValue = (msb << 16) | (lsb1 << 8) | (lsb2);
		if (( msb>>7 ) == 1) {
			reconstructedValue = reconstructedValue | 0xff000000;
		}
		float32 floatValue = reconstructedValue;
		m_pSample[j*m_ui32SampleCountPerSentBlock+phaseCounter] = floatValue/83886070; //10*(2^23-1) non funziona
		//m_rDriverContext.getLogManager() << LogLevel_Info << j*m_ui32SampleCountPerSentBlock+phaseCounter << "\n";		
	}
	return false;
}

boolean CDrivertestEEGAcquisition::statesMachine() {
	
	unsigned __int8 bufferToRead[READ_SIZE+1] ={0};
	uint32 bufferPhaseCount = 0;
	while(bufferPhaseCount<m_ui32SampleCountPerSentBlock) {
	
		getDataStrict(bufferToRead, 1);
		if (bufferToRead[0] != MARKER_1) {
			m_rDriverContext.getLogManager() << LogLevel_Info << "Missed first marker: " << (int) bufferToRead[0] << "\n";
			discardedSampleCount+=1;
			continue;
		}
		
		getDataStrict(bufferToRead, 3);
		if (bufferToRead[2] != MARKER_2) {
			m_rDriverContext.getLogManager() << LogLevel_Info << "Missed second marker: " << (int) bufferToRead[2] << "\n";
			discardedSampleCount+=3;
			continue;
		}
		
		unsigned __int8 header1 = bufferToRead[0], header2 = bufferToRead[1];
		int flagRawData = header1 >> 7;
		int flagFFTData = (header1 >> 6) & 1;
		
		int dataBytesToRead = flagRawData*RAW_DATA_SIZE + 1;
		
		getDataStrict(bufferToRead, dataBytesToRead);
		if (bufferToRead[dataBytesToRead-1] != MARKER_3) {
			m_rDriverContext.getLogManager() << LogLevel_Info << "Missed third marker: " << (int) bufferToRead[dataBytesToRead-1] << "\n";
			discardedSampleCount+=dataBytesToRead;
			continue;
		}
		
		m_rDriverContext.getLogManager() << LogLevel_Info << "Transmission successful!\n";
		saveDataToSampleBuffer(bufferPhaseCount, bufferToRead);
		bufferPhaseCount++;
		totalSampleCount++;
	}
	
	return false;
}

boolean CDrivertestEEGAcquisition::debugIncomingArray() {
	unsigned __int8 bufferToDebug[READ_SIZE] = {0};
	getDataStrict(bufferToDebug, 29);
	if (bufferToDebug[0]==MARKER_1&&bufferToDebug[3]==MARKER_2&&bufferToDebug[28]==MARKER_3)
		m_rDriverContext.getLogManager() << LogLevel_Info << "Acquisition ok!\n";
	else {
		for (int i=0; i<29; i++) {
			m_rDriverContext.getLogManager() << LogLevel_Info << (int) bufferToDebug[i] << " , " << i << "\n";			
		}
	}
	return true;
}

boolean CDrivertestEEGAcquisition::getDataStrict(unsigned __int8* dataArray, int byteNum) {
	DWORD bytesIn;
	int totalBytesRead = 0;
	if(!ReadFile(hSerialMicro, dataArray, byteNum, &bytesIn, NULL)){
		m_rDriverContext.getLogManager() << LogLevel_Error << "Error occurred during read\n";
		return false;
	}
	while ((int) bytesIn != byteNum) {
		int temp = byteNum - bytesIn;
		totalBytesRead += bytesIn;
		if(!ReadFile(hSerialMicro, dataArray + totalBytesRead, byteNum - bytesIn, &bytesIn, NULL)){
			m_rDriverContext.getLogManager() << LogLevel_Error << "Error occurred during read\n";
			return false;
		}
		byteNum = temp;
	}
	return true;
}