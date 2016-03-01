#ifndef __OpenViBE_AcquisitionServer_CDrivertestEEGAcquisition_H__
#define __OpenViBE_AcquisitionServer_CDrivertestEEGAcquisition_H__

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CDrivertestEEGAcquisition
	 * \author Luca Pasquali (Unibo)
	 * \date Wed Feb 24 14:29:45 2016
	 * \brief The CDrivertestEEGAcquisition allows the acquisition server to acquire data from a Test EEG Acquisition device.
	 *
	 * TODO: details
	 *
	 * \sa CConfigurationtestEEGAcquisition
	 */
	class CDrivertestEEGAcquisition : public OpenViBEAcquisitionServer::IDriver
	{
	public:

		CDrivertestEEGAcquisition(OpenViBEAcquisitionServer::IDriverContext& rDriverContext);
		virtual ~CDrivertestEEGAcquisition(void);
		virtual const char* getName(void);

		virtual OpenViBE::boolean initialize(
			const OpenViBE::uint32 ui32SampleCountPerSentBlock,
			OpenViBEAcquisitionServer::IDriverCallback& rCallback);
		virtual OpenViBE::boolean uninitialize(void);

		virtual OpenViBE::boolean start(void);
		virtual OpenViBE::boolean stop(void);
		virtual OpenViBE::boolean loop(void);

		virtual OpenViBE::boolean isConfigurable(void);
		virtual OpenViBE::boolean configure(void);
		virtual const OpenViBEAcquisitionServer::IHeader* getHeader(void) { return &m_oHeader; }
		
		virtual OpenViBE::boolean isFlagSet(
			const OpenViBEAcquisitionServer::EDriverFlag eFlag) const
		{
			return eFlag==DriverFlag_IsUnstable;
		}

	protected:
		
		SettingsHelper m_oSettings;
		
		OpenViBEAcquisitionServer::IDriverCallback* m_pCallback;

		// Replace this generic Header with any specific header you might have written
		OpenViBEAcquisitionServer::CHeader m_oHeader;

		OpenViBE::uint32 m_ui32SampleCountPerSentBlock;
		OpenViBE::float32* m_pSample;
	
	private:

		/*
		 * Insert here all specific attributes, such as USB port number or device ID.
		 * Example :
		 */
		// OpenViBE::uint32 m_ui32ConnectionID;
		
		OpenViBE::uint32 m_ui32ComPort;
	};
};

#endif // __OpenViBE_AcquisitionServer_CDrivertestEEGAcquisition_H__
