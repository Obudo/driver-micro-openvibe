#include "ovasCConfigurationtestEEGAcquisition.h"

using namespace OpenViBE;
using namespace OpenViBE::Kernel;
using namespace OpenViBEAcquisitionServer;
using namespace std;

/*_________________________________________________

Insert callback to specific widget here
Example with a button that launch a calibration of the device:

//Callback connected to a dedicated gtk button:
static void button_calibrate_pressed_cb(::GtkButton* pButton, void* pUserData)
{
	CConfigurationtestEEGAcquisition* l_pConfig=static_cast<CConfigurationtestEEGAcquisition*>(pUserData);
	l_pConfig->buttonCalibratePressedCB();
}

//Callback actually called:
void CConfigurationGTecGUSBamp::buttonCalibratePressedCB(void)
{
	// Connect to the hardware, ask for calibration, verify the return code, etc.
}
_________________________________________________*/

// If you added more reference attribute, initialize them here
CConfigurationtestEEGAcquisition::CConfigurationtestEEGAcquisition(IDriverContext& rDriverContext, const char* sGtkBuilderFileName, uint32& rCOMPort)
	:CConfigurationBuilder(sGtkBuilderFileName)
	,m_rDriverContext(rDriverContext)
	,m_rCOMPort(rCOMPort)
{
}

boolean CConfigurationtestEEGAcquisition::preConfigure(void)
{
	if(! CConfigurationBuilder::preConfigure())
	{
		return false;
	}
	
	::GtkSpinButton* l_pSpinButtonCOMPort=GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_COM_port"));
	
	gtk_spin_button_set_value(l_pSpinButtonCOMPort, m_rCOMPort);

	// Connect here all callbacks
	// Example:
	// g_signal_connect(gtk_builder_get_object(m_pBuilderConfigureInterface, "button_calibrate"), "pressed", G_CALLBACK(button_calibrate_pressed_cb), this);

	// Insert here the pre-configure code.
	// For example, you may want to check if a device is currently connected
	// and if more than one are connected. Then you can list in a dedicated combo-box 
	// the device currently connected so the user can choose which one he wants to acquire from.

	return true;
}

boolean CConfigurationtestEEGAcquisition::postConfigure(void)
{
	if(m_bApplyConfiguration)
	{
		::GtkSpinButton* l_pSpinButtonCOMPort=GTK_SPIN_BUTTON(gtk_builder_get_object(m_pBuilderConfigureInterface, "spinbutton_COM_port"));
		
		m_rCOMPort = (uint32)::gtk_spin_button_get_value(l_pSpinButtonCOMPort);
		
		// If the user pressed the "apply" button, you need to save the changes made in the configuration.
		// For example, you can save the connection ID of the selected device:
		// m_ui32ConnectionID = <value-from-gtk-widget>
	}

	if(! CConfigurationBuilder::postConfigure()) // normal header is filled (Subject ID, Age, Gender, channels, sampling frequency), ressources are realesed
	{
		return false;
	}

	return true;
}
