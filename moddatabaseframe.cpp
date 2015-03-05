#include "moddatabaseframe.h"
#include "information.h"
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
 : QFrame(parent), mIgnoreStateChange(false)
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

	QList<mod_t *> mod_list;

	QDirIterator it(local_pa_dir + "/" + mod_dir);
	while(it.hasNext())
	{
		QString dir = it.next();
		QFile modinfo_json_file(dir + "/modinfo.json");
		if(modinfo_json_file.open(QFile::ReadOnly))
		{
			QJsonObject mod_info = QJsonDocument::fromJson(modinfo_json_file.readAll()).object();
			modinfo_json_file.close();

			mod_t *mod = new mod_t;
			mod->name = mod_info["display_name"].toString();
			mod->identifier = mod_info["identifier"].toString();
			mod->priority = mod_info["priority"].toInt();
			mod->enabled = false;
			mod->processed = false;
			mod->check_box = NULL;
			mod->mods_json_file_name = local_pa_dir + "/" + mod_dir + "/mods.json";

			QJsonArray dep_array = mod_info["dependencies"].toArray();
			for(QJsonArray::const_iterator dep = dep_array.constBegin(); dep != dep_array.constEnd(); ++dep)
				mod->dependencies.push_back((*dep).toString());

			if(mod_info.keys().contains("scenes"))
			{
				mod->scenes = mod_info["scenes"].toObject();
			}
			else
			{
				QStringList keys = mod_info.keys();
				
				if(keys.contains("armory"))
					mod->scenes["armory"] = mod_info["armory"];
				if(keys.contains("building_planets"))
					mod->scenes["building_planets"] = mod_info["building_planets"];
				if(keys.contains("game_over"))
					mod->scenes["game_over"] = mod_info["game_over"];
				if(keys.contains("global"))
					mod->scenes["global"] = mod_info["global"];
				if(keys.contains("guide"))
					mod->scenes["guide"] = mod_info["guide"];
				if(keys.contains("gw_game_over"))
					mod->scenes["gw_game_over"] = mod_info["gw_game_over"];
				if(keys.contains("gw_lobby"))
					mod->scenes["gw_lobby"] = mod_info["gw_lobby"];
				if(keys.contains("gw_play"))
					mod->scenes["gw_play"] = mod_info["gw_play"];
				if(keys.contains("gw_start"))
					mod->scenes["gw_start"] = mod_info["gw_start"];
				if(keys.contains("gw_war_over"))
					mod->scenes["gw_war_over"] = mod_info["gw_war_over"];
				if(keys.contains("icon_atlas"))
					mod->scenes["icon_atlas"] = mod_info["icon_atlas"];
				if(keys.contains("leaderboard"))
					mod->scenes["leaderboard"] = mod_info["leaderboard"];
				if(keys.contains("live_game"))
					mod->scenes["live_game"] = mod_info["live_game"];
				if(keys.contains("live_game_action_bar"))
					mod->scenes["live_game_action_bar"] = mod_info["live_game_action_bar"];
				if(keys.contains("live_game_build_bar"))
					mod->scenes["live_game_build_bar"] = mod_info["live_game_build_bar"];
				if(keys.contains("live_game_build_hover"))
					mod->scenes["live_game_build_hover"] = mod_info["live_game_build_hover"];
				if(keys.contains("live_game_celestial_control"))
					mod->scenes["live_game_celestial_control"] = mod_info["live_game_celestial_control"];
				if(keys.contains("live_game_chat"))
					mod->scenes["live_game_chat"] = mod_info["live_game_chat"];
				if(keys.contains("live_game_control_group_bar"))
					mod->scenes["live_game_control_group_bar"] = mod_info["live_game_control_group_bar"];
				if(keys.contains("live_game_devmode"))
					mod->scenes["live_game_devmode"] = mod_info["live_game_devmode"];
				if(keys.contains("live_game_econ"))
					mod->scenes["live_game_econ"] = mod_info["live_game_econ"];
				if(keys.contains("live_game_footer"))
					mod->scenes["live_game_footer"] = mod_info["live_game_footer"];
				if(keys.contains("live_game_header"))
					mod->scenes["live_game_header"] = mod_info["live_game_header"];
				if(keys.contains("live_game_hover"))
					mod->scenes["live_game_hover"] = mod_info["live_game_hover"];
				if(keys.contains("live_game_menu"))
					mod->scenes["live_game_menu"] = mod_info["live_game_menu"];
				if(keys.contains("live_game_message"))
					mod->scenes["live_game_message"] = mod_info["live_game_message"];
				if(keys.contains("live_game_options_bar"))
					mod->scenes["live_game_options_bar"] = mod_info["live_game_options_bar"];
				if(keys.contains("live_game_paused_popup"))
					mod->scenes["live_game_paused_popup"] = mod_info["live_game_paused_popup"];
				if(keys.contains("live_game_pip"))
					mod->scenes["live_game_pip"] = mod_info["live_game_pip"];
				if(keys.contains("live_game_planets"))
					mod->scenes["live_game_planets"] = mod_info["live_game_planets"];
				if(keys.contains("live_game_players"))
					mod->scenes["live_game_players"] = mod_info["live_game_players"];
				if(keys.contains("live_game_popup"))
					mod->scenes["live_game_popup"] = mod_info["live_game_popup"];
				if(keys.contains("live_game_sandbox"))
					mod->scenes["live_game_sandbox"] = mod_info["live_game_sandbox"];
				if(keys.contains("live_game_selection"))
					mod->scenes["live_game_selection"] = mod_info["live_game_selection"];
				if(keys.contains("live_game_time_bar"))
					mod->scenes["live_game_time_bar"] = mod_info["live_game_time_bar"];
				if(keys.contains("live_game_time_bar_alerts"))
					mod->scenes["live_game_time_bar_alerts"] = mod_info["live_game_time_bar_alerts"];
				if(keys.contains("live_game_unit_alert"))
					mod->scenes["live_game_unit_alert"] = mod_info["live_game_unit_alert"];
				if(keys.contains("load_planet"))
					mod->scenes["load_planet"] = mod_info["load_planet"];
				if(keys.contains("main"))
					mod->scenes["main"] = mod_info["main"];
				if(keys.contains("matchmaking"))
					mod->scenes["matchmaking"] = mod_info["matchmaking"];
				if(keys.contains("new_game"))
					mod->scenes["new_game"] = mod_info["new_game"];
				if(keys.contains("new_game_cinematic"))
					mod->scenes["new_game_cinematic"] = mod_info["new_game_cinematic"];
				if(keys.contains("new_game_ladder"))
					mod->scenes["new_game_ladder"] = mod_info["new_game_ladder"];
				if(keys.contains("replay_browser"))
					mod->scenes["replay_browser"] = mod_info["replay_browser"];
				if(keys.contains("replay_loading"))
					mod->scenes["replay_loading"] = mod_info["replay_loading"];
				if(keys.contains("server_browser"))
					mod->scenes["server_browser"] = mod_info["server_browser"];
				if(keys.contains("settings"))
					mod->scenes["settings"] = mod_info["settings"];
				if(keys.contains("social"))
					mod->scenes["social"] = mod_info["social"];
				if(keys.contains("special_icon_atlas"))
					mod->scenes["special_icon_atlas"] = mod_info["special_icon_atlas"];
				if(keys.contains("start"))
					mod->scenes["start"] = mod_info["start"];
				if(keys.contains("system_editor"))
					mod->scenes["system_editor"] = mod_info["system_editor"];
				if(keys.contains("transit"))
					mod->scenes["transit"] = mod_info["transit"];
				if(keys.contains("uberbar"))
					mod->scenes["uberbar"] = mod_info["uberbar"];
			}

			for(QJsonArray::const_iterator enabled_mod = enabled_mods.constBegin(); enabled_mod != enabled_mods.constEnd(); ++enabled_mod)
			{
				if(*enabled_mod == mod->identifier)
				{
					mod->enabled = true;
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

		for(QList<mod_t *>::iterator mod = mod_list.begin(); mod != mod_list.end(); ++mod)
		{
			QCheckBox *mod_check_box = new QCheckBox((*mod)->name, main_widget);
			mod_check_box->setChecked((*mod)->enabled);
			if((*mod)->identifier == "com.pa.deathbydenim.dpamm" || (*mod)->identifier == "com.pa.raevn.pamm")
				mod_check_box->setEnabled(false);

			QPalette mod_check_box_palette = mod_check_box->palette();
			mod_check_box_palette.setColor(QPalette::WindowText, Qt::white);
			mod_check_box->setPalette(mod_check_box_palette);
			main_layout->addWidget(mod_check_box);
			(*mod)->check_box = mod_check_box;

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
	if(mIgnoreStateChange)
		return;

	QCheckBox *mod_check_box = dynamic_cast<QCheckBox *>(sender());
	if(mod_check_box)
	{
		for(QList<mod_t *>::iterator mod = mModList.begin(); mod != mModList.end(); ++mod)
		{
			if((*mod)->check_box == mod_check_box)
			{
				for(QList<mod_t *>::iterator mod2 = mModList.begin(); mod2 != mModList.end(); ++mod2)
					(*mod2)->processed = false;

				mIgnoreStateChange = true;

				if(state == Qt::Checked)
					enableMod(*mod);
				else
					disableMod(*mod);

				updateModFiles((*mod)->mods_json_file_name);
				mIgnoreStateChange = false;
				break;
			}
		}

	}
}

void ModDatabaseFrame::updateModFiles(QString mod_json_file_name)
{
	QFile mods_json_file(mod_json_file_name);
	if(mods_json_file.open(QFile::ReadWrite))
	{
		QJsonDocument mods_json_document = QJsonDocument::fromJson(mods_json_file.readAll());
		QJsonArray mount_order = mods_json_document.object()["mount_order"].toArray();
		
		QJsonArray new_mount_order;
		for(QList<mod_t *>::const_iterator mod = mModList.constBegin(); mod != mModList.constEnd(); ++mod)
		{
			if((*mod)->enabled)
				new_mount_order.append((*mod)->identifier);
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
	else
		info.critical(tr("Mod file update"), tr("Failed to write to \"%1\".").arg(mod_json_file_name));

	QFileInfo file_info(mod_json_file_name);
	QFile ui_mod_list_file(file_info.absolutePath() + "/PAMM/ui/mods/ui_mod_list.js");
	if(ui_mod_list_file.open(QFile::WriteOnly))
	{
		QMap<QString,QStringList> scene_map;
		for(QList<mod_t *>::const_iterator mod = mModList.constBegin(); mod != mModList.constEnd(); ++mod)
		{
			if(!(*mod)->enabled)
				continue;

			for(QJsonObject::const_iterator scene = (*mod)->scenes.constBegin(); scene != (*mod)->scenes.constEnd(); ++scene)
			{
				QJsonArray scene_items = scene.value().toArray();
				for(QJsonArray::const_iterator item = scene_items.constBegin(); item != scene_items.constEnd(); ++item)
					scene_map[scene.key()] << (*item).toString();
			}
		}

		QTextStream out(&ui_mod_list_file);

		out << "var global_mod_list = [\"" << scene_map["global_mod_list"].join("\",\"") << "\"];\n";

		out << "var scene_mod_list = {";

		for(QMap<QString,QStringList>::const_iterator scene = scene_map.constBegin(); scene != scene_map.constEnd(); ++scene)
		{
			if(scene.key() == "global_mod_list")
				continue;

			if(scene != scene_map.constBegin())
				out << ',';

			out << "\"" << scene.key() << "\": [\"" << scene.value().join("\",\"") << "\"]";
		}
		out << "};";

	}
	else
		info.critical(tr("Mod file update"), tr("Failed to write to \"%1\".").arg(ui_mod_list_file.fileName()));
}

void ModDatabaseFrame::disableMod(ModDatabaseFrame::mod_t * mod)
{
	if(mod->processed)
		return;

	mod->processed = true;
	mod->enabled = false;
	mod->check_box->setChecked(false);
	for(QList<mod_t *>::iterator mod2 = mModList.begin(); mod2 != mModList.end(); ++mod2)
	{
		for(QStringList::const_iterator dep = (*mod2)->dependencies.constBegin(); dep != (*mod2)->dependencies.constEnd(); ++dep)
		{
			if(*dep == mod->identifier)
			{
				disableMod(*mod2);
			}
		}
	}
}

void ModDatabaseFrame::enableMod(ModDatabaseFrame::mod_t * mod)
{
	if(mod->processed)
		return;

	mod->processed = true;
	mod->enabled = true;
	mod->check_box->setChecked(true);
	for(QStringList::const_iterator dep = mod->dependencies.constBegin(); dep != mod->dependencies.constEnd(); ++dep)
	{
		for(QList<mod_t *>::iterator mod2 = mModList.begin(); mod2 != mModList.end(); ++mod2)
		{
			if((*mod2)->identifier == *dep)
			{
				enableMod(*mod2);
			}
		}
	}
}


#include "moddatabaseframe.moc"
