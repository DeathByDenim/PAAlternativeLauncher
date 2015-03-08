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

private:
	enum mod_type
	{
		client_mod,
		server_mod
	};
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
		mod_type type;
		QJsonObject scenes;
	};

	QList<mod_t *> mModList;
	bool mIgnoreStateChange;

	QWidget* loadMods(QString mod_dir, mod_type type, QWidget* parent);
	void updateModFiles(QString mod_json_file_name, ModDatabaseFrame::mod_type type);
	static bool priorityCompare(mod_t *m1, mod_t *m2) { return (m1->priority > m2->priority); }
	void enableMod(ModDatabaseFrame::mod_t* mod);
	void disableMod(ModDatabaseFrame::mod_t* mod);

private slots:
	void getMoreClicked(bool);
	void modCheckBoxStateChanged(int state);
};

#endif // MODDATABASEFRAME_H
