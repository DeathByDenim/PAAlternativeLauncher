#include "moddatabaseframe.h"
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QDir>
#include <QDirIterator>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QJsonArray>
#include <QDesktopServices>
#include <QUrl>
#include <QCheckBox>
#include <QScrollArea>
#include <algorithm>

ModDatabaseFrame::ModDatabaseFrame(QWidget* parent)
 : QFrame(parent)
{
	setFrameShape(QFrame::Box);
	setFrameShadow(QFrame::Raised);
	setMinimumWidth(50);
	
	QVBoxLayout *this_layout = new QVBoxLayout(this);

	QScrollArea *scroll_area = new QScrollArea(this);
	scroll_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	QWidget *main_widget = new QWidget(scroll_area);
	QVBoxLayout *main_layout = new QVBoxLayout(main_widget);

	QWidget *client_mods_widget = loadMods("client_mods", tr("Client Mods"), main_widget);
	if(client_mods_widget == NULL)
		client_mods_widget = loadMods("mods", tr("Client Mods"), main_widget);

	if(client_mods_widget)
		main_layout->addWidget(client_mods_widget);

	QWidget *server_mods_widget = loadMods("server_mods", tr("Server Mods"), main_widget);
	if(server_mods_widget)
		main_layout->addWidget(server_mods_widget);
	
	if(client_mods_widget == NULL && server_mods_widget == NULL)
		main_layout->addWidget(new QLabel(tr("No mods founds"), main_widget));

	main_layout->addStretch();

	main_widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	main_widget->adjustSize();
	scroll_area->setWidget(main_widget);
	this_layout->addWidget(scroll_area);

	QPushButton *get_more_push_button = new QPushButton(tr("Get More Mods..."), this);
	connect(get_more_push_button, SIGNAL(clicked(bool)), SLOT(getMoreClicked(bool)));
	this_layout->addWidget(get_more_push_button);

}

ModDatabaseFrame::~ModDatabaseFrame()
{
}

void ModDatabaseFrame::setInstallPath(QString install_path)
{
}

void ModDatabaseFrame::getMoreClicked(bool)
{
	QDesktopServices::openUrl(QUrl("https://forums.uberent.com/threads/rel-pa-mod-manager-cross-platform.59992/"));
}

QWidget * ModDatabaseFrame::loadMods(QString mod_dir, QString header, QWidget* parent)
{
#if defined(linux)
	const QString local_pa_dir(QDir::homePath() + "/.local/Uber Entertainment/Planetary Annihilation");
#elif defined(__APPLE__)
	const QString local_pa_dir(QStandardPaths::displayName(QStandardPaths::GenericDataLocation) + "/Uber Entertainment/Planetary Annihilation");
#elif defined(_WIN32)
	const QString local_pa_dir(QStandardPaths::displayName(QStandardPaths::AppLocalDataLocation) + "\\Uber Entertainment\\Planetary Annihilation");
#else
#	error Not a supported os
#endif

	QFile mods_json_file(local_pa_dir + "/" + mod_dir + "/mods.json");
	if(!mods_json_file.open(QFile::ReadOnly))
		return NULL;

	QJsonArray enabled_mods = QJsonDocument::fromJson(mods_json_file.readAll()).object()["mount_order"].toArray();
	mods_json_file.close();

	QList<mod_t> mod_list;

	QDirIterator it(local_pa_dir + "/" + mod_dir);
	while(it.hasNext())
	{
		QString dir = it.next();
		QFile modinfo_json_file(dir + "/modinfo.json");
		if(modinfo_json_file.open(QFile::ReadOnly))
		{
			QJsonObject mod_info = QJsonDocument::fromJson(modinfo_json_file.readAll()).object();
			modinfo_json_file.close();

			mod_t mod = {
				mod_info["display_name"].toString(),
				mod_info["identifier"].toString(),
				mod_info["priority"].toInt(),
				false,
				NULL,
				local_pa_dir + "/" + mod_dir + "/mods.json"
			};

			for(QJsonArray::const_iterator enabled_mod = enabled_mods.constBegin(); enabled_mod != enabled_mods.constEnd(); ++enabled_mod)
			{
				if(*enabled_mod == mod.identifier)
				{
					mod.enabled = true;
					break;
				}
			}

			mod_list.append(mod);
		}
	}

	if(mod_list.count() > 0)
	{
		std::sort(mod_list.begin(), mod_list.end(), priorityCompare);

		QWidget *main_widget = new QWidget(parent);
		QVBoxLayout *main_layout = new QVBoxLayout(main_widget);

		QLabel *header_label = new QLabel(header, main_widget);
		QFont header_font = header_label->font();
		header_font.setBold(true);
		header_font.setUnderline(true);
		QPalette header_palette = header_label->palette();
		header_palette.setColor(QPalette::WindowText, Qt::white);
		header_label->setFont(header_font);
		header_label->setPalette(header_palette);
		main_layout->addWidget(header_label);

		for(QList<mod_t>::iterator mod = mod_list.begin(); mod != mod_list.end(); ++mod)
		{
			QCheckBox *mod_check_box = new QCheckBox((*mod).name, main_widget);
			mod_check_box->setChecked((*mod).enabled);
			if((*mod).identifier == "com.pa.deathbydenim.dpamm" || (*mod).identifier == "com.pa.raevn.pamm")
				mod_check_box->setEnabled(false);

			QPalette mod_check_box_palette = mod_check_box->palette();
			mod_check_box_palette.setColor(QPalette::WindowText, Qt::white);
			mod_check_box->setPalette(mod_check_box_palette);
			main_layout->addWidget(mod_check_box);
			(*mod).check_box = mod_check_box;

			connect(mod_check_box, SIGNAL(stateChanged(int)), SLOT(modCheckBoxStateChanged(int)));
		}

		mModList = mod_list;

		return main_widget;
	}
	else
		return NULL;
}

void ModDatabaseFrame::modCheckBoxStateChanged(int state)
{
	QCheckBox *mod_check_box = dynamic_cast<QCheckBox *>(sender());
	if(mod_check_box)
	{
		for(QList<mod_t>::iterator mod = mModList.begin(); mod != mModList.end(); ++mod)
		{
			if((*mod).check_box == mod_check_box)
			{
				(*mod).enabled = (state == Qt::Checked);
				updateModsJson((*mod).mods_json_file_name);
				break;
			}
		}

	}
}

void ModDatabaseFrame::updateModsJson(QString mod_json_file_name)
{
	QFile mods_json_file(mod_json_file_name);
	if(mods_json_file.open(QFile::ReadWrite))
	{
		QJsonDocument mods_json_document = QJsonDocument::fromJson(mods_json_file.readAll());
		QJsonArray mount_order = mods_json_document.object()["mount_order"].toArray();
		
		QJsonArray new_mount_order;
		for(QList<mod_t>::const_iterator mod = mModList.constBegin(); mod != mModList.constEnd(); ++mod)
		{
			if((*mod).enabled)
				new_mount_order.append((*mod).identifier);
		}

		QJsonObject obj = mods_json_document.object();
		obj.remove("mount_order");
		obj.insert("mount_order", new_mount_order);
		mods_json_document.setObject(obj);
		qDebug() << mods_json_document;
		mods_json_file.seek(0);
		mods_json_file.resize(0);
		mods_json_file.write(mods_json_document.toJson());
		mods_json_file.close();
	}
}

#include "moddatabaseframe.moc"
