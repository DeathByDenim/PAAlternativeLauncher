#include "advanceddialog.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QRadioButton>

AdvancedDialog::AdvancedDialog(const QString& extraparameters, const AdvancedDialog::optimus_t useoptimus, QWidget* parent)
 : QDialog(parent)
{
	setWindowTitle("Advanced");
	QVBoxLayout *mainLayout = new QVBoxLayout(this);

	QLabel *parameterLabel = new QLabel(tr("Extra parameters"), this);
	mainLayout->addWidget(parameterLabel);

	m_parametersLineEdit = new QLineEdit(this);
	m_parametersLineEdit->setText(extraparameters);
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
#endif

	QLabel *availableParameters = new QLabel(this);
	availableParameters->setText(
		"Available parameters\n"
		"  --ubernetdev             : connect to ubernet dev instead of ubernet live\n"
		"  --ubernet-url            : connect to ubernet via explicit url (e.g. localhost)\n"
		"  --devmode                : enable dev only features\n"
		"  --username               : override username\n"
		"  --nofeatures             : turn off features\n"
		"  --nomouselock            : turn off mouse lock\n"
		"  --forwardlogs            : forward engine logs to ui.\n"
		"  --uiurl                  : url for custom ui.\n"
		"  --paurl                  : url for custom content json.\n"
		"  --shaderurl              : url for custom shaders.\n"
		"  --audiologging           : log audio events in the console.\n"
		"  --audioprofiler          : allows fmod to connect to an external profiler.\n"
		"  --audionetmix            : allows fmod to respond to external connections from the design software.\n"
		"  --colormips              : displays color-codes for loaded texture mip levels.\n"
		"  --log-threshold          : set logging level [0 : LOG_Error , LOG_Trace : 4]\n"
		"  --software-ui            : render ui without using gpu\n"
		"  --twitchuser             : override twitch username\n"
		"  --twitchpass             : override twitch password\n"
		"  --no-audio               : disable all audio\n"
		"  --locale                 : override locale code for localized text display, e.g. 'en-us'\n"
		"  --coherent_port          : Coherent debugger port\n"
		"  --localstorageurl        : local storage url\n"
		"  --uioptions              : optional data sent to the ui on start\n"
		"  --nomods                 : disable loading and mounting of mod folders\n"
		"  --steam                  : attempt to initialize the Steam API\n"
		"  --forcefullscreen        : lock display mode to fullscreen\n"
		"  --disable-ui-boot-cache  : Don't cache boot.js - for development\n"
		"  --coherent-log-level     : Set coherent logging level [TRACE|DEBUG|INFO|WARNING|ASSERTFAILURE|ERROR|]\n"
		"  --coherent-allow-minidumps : allow coherent to create minidumps\n"
		"  --coherent-options       : pass options to the Coherent host\n"
		"  --gl-vendor              : Override the GL_VENDOR string\n"
		"  --gl-force-enable-capabilities : Comma separated list of capabilities that are forced ON.\n"
		"  --gl-force-disable-capabilities : Comma separated list of capabilities that are forced OFF.\n"
		"\n"
		"Undocumented:\n"
		"  --hacks\n"
	);
	mainLayout->addWidget(availableParameters);

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
