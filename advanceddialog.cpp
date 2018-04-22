#include "advanceddialog.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QRadioButton>
#include <QCheckBox>

AdvancedDialog::AdvancedDialog(const QString& extraparameters, const AdvancedDialog::optimus_t useoptimus, bool usesteamruntime, QWidget* parent)
 : QDialog(parent)
{
	setWindowTitle("Advanced");
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	QLabel *parameterLabel = new QLabel(tr("Extra parameters"), this);
	mainLayout->addWidget(parameterLabel);

	m_parametersLineEdit = new QLineEdit(this);
	m_parametersLineEdit->setText(extraparameters);
	m_parametersLineEdit->setToolTip(
		"Available parameters\n"
		"  --ubernetdev             : connect to ubernet dev instead of ubernet live\n"
		"  --ubernet-url            : connect to ubernet via explicit url (e.g. localhost)\n"
		"  --devmode                : enable dev only features\n"
		"  --server-debug-port      : enable server debug with port\n"
		"  --username               : override username\n"
		"  --nofeatures             : turn off features\n"
		"  --nomouselock            : turn off mouse lock\n"
		"  --forwardlogs            : forward engine logs to ui.\n"
		"  --audiologging           : log audio events in the console.\n"
		"  --audioprofiler          : allows fmod to connect to an external profiler.\n"
		"  --audionetmix            : allows fmod to respond to external connections from the design software.\n"
		"  --colormips              : displays color-codes for loaded texture mip levels.\n"
		"  --log-threshold          : set logging level [1 : LOG_Test , LOG_Trace : 6]\n"
		"  --software-ui            : render ui without using gpu\n"
		"  --no-audio               : disable all audio\n"
		"  --coherent_port          : Coherent debugger port\n"
		"  --localstorageurl        : local storage url\n"
		"  --uioptions              : optional data sent to the ui on start\n"
		"  --nomods                 : disable loading and mounting of mod folders\n"
		"  --steam                  : attempt to initialize the Steam API\n"
		"  --forcefullscreen        : lock display mode to fullscreen\n"
		"  --local-server           : path to local server executable\n"
		"  --matchmaking-url        : connect to matchmaking via explicit url (e.g. localhost)\n"
		"  --no-content             : disable all extra content support\n"
		"  --list-errors            : show a list of the registered fatal errors\n"
		"  --simulate-error         : simulate the fatal error with the given code\n"
		"  --profile-until-main-menu : create an uberprobe up until the main menu starts executing JavaScript.\n"
		"  --content-dev            : Assume development directory structure and use content from Perforce root.\n"
		"  --uiurl                  : url for custom ui.\n"
		"  --shaderurl              : url for custom shaders.\n"
		"  --scripturl              : url for server scripts, for local server development.\n"
		"  --paurl                  : [DEPRECATED] url for custom content json.\n"
		"  --coherent-log-level     : Set coherent logging level [TRACE|DEBUG|INFO|WARNING|ASSERTFAILURE|ERROR|]\n"
		"  --coherent-options       : pass options to the Coherent host\n"
		"  --disable-ui-cache       : never return 304 Not Modified to Coherent\n"
		"  -h   | --help            : Prints the list of command line options\n"
		"  --gl-debug               : set OpenGL context debug flags\n"
		"  --gl-debug-verbose       : log NOTIFICATION level debug output from OpenGL\n"
		"  --gl-vendor              : Override the GL_VENDOR string\n"
		"  --gl-force-enable-capabilities : Comma separated list of capabilities that are forced ON.\n"
		"  --gl-force-disable-capabilities : Comma separated list of capabilities that are forced OFF.\n"
		"  --gl-force-mrt-srgb-capable : Force proper MRT sRGB blending capability.\n"
		"\n"
		"Undocumented:\n"
		"  --hacks"
	);
	mainLayout->addWidget(m_parametersLineEdit);

#ifdef linux
	m_optimusGroupBox = new QGroupBox(tr("NVidia Optimus"), this);
	QVBoxLayout *optimusLayout = new QVBoxLayout(m_optimusGroupBox);
	m_nooptimusRadioButton = new QRadioButton(tr("Don't use Optimus"), m_optimusGroupBox);
	m_optirunRadioButton = new QRadioButton(tr("Use optirun"), m_optimusGroupBox);
	m_primusrunRadioButton = new QRadioButton(tr("Use primusrun"), m_optimusGroupBox);
	optimusLayout->addWidget(m_nooptimusRadioButton);
	optimusLayout->addWidget(m_optirunRadioButton);
	optimusLayout->addWidget(m_primusrunRadioButton);
	switch(useoptimus)
	{
		case nooptimus:
			m_nooptimusRadioButton->setChecked(true);
			break;
		case optirun:
			m_optirunRadioButton->setChecked(true);
			break;
		case primusrun:
			m_primusrunRadioButton->setChecked(true);
			break;
	}
	mainLayout->addWidget(m_optimusGroupBox);

	m_useSteamRuntimeCheckBox = new QCheckBox(tr("Use steam-runtime"), this);
	m_useSteamRuntimeCheckBox->setChecked(usesteamruntime);
	mainLayout->addWidget(m_useSteamRuntimeCheckBox);
#else
	Q_UNUSED(useoptimus);
	Q_UNUSED(usesteamruntime);
#endif

	QDialogButtonBox *buttonBox = new QDialogButtonBox(this);
	buttonBox->addButton(QDialogButtonBox::Ok);
	buttonBox->addButton(QDialogButtonBox::Cancel);
	mainLayout->addWidget(buttonBox);
	connect(buttonBox, SIGNAL(accepted()), SLOT(accept()));
	connect(buttonBox, SIGNAL(rejected()), SLOT(reject()));
}

AdvancedDialog::~AdvancedDialog()
{
}

AdvancedDialog::optimus_t AdvancedDialog::useOptimus()
{
#ifdef linux
	if(m_optirunRadioButton->isChecked())
		return optirun;
	else if(m_primusrunRadioButton->isChecked())
		return primusrun;
#endif

	return nooptimus;
}

#include "advanceddialog.moc"
