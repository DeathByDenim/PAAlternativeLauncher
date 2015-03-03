#ifndef MODDATABASEFRAME_H
#define MODDATABASEFRAME_H

#include <QFrame>
#include <QLabel>

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
		QCheckBox *check_box;
		QString mods_json_file_name;
	};
	QList<mod_t> mModList;

	QWidget* loadMods(QString mod_dir, QString header, QWidget* parent);
	void updateModsJson(QString mods_json_file_name);
	static bool priorityCompare(mod_t m1, mod_t m2) { return (m1.priority > m2.priority); }

private slots:
	void getMoreClicked(bool);
	void modCheckBoxStateChanged(int state);
};

#endif // MODDATABASEFRAME_H
