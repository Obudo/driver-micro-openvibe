#ifndef __OpenViBE_AcquisitionServer_CConfigurationtestEEGAcquisition_H__
#define __OpenViBE_AcquisitionServer_CConfigurationtestEEGAcquisition_H__

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

#include <gtk/gtk.h>

namespace OpenViBEAcquisitionServer
{
	/**
	 * \class CConfigurationtestEEGAcquisition
	 * \author Luca Pasquali (Unibo)
	 * \date Wed Feb 24 14:29:45 2016
	 * \brief The CConfigurationtestEEGAcquisition handles the configuration dialog specific to the Test EEG Acquisition device.
	 *
	 * TODO: details
	 *
	 * \sa CDrivertestEEGAcquisition
	 */
	class CConfigurationtestEEGAcquisition : public OpenViBEAcquisitionServer::CConfigurationBuilder
	{
	public:

		// you may have to add to your constructor some reference parameters
		// for example, a connection ID:
		//CConfigurationtestEEGAcquisition(OpenViBEAcquisitionServer::IDriverContext& rDriverContext, const char* sGtkBuilderFileName, OpenViBE::uint32& rConnectionId);
		CConfigurationtestEEGAcquisition(OpenViBEAcquisitionServer::IDriverContext& rDriverContext, const char* sGtkBuilderFileName, OpenViBE::uint32& rCOMPort);

		virtual OpenViBE::boolean preConfigure(void);
		virtual OpenViBE::boolean postConfigure(void);
		
		OpenViBE::uint32& m_rCOMPort;

	protected:

		OpenViBEAcquisitionServer::IDriverContext& m_rDriverContext;

	private:

		/*
		 * Insert here all specific attributes, such as a connection ID.
		 * use references to directly modify the corresponding attribute of the driver
		 * Example:
		 */
		// OpenViBE::uint32& m_ui32ConnectionID;
	};
};

#endif // __OpenViBE_AcquisitionServer_CConfigurationtestEEGAcquisition_H__
