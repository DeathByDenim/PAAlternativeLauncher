#ifndef MODDATABASEFRAME_H
#define MODDATABASEFRAME_H

#include <QFrame>
#include <QJsonObject>

class QCheckBox;
class ModDatabaseFrame : public QFrame
{
	Q_OBJECT

public:
	ModDatabaseFrame(QWidget * parent = 0);
	~ModDatabaseFrame();

	void setInstallPath(QString install_path);

private:
	struct mod_t
	{
		QString name;
		QString identifier;
		int priority;
		bool enabled;
		bool processed;
		QStringList dependencies;
		QCheckBox *check_box;
		QString mods_json_file_name;
		QJsonObject scenes;
	};
	QList<mod_t *> mModList;
	bool mIgnoreStateChange;

	QWidget* loadMods(QString mod_dir, QString header, QWidget* parent);
	void updateModFiles(QString mods_json_file_name);
	static bool priorityCompare(mod_t *m1, mod_t *m2) { return (m1->priority > m2->priority); }
	void enableMod(ModDatabaseFrame::mod_t* mod);
	void disableMod(ModDatabaseFrame::mod_t* mod);

private slots:
	void getMoreClicked(bool);
	void modCheckBoxStateChanged(int state);
};

#endif // MODDATABASEFRAME_H
