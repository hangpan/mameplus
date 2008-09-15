#include "mamepguimain.h"

MameGame *mamegame = NULL, *mamegame0 = NULL;

QString currentGame, currentFolder;
Gamelist *gamelist = NULL;
QStringList consoleGamesL;
TreeModel *gameListModel;
GameListSortFilterProxyModel *gameListPModel;

QTimer *searchTimer = NULL;
GamelistDelegate gamelistDelegate(0);
QSet<QString> visibleGames;

enum
{
	FOLDER_ALLGAME = 0,
	FOLDER_ALLARC,
	FOLDER_AVAILABLE,
	FOLDER_UNAVAILABLE,
	FOLDER_CONSOLE,
	FOLDER_MANUFACTURER,
	FOLDER_YEAR,
	FOLDER_SOURCE,
	FOLDER_BIOS,
	/*
	FOLDER_CPU,
	FOLDER_SND,
	FOLDER_ORIENTATION,
	FOLDER_DEFICIENCY,
	FOLDER_DUMPING,
	FOLDER_WORKING,
	FOLDER_NONWORKING,
	FOLDER_ORIGINAL,
	FOLDER_CLONES,
	FOLDER_RASTER,
	FOLDER_VECTOR,
	FOLDER_RESOLUTION,
	FOLDER_FPS,
	FOLDER_SAVESTATE,
	FOLDER_CONTROL,
	FOLDER_STEREO,
	FOLDER_HARDDISK,
	FOLDER_SAMPLES,
	FOLDER_ARTWORK,*/
	MAX_FOLDERS
};

enum
{
	DOCK_SNAP,
	DOCK_FLYER,
	DOCK_CABINET,
	DOCK_MARQUEE,
	DOCK_TITLE,
	DOCK_CPANEL,
	DOCK_PCB,
	
	DOCK_HISTORY,
	DOCK_MAMEINFO,
	DOCK_STORY,
	DOCK_LAST
};

enum
{
	COL_DESC = 0,
	COL_NAME,
	COL_ROM,
	COL_MFTR,
	COL_SRC,
	COL_YEAR,
	COL_CLONEOF,
	COL_LAST
};

/* to support mameplus .mmo translation */
enum {
	UI_LANG_EN_US = 0,
	UI_LANG_ZH_CN,
	UI_LANG_ZH_TW,
	UI_LANG_FR_FR,
	UI_LANG_DE_DE,
	UI_LANG_IT_IT,
	UI_LANG_JA_JP,
	UI_LANG_KO_KR,
	UI_LANG_ES_ES,
	UI_LANG_CA_ES,
	UI_LANG_VA_ES,
	UI_LANG_PL_PL,
	UI_LANG_PT_PT,
	UI_LANG_PT_BR,
	UI_LANG_HU_HU,
	UI_LANG_MAX
};

enum {
	UI_MSG_MAME = 0,
	UI_MSG_LIST,
	UI_MSG_READINGS,
	UI_MSG_MANUFACTURE,
	UI_MSG_OSD0,
	UI_MSG_OSD1,
	UI_MSG_OSD2,
	UI_MSG_OSD3,
	UI_MSG_OSD4,
	UI_MSG_OSD5,
	UI_MSG_MAX = 31
};

class ListXMLHandler : public QXmlDefaultHandler
{
public:
	ListXMLHandler(int d = 0)
	{
		gameinfo = 0;
		metMameTag = false;

// fixme: cannot delete it when new mame version
//		if (mamegame)
//			delete mamegame;
		mamegame = new MameGame(win);
	}

	bool startElement(const QString & /* namespaceURI */,
		const QString & /* localName */,
		const QString &qName,
		const QXmlAttributes &attributes)
	{
		if (!metMameTag && qName != "mame")
			return false;

		if (qName == "mame")
		{
			metMameTag = true;
			mamegame->mameVersion = attributes.value("build");
		}
		else if (qName == "game")
		{
			//update progress
			static int i;
			gamelist->updateProgress(i++);
			qApp->processEvents();
			
			gameinfo = new GameInfo(mamegame);
			currentDevice = "";
			gameinfo->sourcefile = attributes.value("sourcefile");
			gameinfo->isBios = attributes.value("isbios") == "yes";
			gameinfo->isExtRom = false;
			gameinfo->cloneof = attributes.value("cloneof");
			gameinfo->romof = attributes.value("romof");

			mamegame->gamenameGameInfoMap[attributes.value("name")] = gameinfo;
		}
		else if (qName == "rom")
		{
			RomInfo *rominfo = new RomInfo(gameinfo);
			rominfo->name = attributes.value("name");
			rominfo->bios = attributes.value("bios");
			rominfo->size = attributes.value("size").toULongLong();
			rominfo->status = attributes.value("status");

			bool ok;
			gameinfo->crcRomInfoMap[attributes.value("crc").toUInt(&ok, 16)] = rominfo;
		}
		else if (gameinfo->isBios && qName == "biosset")
		{
			BiosInfo *biosinfo = new BiosInfo(gameinfo);
			biosinfo->description = attributes.value("description");
			biosinfo->isdefault = attributes.value("default") == "yes";

			gameinfo->nameBiosInfoMap[attributes.value("name")] = biosinfo;
		}
		else if (qName == "driver")
		{
			gameinfo->status = utils->getStatus(attributes.value("status"));
			gameinfo->emulation = utils->getStatus(attributes.value("emulation"));
			gameinfo->color = utils->getStatus(attributes.value("color"));
			gameinfo->sound = utils->getStatus(attributes.value("sound"));
			gameinfo->graphic = utils->getStatus(attributes.value("graphic"));
			gameinfo->cocktail = utils->getStatus(attributes.value("cocktail"));
			gameinfo->protection = utils->getStatus(attributes.value("protection"));
			gameinfo->savestate = utils->getStatus(attributes.value("savestate"));
			gameinfo->palettesize = attributes.value("palettesize").toUInt();
		}
		else if (qName == "device")
		{
			DeviceInfo *deviceinfo = new DeviceInfo(gameinfo);
			deviceinfo->type = attributes.value("type");
			currentDevice = deviceinfo->type;
			deviceinfo->mandatory = attributes.value("mandatory") == "1";

			gameinfo->nameDeviceInfoMap[currentDevice] = deviceinfo;
		}
		else if (!currentDevice.isEmpty() && qName == "extension")
		{
			DeviceInfo *deviceinfo = gameinfo->nameDeviceInfoMap[currentDevice];
			deviceinfo->extension << attributes.value("name");
		}

		currentText.clear();
		return true;
	}

	bool endElement(const QString & /* namespaceURI */,
		const QString & /* localName */,
		const QString &qName)
	{
		if (qName == "description")
			gameinfo->description = currentText;
		else if (qName == "manufacturer")
			gameinfo->manufacturer = currentText;
		else if (qName == "year")
			gameinfo->year = currentText;
		return true;
	}

	bool characters(const QString &str)
	{
		currentText += str;
		return true;
	}	

private:
	GameInfo *gameinfo;
	QString currentText;
	QString currentDevice;
	bool metMameTag;
};

LoadIconThread::~LoadIconThread()
{
	done = true;
	wait();
}

void LoadIconThread::load()
{
	QMutexLocker locker(&mutex);

	if (isRunning())
	{
		cancel = true;
	}
	else
	{
		cancel = false;
		done = false;
		
		start(LowPriority);
	}
}

void LoadIconThread::run()
{
	GameInfo *gameInfo, *gameInfo2;

//	win->log(QString("ico count: %1").arg(mamegame->gamenameGameInfoMap.count()));

	while(!done)
	{
		// iteratre split dirpath
		QStringList dirpaths = mameOpts["icons_directory"]->globalvalue.split(";");
		foreach (QString _dirpath, dirpaths)
		{
			QDir dir(_dirpath);
			QString dirpath = utils->getPath(_dirpath);
		
			QStringList nameFilter;
			nameFilter << "*.ico";
			
			// iterate all files in the path
			QStringList files = dir.entryList(nameFilter, QDir::Files | QDir::Readable);
			for (int i = 0; i < files.count(); i++)
			{
				QString gameName = files[i].toLower().remove(".ico");

				if (mamegame->gamenameGameInfoMap.contains(gameName))
				{
					gameInfo = mamegame->gamenameGameInfoMap[gameName];

					if (iconQueue.contains(gameName) && gameInfo->icondata.isNull())
					{
						QFile icoFile(dirpath + gameName + ".ico");
						if (icoFile.open(QIODevice::ReadOnly))
						{
							gameInfo->icondata = icoFile.readAll();
							emit icoUpdated(gameName);
//							win->log(QString("icoUpdated f: %1").arg(gameName));
						}
					}
				}

				if (cancel)
					break;
			}

			if (cancel)
				break;

			// iterate all files in the zip
			QuaZip zip(dirpath + "icons.zip");

			if(!zip.open(QuaZip::mdUnzip))
				continue;
			
			QuaZipFileInfo info;
			QuaZipFile zipFile(&zip);
			for (bool more = zip.goToFirstFile(); more; more = zip.goToNextFile())
			{
				if(!zip.getCurrentFileInfo(&info))
					continue;

				QString gameName = info.name.toLower().remove(".ico");

				if (mamegame->gamenameGameInfoMap.contains(gameName))
				{
					gameInfo = mamegame->gamenameGameInfoMap[gameName];
					// fixme: read cloneof first
					if (iconQueue.contains(gameName) &&
						gameInfo->icondata.isNull())
					{
						QuaZipFile icoFile(&zip);
						if (icoFile.open(QIODevice::ReadOnly))
						{
							gameInfo->icondata = icoFile.readAll();
							emit icoUpdated(gameName);
//							win->log(QString("icoUpdated z: %1").arg(gameName));
						}
					}
				}
				if (cancel)
					break;
			}
			if (cancel)
				break;
		}

		if (!cancel)
		{
	///*
			// get clone icons from parent
			mutex.lock();
			for (int i = 0; i < iconQueue.count(); i++)
			{
				QString gameName = iconQueue.value(i);
	
				if (mamegame->gamenameGameInfoMap.contains(gameName))
				{
					gameInfo = mamegame->gamenameGameInfoMap[gameName];
					if (!gameInfo->isExtRom && gameInfo->icondata.isNull() && !gameInfo->cloneof.isEmpty())
					{
						gameInfo2 = mamegame->gamenameGameInfoMap[gameInfo->cloneof];
						if (!gameInfo2->icondata.isNull())
						{
							gameInfo->icondata = gameInfo2->icondata;
							emit icoUpdated(gameName);
	//						win->log(QString("icoUpdated c: %1").arg(gameName));
						}
					}
				}
	//			else
	//				win->log(QString("errico: %1 %2").arg(gameName).arg(mamegame->gamenameGameInfoMap.count()));
			}
			mutex.unlock();
	//*/

			done = true;
		}

		cancel = false;
	}
}

UpdateSelectionThread::UpdateSelectionThread(QObject *parent)
: QThread(parent)
{
	abort = false;
}

UpdateSelectionThread::~UpdateSelectionThread()
{
	abort = true;
	wait();
}

void UpdateSelectionThread::update()
{
	QMutexLocker locker(&mutex);

	if (!isRunning())
		start(NormalPriority);
}

void UpdateSelectionThread::run()
{
	while (!myqueue.isEmpty() && !abort)
	{
		QString gameName = myqueue.dequeue();

		//fixme: do not update tabbed docks
		if (win->ssSnap->isVisible())
		{
			pmdataSnap = utils->getScreenshot(mameOpts["snapshot_directory"]->globalvalue, gameName);
			emit snapUpdated(DOCK_SNAP);
		}
		if (win->ssFlyer->isVisible())
		{
			pmdataFlyer = utils->getScreenshot(mameOpts["flyer_directory"]->globalvalue, gameName);
			emit snapUpdated(DOCK_FLYER);
		}
		if (win->ssCabinet->isVisible())
		{
			pmdataCabinet = utils->getScreenshot(mameOpts["cabinet_directory"]->globalvalue, gameName);
			emit snapUpdated(DOCK_CABINET);
		}
		if (win->ssMarquee->isVisible())			
		{
			pmdataMarquee = utils->getScreenshot(mameOpts["marquee_directory"]->globalvalue, gameName);
			emit snapUpdated(DOCK_MARQUEE);
		}
		if (win->ssTitle->isVisible())
		{
			pmdataTitle = utils->getScreenshot(mameOpts["title_directory"]->globalvalue, gameName);
			emit snapUpdated(DOCK_TITLE);
		}
		if (win->ssCPanel->isVisible())
		{
			pmdataCPanel = utils->getScreenshot(mameOpts["cpanel_directory"]->globalvalue, gameName);
			emit snapUpdated(DOCK_CPANEL);
		}
		if (win->ssPCB->isVisible())
		{
			pmdataPCB = utils->getScreenshot(mameOpts["pcb_directory"]->globalvalue, gameName);
			emit snapUpdated(DOCK_PCB);
		}
//		static QMovie movie( "xxx.mng" );
//		win->lblPCB->setMovie( &movie );

		QString path;
		if (win->tbHistory->isVisible())
		{
			path = "history.dat";
			if (mameOpts.contains("history_file"))
				path = mameOpts["history_file"]->globalvalue;

			historyText = utils->getHistory(path, gameName);

			emit snapUpdated(DOCK_HISTORY);
		}
		if (win->tbMameinfo->isVisible())
		{
			path = "mameinfo.dat";
			if (mameOpts.contains("mameinfo_file"))
				path = mameOpts["mameinfo_file"]->globalvalue;
		
			mameinfoText = utils->getHistory(path, gameName);
			emit snapUpdated(DOCK_MAMEINFO);
		}
		if (win->tbStory->isVisible())
		{
			path = "story.dat";
			if (mameOpts.contains("story_file"))
				path = mameOpts["story_file"]->globalvalue;

			storyText = utils->getHistory(path, gameName);
			emit snapUpdated(DOCK_STORY);
		}
	}
}

TreeItem::TreeItem(const QList<QVariant> &data, TreeItem *parent)
{
	parentItem = parent;
	itemData = data;
}

TreeItem::~TreeItem()
{
	qDeleteAll(childItems);
}

void TreeItem::appendChild(TreeItem *item)
{
	childItems.append(item);
}

TreeItem *TreeItem::child(int row)
{
	return childItems[row];
}

int TreeItem::childCount() const
{
	return childItems.count();
}

int TreeItem::columnCount() const
{
	return itemData.count();
}

QVariant TreeItem::data(int column) const
{
	return itemData[column];
}

int TreeItem::row() const
{
	if (parentItem)
		return parentItem->childItems.indexOf(const_cast<TreeItem*>(this));

	return 0;
}

TreeItem *TreeItem::parent()
{
	return parentItem;
}

bool TreeItem::setData(int column, const QVariant &value)
{
	if (column < 0 || column >= itemData.size())
		return false;

	itemData[column] = value;
	return true;
}

TreeModel::TreeModel(QObject *parent, bool isGroup)
: QAbstractItemModel(parent)
{
	QList<QVariant> rootData;

	static const QStringList columnList = (QStringList() 
		<< QT_TR_NOOP("Description") 
		<< QT_TR_NOOP("Name") 
		<< QT_TR_NOOP("ROMs") 
		<< QT_TR_NOOP("Manufacturer") 
		<< QT_TR_NOOP("Driver") 
		<< QT_TR_NOOP("Year") 
		<< QT_TR_NOOP("Clone of"));

	foreach (QString header, columnList)
		rootData << tr(qPrintable(header));

	rootItem = new TreeItem(rootData);

	foreach (QString gameName, mamegame->gamenameGameInfoMap.keys())
	{
		if (gameName.trimmed()=="")
		win->log("ERR1");
		GameInfo *gameInfo = mamegame->gamenameGameInfoMap[gameName];

		// build parent
		if (!isGroup || gameInfo->cloneof.isEmpty())
		{
			TreeItem *parent = buildItem(rootItem, gameName, isGroup);

			// build clones
			if (isGroup)
			foreach (QString cloneName, gameInfo->clones)
				buildItem(parent, cloneName, isGroup);
		}
	}
}

TreeItem * TreeModel::buildItem(TreeItem *parent, QString gameName, bool isGroup)
{
	GameInfo *gameInfo = mamegame->gamenameGameInfoMap[gameName];

	if (gameName.trimmed()=="")
		win->log("ERR2");

	QList<QVariant> columnData;
	columnData << gameInfo->description;
	columnData << gameName;
	columnData << gameInfo->available;
	columnData << gameInfo->manufacturer;
	columnData << gameInfo->sourcefile;
	columnData << gameInfo->year;
	columnData << gameInfo->cloneof;

	// Append a new item to the current parent's list of children
//	if (gameInfo->pModItem)
//		delete gameInfo->pModItem;
	gameInfo->pModItem = new TreeItem(columnData, parent);
	parent->appendChild(gameInfo->pModItem);
	return gameInfo->pModItem;
}

TreeModel::~TreeModel()
{
	delete rootItem;
}

int TreeModel::columnCount(const QModelIndex &parent) const
{
	return rootItem->columnCount();
}

QVariant TreeModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();

	TreeItem *item = getItem(index);
	const QString gameName = item->data(1).toString();
	GameInfo *gameInfo = mamegame->gamenameGameInfoMap[gameName];
	int col = index.column();

	switch (role)
	{
	case Qt::ForegroundRole:
		if (gameInfo->emulation == 0 && !gameInfo->isExtRom)
			return qVariantFromValue(QColor(isDarkBg ? QColor(255, 96, 96) : Qt::darkRed));
		else
			//fixme: use palette color
			return qVariantFromValue(QColor(isDarkBg ? Qt::white : Qt::black));

	case Qt::DecorationRole:
		if (col == COL_DESC)
		{
			QByteArray icondata;
			if (gameInfo->icondata.isNull())
			{
				gamelist->iconThread.iconQueue.setSize(win->tvGameList->viewport()->height() / 17 + 2);
				gamelist->iconThread.iconQueue.enqueue(gameName);
				gamelist->iconThread.load();
				icondata = utils->deficondata;
			}
			else
				icondata = gameInfo->icondata;
			
			QPixmap pm;
			pm.loadFromData(icondata, "ico");
			pm = pm.scaled(QSize(16, 16), Qt::KeepAspectRatio, Qt::SmoothTransformation);

			if(!gameInfo->available)
			{
				QPainter p;
				p.begin(&pm);
				p.drawPixmap(8, 8, QPixmap(":/res/status-na.png"));
				p.end();
			}
			
			return QIcon(pm);
		}
		break;

 	case Qt::UserRole + FOLDER_BIOS:
		return gameInfo->biosof();
		
	case Qt::UserRole + FOLDER_CONSOLE:
		return gameInfo->isExtRom ? true : false;

	case Qt::UserRole:
		//convert 'Name' column for Console
		if (col == COL_NAME && gameInfo->isExtRom)
			return item->data(col);
		break;
		
	case Qt::DisplayRole:
		if (col == COL_DESC && !gameInfo->lcDesc.isEmpty() && local_game_list)
			return gameInfo->lcDesc;

		else if (col == COL_MFTR && !gameInfo->lcMftr.isEmpty() && local_game_list)
			return gameInfo->lcMftr;

		else if (col == COL_YEAR && gameInfo->year.isEmpty())
			return "?";

		else if (col == COL_NAME && gameInfo->isExtRom)
			return gameInfo->romof;
	
		//convert 'ROMs' column
		else if (col == COL_ROM)
		{
			int status = item->data(COL_ROM).toInt();
			switch (status)
			{
			case -1:
				return "";
				
			case 0:
				return "No";

			case 1:
				return "Yes";
			}
		}

		return item->data(col);
	}
	return QVariant();
}

Qt::ItemFlags TreeModel::flags(const QModelIndex &index) const
{
	if (!index.isValid())
		return Qt::ItemIsEnabled;

	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

TreeItem * TreeModel::getItem(const QModelIndex &index) const
{
	if (index.isValid())
	{
		TreeItem *item = static_cast<TreeItem*>(index.internalPointer());
		if (item)
			return item;
	}
	return rootItem;
}

QVariant TreeModel::headerData(int section, Qt::Orientation orientation,
							   int role) const
{
	if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
		return rootItem->data(section);

	return QVariant();
}

QModelIndex TreeModel::index(int row, int column, const QModelIndex &parent) const
{
	if (parent.isValid() && parent.column() != 0)
		return QModelIndex();

	TreeItem *parentItem = getItem(parent);

	TreeItem *childItem = parentItem->child(row);
	if (childItem)
		return createIndex(row, column, childItem);
	else
		return QModelIndex();
}

QModelIndex TreeModel::index(int column, TreeItem *childItem) const
{
	if (childItem)
		return createIndex(childItem->row(), column, childItem);
	else
		return QModelIndex();
}

QModelIndex TreeModel::parent(const QModelIndex &index) const
{
	if (!index.isValid())
		return QModelIndex();

	TreeItem *childItem = getItem(index);
	TreeItem *parentItem = childItem->parent();

	if (parentItem == rootItem)
		return QModelIndex();

	return createIndex(parentItem->row(), 0, parentItem);
}

int TreeModel::rowCount(const QModelIndex &parent) const
{
	TreeItem *parentItem = getItem(parent);

	return parentItem->childCount();
}

void TreeModel::rowChanged(const QModelIndex &index)
{

	QModelIndex i = index.sibling(index.row(), 0);
	QModelIndex j = index.sibling(index.row(), columnCount() - 1);

	emit dataChanged(i, j);
}

bool TreeModel::setData(const QModelIndex &index, const QVariant &value,
						int role)
{
	TreeItem *item = getItem(index);

	if (index.isValid())
	{
		switch (role)
		{
		case Qt::DisplayRole:
			if (item->setData(index.column(), value))
				emit dataChanged(index, index);
			return true;
			break;

		case Qt::DecorationRole:
			if(!value.isNull())
			{
				emit dataChanged(index, index);
				return true;
			}
			break;
		}
	}
	return false;
}

bool TreeModel::setHeaderData(int section, Qt::Orientation orientation,
							  const QVariant &value, int role)
{
	if (role != Qt::EditRole || orientation != Qt::Horizontal)
		return false;

	return rootItem->setData(section, value);
}


RomInfo::RomInfo(QObject *parent)
: QObject(parent)
{
	available = false;
	//	win->log("# RomInfo()");
}

RomInfo::~RomInfo()
{
	//    win->log("# ~RomInfo()");
}

BiosInfo::BiosInfo(QObject *parent)
: QObject(parent)
{
	//	win->log("# BiosInfo()");
}

DeviceInfo::DeviceInfo(QObject *parent)
: QObject(parent)
{
	//	win->log("# DeviceInfo()");
}

GameInfo::GameInfo(QObject *parent) :
QObject(parent),
cocktail(64),
protection(64),
available(0)
{
	//	win->log("# GameInfo()");
}

GameInfo::~GameInfo()
{
	available = -1;
//	win->log("# ~GameInfo()");
}

QString GameInfo::biosof()
{
	QString biosof;
	GameInfo *gameInfo = NULL;

	if (!romof.isEmpty())
	{
		biosof = romof;
			if (romof.trimmed()=="")
		win->log("ERR4");
		gameInfo = mamegame->gamenameGameInfoMap[romof];

		if (!gameInfo->romof.isEmpty())
		{
			biosof = gameInfo->romof;
				if (gameInfo->romof.trimmed()=="")
		win->log("ERR5");
			gameInfo = mamegame->gamenameGameInfoMap[gameInfo->romof];
		}
	}
	
	if (gameInfo && gameInfo->isBios)
		return biosof;
	else
		return NULL;
}

MameGame::MameGame(QObject *parent)
: QObject(parent)
{
	win->log("# MameGame()");
	this->mameVersion = mameVersion;
}

MameGame::~MameGame()
{
	win->log("# ~MameGame()");
}

void MameGame::s11n()
{
	win->log("start s11n()");

	QDir().mkpath(CFG_PREFIX + "cache");
	QFile file(CFG_PREFIX + "cache/gamelist.cache");
	file.open(QIODevice::WriteOnly);
	QDataStream out(&file);

	out << (quint32)MAMEPLUS_SIG; //mameplus signature
	out << (qint16)S11N_VER; //s11n version
	out.setVersion(QDataStream::Qt_4_3);
	out << mamegame->mameVersion;
	out << mamegame->mameDefaultIni;	//default.ini
	out << mamegame->gamenameGameInfoMap.count();

	win->log(QString("s11n %1 games").arg(mamegame->gamenameGameInfoMap.count()));

	//fixme: should place in thread and use mamegame directly
	gamelist->switchProgress(gamelist->numTotalGames, tr("Saving listxml"));
	int i = 0;
	foreach (QString gamename, mamegame->gamenameGameInfoMap.keys())
	{
		gamelist->updateProgress(i++);
		qApp->processEvents();
	
		GameInfo *gameinfo = mamegame->gamenameGameInfoMap[gamename];
		out << gamename;
		out << gameinfo->description;
		out << gameinfo->year;
		out << gameinfo->manufacturer;
		out << gameinfo->sourcefile;
		out << gameinfo->isBios;
		out << gameinfo->isExtRom;
		out << gameinfo->cloneof;
		out << gameinfo->romof;
		out << gameinfo->available;

		out << gameinfo->status;
		out << gameinfo->emulation;
		out << gameinfo->color;
		out << gameinfo->sound;
		out << gameinfo->graphic;
		out << gameinfo->cocktail;
		out << gameinfo->protection;
		out << gameinfo->savestate;
		out << gameinfo->palettesize;

		out << gameinfo->clones.count();
		foreach (QString clonename, gameinfo->clones)
			out << clonename;

		out << gameinfo->crcRomInfoMap.count();
		foreach (quint32 crc, gameinfo->crcRomInfoMap.keys())
		{
			RomInfo *rominfo = gameinfo->crcRomInfoMap[crc];
			out << crc;
			out << rominfo->name;
			out << rominfo->status;
			out << rominfo->size;
		}

		out << gameinfo->nameBiosInfoMap.count();
		foreach (QString name, gameinfo->nameBiosInfoMap.keys())
		{
			BiosInfo *biosinfo = gameinfo->nameBiosInfoMap[name];
			out << name;
			out << biosinfo->description;
			out << biosinfo->isdefault;
		}

		out << gameinfo->nameDeviceInfoMap.count();
		foreach (QString name, gameinfo->nameDeviceInfoMap.keys())
		{
			DeviceInfo *deviceinfo = gameinfo->nameDeviceInfoMap[name];
			out << name;
			out << deviceinfo->mandatory;
			out << deviceinfo->extension;
		}
	}
	gamelist->switchProgress(-1, "");
	
	file.close();
}

int MameGame::des11n()
{
	QFile file(CFG_PREFIX + "cache/gamelist.cache");
	file.open(QIODevice::ReadOnly);
	QDataStream in(&file);

	// Read and check the header
	quint32 mamepSig;
	in >> mamepSig;
	if (mamepSig != MAMEPLUS_SIG)
	{
		win->log(tr("Cache signature error."));
		return QDataStream::ReadCorruptData;
	}

	// Read the version
	qint16 version;
	in >> version;
	if (version != S11N_VER)
	{
		win->log(tr("Cache version has been updated. A full refresh is required."));
		return QDataStream::ReadCorruptData;
	}

	if (version < 1)
		in.setVersion(QDataStream::Qt_4_2);
	else
		in.setVersion(QDataStream::Qt_4_3);

	//finished checking
	if (mamegame)
		delete mamegame;
	mamegame = new MameGame(win);

	// MAME Version
	mamegame->mameVersion = utils->getMameVersion();
	QString mameVersion0;
	in >> mameVersion0;

	// default mame.ini text
	in >> mamegame->mameDefaultIni;
	win->log(QString("mamegame->mameDefaultIni.count %1").arg(mamegame->mameDefaultIni.count()));
	
	int gamecount;
	in >> gamecount;

	for (int i = 0; i < gamecount; i++)
	{
		GameInfo *gameinfo = new GameInfo(mamegame);
		QString gamename;
		in >> gamename;
		in >> gameinfo->description;
		in >> gameinfo->year;
		in >> gameinfo->manufacturer;
		in >> gameinfo->sourcefile;
		in >> gameinfo->isBios;
		in >> gameinfo->isExtRom;
		in >> gameinfo->cloneof;
		in >> gameinfo->romof;
		in >> gameinfo->available;

		in >> gameinfo->status;
		in >> gameinfo->emulation;
		in >> gameinfo->color;
		in >> gameinfo->sound;
		in >> gameinfo->graphic;
		in >> gameinfo->cocktail;
		in >> gameinfo->protection;
		in >> gameinfo->savestate;
		in >> gameinfo->palettesize;

		mamegame->gamenameGameInfoMap[gamename] = gameinfo;

		int count;
		QString clone;
		in >> count;
		for (int j = 0; j < count; j++)
		{
			in >> clone;
			gameinfo->clones.insert(clone);
		}

		quint32 crc;
		in >> count;
		for (int j = 0; j < count; j++)
		{
			RomInfo *rominfo = new RomInfo(gameinfo);
			in >> crc;
			in >> rominfo->name;
			in >> rominfo->status;
			in >> rominfo->size;
			gameinfo->crcRomInfoMap[crc] = rominfo;
		}

		QString name;
		in >> count;
		for (int j = 0; j < count; j++)
		{
			BiosInfo *biosinfo = new BiosInfo(gameinfo);
			in >> name;
			in >> biosinfo->description;
			in >> biosinfo->isdefault;
			gameinfo->nameBiosInfoMap[name] = biosinfo;
		}

		in >> count;
		for (int j = 0; j < count; j++)
		{
			DeviceInfo *deviceinfo = new DeviceInfo(gameinfo);
			in >> name;
			in >> deviceinfo->mandatory;
			in >> deviceinfo->extension;
			gameinfo->nameDeviceInfoMap[name] = deviceinfo;
		}
	}

	win->log(QString("des11n2.gamecount %1").arg(gamecount));

	// verify MAME Version
	if (mamegame->mameVersion != mameVersion0)
	{
		win-> log(QString("new mame version: %1 / %2").arg(mameVersion0).arg(mamegame->mameVersion));
		mamegame0 = mamegame;
		return QDataStream::ReadCorruptData;
	}

	return in.status();
}


GamelistDelegate::GamelistDelegate(QObject *parent)
: QItemDelegate(parent)
{
}

QSize GamelistDelegate::sizeHint (const QStyleOptionViewItem & option, 
								  const QModelIndex & index) const
{
	QString gameName = gamelist->getViewString(index, COL_NAME);
	GameInfo *gameInfo = mamegame->gamenameGameInfoMap[gameName];
//fixme: combine @ console gamename
	if (!gameInfo->nameDeviceInfoMap.isEmpty())
	{
		QString gameName2 = gamelist->getViewString(index, COL_NAME + COL_LAST);

		if (!gameName2.isEmpty())
		{
			gameInfo = mamegame->gamenameGameInfoMap[gameName2];
			if (gameInfo && gameInfo->isExtRom)
				gameName = gameName2;
		}
	}

	if (currentGame == gameName)
		return QSize(1,33);
	else
		return QSize(1,17);
}

void GamelistDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
							 const QModelIndex &index ) const
{
	QString gameName = gamelist->getViewString(index, COL_NAME);
	GameInfo *gameInfo = mamegame->gamenameGameInfoMap[gameName];
//fixme: combine @ console gamename
	if (!gameInfo->nameDeviceInfoMap.isEmpty())
	{
		QString gameName2 = gamelist->getViewString(index, COL_NAME + COL_LAST);
		if (!gameName2.isEmpty())
		{
			gameInfo = mamegame->gamenameGameInfoMap[gameName2];
			if (gameInfo && gameInfo->isExtRom)
				gameName = gameName2;
		}
	}
	
	QString gameDesc = gamelist->getViewString(index, COL_DESC);
	
	if (currentGame == gameName && index.column() == COL_DESC)
	{
		//draw big icon
		QRect rc = option.rect;
		QPoint p = rc.topLeft();
		p.setX(p.x() + 2);
		rc = QRect(p, rc.bottomRight());

		if (gameName.trimmed()=="")
			win->log("ERRb");

		GameInfo *gameInfo = mamegame->gamenameGameInfoMap[gameName];

		QByteArray icondata;
		if (gameInfo->icondata.isNull())
		{
			icondata = utils->deficondata;
//			win->log("paint def ico: " + gameName);
		}
		else
		{
			icondata = gameInfo->icondata;
//			win->log("paint game ico: " + gameName);
		}
		
		QPixmap pm;
		pm.loadFromData(icondata, "ico");

		if(!gameInfo->available)
		{
			QPainter p;
			p.begin(&pm);
			p.drawPixmap(24, 24, QPixmap(":/res/status-na.png"));
			p.end();
		}

		QApplication::style()->drawItemPixmap (painter, rc, Qt::AlignLeft | Qt::AlignVCenter, pm);

		// calc text rect
		p = rc.topLeft();
		p.setX(p.x() + 34);
		rc = QRect(p, rc.bottomRight());

		//draw text bg
		if (option.state & QStyle::State_Selected)
		{
/*			painter->fillRect(rc, QBrush(
				QPixmap("res/images_nav_tab_sel.png").scaled(QSize(33, 33))
				));
*/
			painter->fillRect(rc, option.palette.highlight());
		}

		//draw text
		p = rc.topLeft();
		p.setX(p.x() + 2);
		rc = QRect(p, rc.bottomRight());
		QApplication::style()->drawItemText(painter, rc, Qt::AlignLeft | Qt::AlignVCenter, option.palette, true, 
											gameDesc, QPalette::HighlightedText);
		return;
	}

	QItemDelegate::paint(painter, option, index);
	return;
}


Gamelist::Gamelist(QObject *parent)
: QObject(parent)
{
	connect(&iconThread, SIGNAL(icoUpdated(QString)), this, SLOT(setupIcon(QString)));
//	connect(&iconThread.iconQueue, SIGNAL(logStatusUpdated(QString)), win, SLOT(logStatus(QString)));

	connect(&selectThread, SIGNAL(snapUpdated(int)), this, SLOT(setupSnap(int)));
	
	if (!searchTimer)
		searchTimer = new QTimer(this);
	connect(searchTimer, SIGNAL(timeout()), this, SLOT(filterRegExpChanged()));

	mAuditor = new MergedRomAuditor(parent);

	numTotalGames = -1;
	loadProc = NULL;
	menu = NULL;
	headerMenu = NULL;
}

Gamelist::~Gamelist()
{
//	win->log("DEBUG: Gamelist::~Gamelist()");

	if ( loadProc )
		loadProc->terminate();
}

void Gamelist::updateProgress(int progress)
{
	win->progressBarGamelist->setValue(progress);
}

void Gamelist::switchProgress(int max, QString title)
{
	win->logStatus(title);

	if (max != -1)
	{
		win->statusbar->addWidget(win->progressBarGamelist);
		win->progressBarGamelist->setRange(0, max);
		win->progressBarGamelist->reset();
		win->progressBarGamelist->show();
	}
	else
	{
		win->statusbar->removeWidget(win->progressBarGamelist);
	}
}

QString Gamelist::getViewString(const QModelIndex &index, int column) const
{
	bool isUserValue = false;
	if (column >= COL_LAST)
	{
		column -= COL_LAST;
		isUserValue = true;
	}

	QModelIndex j = index.sibling(index.row(), column);
	//fixme: sometime model's NULL...
	if (!index.model())
		return "";

	if (isUserValue)
		return index.model()->data(j, Qt::UserRole).toString();
	else
		return index.model()->data(j, Qt::DisplayRole).toString();
}

void Gamelist::updateSelection(const QModelIndex & current, const QModelIndex & previous)
{
//	win->log("updateSelection()");

	if (current.isValid())
	{
		QString gameName = getViewString(current, COL_NAME);
		if (gameName.isEmpty())
			return;

		QString gameDesc = getViewString(current, COL_DESC);

		GameInfo *gameInfo = mamegame->gamenameGameInfoMap[gameName];
		if (gameName.trimmed() == "")
			win->log("ERRc");

		if (!gameInfo->nameDeviceInfoMap.isEmpty())
		{
			QString gameName2 = getViewString(current, COL_NAME + COL_LAST);
			if (!gameName2.isEmpty())
			{
				if (gameName2.trimmed() == "")
					win->log("ERRd");

				gameInfo = mamegame->gamenameGameInfoMap[gameName2];
				if (gameInfo && gameInfo->isExtRom)
					gameName = gameName2;
			}
		}

		currentGame = gameName;
		
		//update statusbar
		win->logStatus(gameDesc);

		win->logStatus(gameInfo);
/*
		if (!gameInfo->nameDeviceInfoMap.isEmpty())
		{
			DeviceInfo *deviceinfo = gameInfo->nameDeviceInfoMap["cartridge"];
			if (deviceinfo)
				win->log(deviceinfo->extension.join(", "));
		}
*/
		selectThread.myqueue.enqueue(currentGame);
		selectThread.update();

		//update selected rows
		gameListModel->rowChanged(gameListPModel->mapToSource(current));
		gameListModel->rowChanged(gameListPModel->mapToSource(previous));
	}
	else
		currentGame = "";
}

void Gamelist::restoreGameSelection()
{
	if (!gameListModel || !gameListPModel)
		return;

	QString gameName = currentGame;
	QModelIndex i;

	// select current game
	if (mamegame->gamenameGameInfoMap.contains(gameName))
	{
		GameInfo *gameinfo = mamegame->gamenameGameInfoMap[gameName];
		i = gameListModel->index(0, gameinfo->pModItem);
		win->log("restore callback: " + gameName);
	}

	QModelIndex pi = gameListPModel->mapFromSource(i);

	if (!pi.isValid())
	{
		// select first row otherwise
		pi = gameListPModel->index(0, 0, QModelIndex());
	}

	win->tvGameList->setCurrentIndex(pi);
	win->lvGameList->setCurrentIndex(pi);
	//fixme: PositionAtCenter doesnt work well (auto scroll right)
	win->tvGameList->scrollTo(pi, QAbstractItemView::PositionAtTop);
	win->lvGameList->scrollTo(pi, QAbstractItemView::PositionAtTop);

	win->labelGameCount->setText(tr("%1 games").arg(visibleGames.count()));
/*
	win->log("--- cleared ---");
	foreach (QString g, visibleGames)
		win->log("vis: " + g);
*/
}

void Gamelist::setupIcon(QString gameName)
{
	if (!gameListModel || !gameListPModel)
		return;

	GameInfo *gameInfo = mamegame->gamenameGameInfoMap[gameName];

	gameListModel->setData(gameListModel->index(0, gameInfo->pModItem), 
		gameInfo->icondata, Qt::DecorationRole);
//	win->log(QString("setupico %1 %2").arg(gameName).arg(gameInfo->icondata.isNull()));
}

void Gamelist::setupSnap(int snapType)
{
	switch (snapType)
	{
	case DOCK_SNAP:
		win->ssSnap->setPixmap(selectThread.pmdataSnap, win->actionEnforceAspect->isChecked());
		break;
	case DOCK_FLYER:
		win->ssFlyer->setPixmap(selectThread.pmdataFlyer);
		break;
	case DOCK_CABINET:
		win->ssCabinet->setPixmap(selectThread.pmdataCabinet);
		break;
	case DOCK_MARQUEE:
		win->ssMarquee->setPixmap(selectThread.pmdataMarquee);
		break;
	case DOCK_TITLE:
		win->ssTitle->setPixmap(selectThread.pmdataTitle, win->actionEnforceAspect->isChecked());
		break;
	case DOCK_CPANEL:
		win->ssCPanel->setPixmap(selectThread.pmdataCPanel);
		break;
	case DOCK_PCB:
		win->ssPCB->setPixmap(selectThread.pmdataPCB);
		break;

	case DOCK_HISTORY:
		win->tbHistory->setHtml(selectThread.historyText);
		break;
	case DOCK_MAMEINFO:
		win->tbMameinfo->setHtml(selectThread.mameinfoText);
		break;
	case DOCK_STORY:
		win->tbStory->setHtml(selectThread.storyText);
	}
}

void Gamelist::init(bool toggleState, int initMethod)
{
	if (!toggleState)
		return;

	// get current game list mode
	if (win->actionDetails->isChecked())
		list_mode = win->actionDetails->objectName().remove("action");
	else if (win->actionLargeIcons->isChecked())
		list_mode = win->actionLargeIcons->objectName().remove("action");
	else
		list_mode = win->actionGrouped->objectName().remove("action");

	int des11n_status = QDataStream::Ok;

	// only des11n on app start
	//fixme: illogical call before mamegame init
	if (!mamegame)
		des11n_status = mamegame->des11n();

//	win->poplog(QString("des11n status: %1").arg(des11n_status));

	if (des11n_status == QDataStream::Ok)
	{
		// disable ctrl updating before deleting its model
		win->lvGameList->disconnect(SIGNAL(activated(const QModelIndex &)));
		win->lvGameList->disconnect(SIGNAL(customContextMenuRequested(const QPoint &)));
		win->lvGameList->hide();
		win->layMainView->removeWidget(win->lvGameList);

		win->tvGameList->disconnect(SIGNAL(activated(const QModelIndex &)));
		win->tvGameList->disconnect(SIGNAL(customContextMenuRequested(const QPoint &)));
		win->tvGameList->header()->disconnect(SIGNAL(customContextMenuRequested(const QPoint &)));
		win->tvGameList->hide();
		win->layMainView->removeWidget(win->tvGameList);

		if (gameListModel)
			delete gameListModel;

		// Grouped
		if (list_mode == win->actionGrouped->objectName().remove("action"))
		{
			gameListModel = new TreeModel(win, true);
			win->layMainView->addWidget(win->tvGameList);		
		}
		// Large Icons
		else if (list_mode == win->actionLargeIcons->objectName().remove("action"))
		{
			gameListModel = new TreeModel(win, false);
			win->layMainView->addWidget(win->lvGameList);		
		}
		// Details
		else
		{
			gameListModel = new TreeModel(win, false);
			win->layMainView->addWidget(win->tvGameList);		
		}

		if (gameListPModel)
			delete gameListPModel;
		gameListPModel = new GameListSortFilterProxyModel(win);

		gameListPModel->setSourceModel(gameListModel);
		gameListPModel->setSortCaseSensitivity(Qt::CaseInsensitive);

		if (list_mode == win->actionLargeIcons->text())
			win->lvGameList->setModel(gameListPModel);
		else
		{
			win->tvGameList->setModel(gameListPModel);
			win->tvGameList->setItemDelegate(&gamelistDelegate);
		}

		if (initMethod == GAMELIST_INIT_FULL)
		{
			/* init everything else here after we have mamegame */
			// init folders
			gamelist->initFolders();

			//fixme: something here should be moved to opt
			// init options from default mame.ini
			optUtils->loadDefault(mamegame->mameDefaultIni);
			// assign option type, defvalue, min, max, etc. from template
			optUtils->loadTemplate();
			// load mame.ini overrides
			optUtils->loadIni(OPTLEVEL_GLOBAL, "mame.ini");

			// add GUI MESS extra software paths
			foreach (QString gameName, mamegame->gamenameGameInfoMap.keys())
			{
				GameInfo *gameInfo = mamegame->gamenameGameInfoMap[gameName];
				if (!gameInfo->nameDeviceInfoMap.isEmpty())
				{
					MameOption *pMameOpt = new MameOption(0);	//fixme parent
					pMameOpt->guivisible = true;
					mameOpts[gameName + "_extra_software"] = pMameOpt;
					//save a list of console system for later use
					consoleGamesL << gameName;
				}
				/*
				if (gameInfo->isBios)
				{
					MameOption *pMameOpt = mameOpts["bios"];
					foreach (QString name, gameInfo->nameBiosInfoMap.keys())
					{
						BiosInfo *bi = gameInfo->nameBiosInfoMap[name];
						win->log(bi->description);
					}
				}
				*/
			}
			consoleGamesL.sort();

			// load GUI path overrides
			foreach (QString optName, mameOpts.keys())
			{
				MameOption *pMameOpt = mameOpts[optName];
			
				if (!pMameOpt->guivisible)
					continue;

				if (guiSettings.contains(optName))
					pMameOpt->globalvalue = guiSettings.value(optName).toString();
			}

			// misc win init
			QFileInfo fi(mame_binary);

			win->setWindowTitle(QString("%1 - %2 %3")
				.arg(win->windowTitle())
				.arg(fi.baseName().toUpper())
				.arg(mamegame->mameVersion));
			win->actionProperties->setEnabled(true);
			win->actionSrcProperties->setEnabled(true);
			win->actionDefaultOptions->setEnabled(true);
		}

		loadMMO(UI_MSG_LIST);
		loadMMO(UI_MSG_MANUFACTURE);

		connect(win->tvGameList, SIGNAL(activated(const QModelIndex &)), this, SLOT(runMame()));
		connect(win->lvGameList, SIGNAL(activated(const QModelIndex &)), this, SLOT(runMame()));

		// connect gameListModel/gameListPModel signals after the view init completed
		// connect gameListModel/gameListPModel signals after mameOpts init
		connect(gameListPModel, SIGNAL(layoutChanged()), this, SLOT(restoreGameSelection()));
		connect(win->tvGameList->selectionModel(),
				SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
				this, SLOT(updateSelection(const QModelIndex &, const QModelIndex &)));
		connect(win->lvGameList->selectionModel(),
				SIGNAL(currentChanged(const QModelIndex &, const QModelIndex &)),
				this, SLOT(updateSelection(const QModelIndex &, const QModelIndex &)));

		//fixme: how about lv?
		win->tvGameList->setSortingEnabled(true);
		win->tvGameList->setFocus();

		//refresh current list
		filterRegExpChanged2(win->treeFolders->currentItem());

		// Grouped
		if (list_mode == win->actionGrouped->text())
			win->tvGameList->show();
		// Large Icons
		else if (list_mode == win->actionLargeIcons->text())
			win->lvGameList->show();
		// Details
		else
			win->tvGameList->show();

		// restore game list column state
		if (initMethod != GAMELIST_INIT_DIR)
		{
			QByteArray column_state;

			// restore view column state, needed on first init and after auditing, but not for folder switching
			if (guiSettings.value("column_state").isValid())
				column_state = guiSettings.value("column_state").toByteArray();
			else
				column_state = defSettings.value("column_state").toByteArray();

			win->tvGameList->header()->restoreState(column_state);
			restoreFolderSelection();
		}
		
		// everything is done
		win->treeFolders->setEnabled(true);
		win->actionDetails->setEnabled(true);
		win->actionGrouped->setEnabled(true);
		win->actionRefresh->setEnabled(true);

		// attach menus
		initMenus();

		//override font for CJK OS
		//fixme: move to main?
		QFont font;
		font.setPointSize(9);
		menu->setFont(font);
		headerMenu->setFont(font);
		win->tvGameList->header()->setFont(font);

		win->log(QString("inited.gamecount %1").arg(mamegame->gamenameGameInfoMap.count()));
	}
	else
	{
		//we'll call init() again later
		mameOutputBuf = "";
		QStringList args;

		args << "-listxml";
		loadTimer.start();
		loadProc = procMan->process(procMan->start(mame_binary, args, FALSE));

		connect(loadProc, SIGNAL(started()), this, SLOT(loadListXmlStarted()));
		connect(loadProc, SIGNAL(readyReadStandardOutput()), this, SLOT(loadListXmlReadyReadStandardOutput()));
		connect(loadProc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(loadListXmlFinished(int, QProcess::ExitStatus)));
	}
}

void Gamelist::loadMMO(int msgCat)
{
	static const QStringList msgFileName = (QStringList() 
		<< "mame"
		<< "lst"
		<< "readings"
		<< "manufact");

	QString dirpath;
	//only mameplus contains this option for now
	if (mameOpts.contains("translation_directory"))
		dirpath = utils->getPath(mameOpts["translation_directory"]->globalvalue);
	else
		dirpath = "lang/";
	QFile file( dirpath + language + "/" + msgFileName[msgCat] + ".mmo");
	if (!file.exists())
	{
		win->log("not exist: " + dirpath + language + "/" + msgFileName[msgCat] + ".mmo");
		return;
	}

	struct mmo_header
	{
		int dummy;
		int version;
		int num_msg;
	};
	
	struct mmo_data
	{
		const unsigned char *uid;
		const unsigned char *ustr;
		const void *wid;
		const void *wstr;
	};
	
	struct mmo {
		enum {
			MMO_NOT_LOADED,
			MMO_NOT_FOUND,
			MMO_READY
		} status;
	
		struct mmo_header header;
		struct mmo_data *mmo_index;
		char *mmo_str;
	};
	
	struct mmo _mmo;
	struct mmo *pMmo = &_mmo;
	QHash<QString, QString> mmohash;
	int size = sizeof pMmo->header;

	if (!file.open(QIODevice::ReadOnly))
		goto mmo_readerr;

	if (file.read((char*)&pMmo->header, size) != size)
		goto mmo_readerr;

	if (pMmo->header.dummy)
		goto mmo_readerr;

	if (pMmo->header.version != 3)
		goto mmo_readerr;

	pMmo->mmo_index = (mmo_data*)malloc(pMmo->header.num_msg * sizeof(pMmo->mmo_index[0]));
	if (!pMmo->mmo_index)
		goto mmo_readerr;

	size = pMmo->header.num_msg * sizeof(pMmo->mmo_index[0]);
	if (file.read((char*)pMmo->mmo_index, size) != size)
		goto mmo_readerr;

	int str_size;
	size = sizeof(str_size);
	if (file.read((char*)&str_size, size) != size)
		goto mmo_readerr;

	pMmo->mmo_str = (char*)malloc(str_size);
	if (!pMmo->mmo_str)
		goto mmo_readerr;

	if (file.read((char*)pMmo->mmo_str, str_size) != str_size)
		goto mmo_readerr;

	for (int i = 0; i < pMmo->header.num_msg; i++)
	{
		QString name((char*)((unsigned char*)pMmo->mmo_str + (unsigned long)pMmo->mmo_index[i].uid));
		QString localName = QString::fromUtf8((char*)((unsigned char*)pMmo->mmo_str + (unsigned long)pMmo->mmo_index[i].ustr));
		mmohash[name] = localName;
	}

	foreach (QString gameName, mamegame->gamenameGameInfoMap.keys())
	{
		GameInfo *gameInfo = mamegame->gamenameGameInfoMap[gameName];
		switch(msgCat)
		{
			case UI_MSG_LIST:
				if (mmohash.contains(gameInfo->description))
					gameInfo->lcDesc = mmohash[gameInfo->description];				
				break;
			case UI_MSG_MANUFACTURE:
				if (mmohash.contains(gameInfo->manufacturer))
					gameInfo->lcMftr = mmohash[gameInfo->manufacturer];
				break;
		}
	}

mmo_readerr:
	if (pMmo->mmo_str)
	{
		free(pMmo->mmo_str);
		pMmo->mmo_str = NULL;
	}

	if (pMmo->mmo_index)
	{
		free(pMmo->mmo_index);
		pMmo->mmo_index = NULL;
	}

	file.close();
}

void Gamelist::initMenus()
{
	// init context menu, we don't need to init it twice
	if (menu == NULL)
	{
		menu = new QMenu(win->tvGameList);
	
		menu->addAction(win->actionPlay);
		menu->addAction(win->actionPlayInp);
		menu->addSeparator();
		menu->addAction(win->actionAudit);
		menu->addSeparator();
		menu->addAction(win->actionConfigIPS);
		menu->addAction(win->actionSrcProperties);
		menu->addAction(win->actionProperties);
	}
	
	win->tvGameList->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(win->tvGameList, SIGNAL(customContextMenuRequested(const QPoint &)), 
			this, SLOT(showContextMenu(const QPoint &)));
	connect(menu, SIGNAL(aboutToShow()), this, SLOT(updateContextMenu()));
	connect(win->menuFile, SIGNAL(aboutToShow()), this, SLOT(updateContextMenu()));
	
	//init gamelist header context menu
	if (headerMenu == NULL)
	{
		headerMenu = new QMenu(win->tvGameList);
	
		headerMenu->addAction(win->actionColDescription);
		headerMenu->addAction(win->actionColName);
		headerMenu->addAction(win->actionColROMs);
		headerMenu->addAction(win->actionColManufacturer);
		headerMenu->addAction(win->actionColDriver);
		headerMenu->addAction(win->actionColYear);
		headerMenu->addAction(win->actionColCloneOf);
		headerMenu->addSeparator();
		headerMenu->addAction(win->actionColSortAscending);
		headerMenu->addAction(win->actionColSortDescending);
	
		// add sorting action to an exclusive group
		QActionGroup *sortingActions = new QActionGroup(headerMenu);
		sortingActions->addAction(win->actionColSortAscending);
		sortingActions->addAction(win->actionColSortDescending);
	}

//fixme: for some reason, it doesnt work on linux?
#ifdef Q_WS_WIN
	win->tvGameList->header()->setContextMenuPolicy (Qt::CustomContextMenu);
	connect(win->tvGameList->header(), SIGNAL(customContextMenuRequested(const QPoint &)), 
			this, SLOT(showHeaderContextMenu(const QPoint &)));
	connect(headerMenu, SIGNAL(aboutToShow()), this, SLOT(updateHeaderContextMenu()));
	connect(win->menuCustomizeFields, SIGNAL(aboutToShow()), this, SLOT(updateHeaderContextMenu()));
#endif
}

void Gamelist::showContextMenu(const QPoint &p)
{
    menu->popup(win->tvGameList->mapToGlobal(p));
}

void Gamelist::updateContextMenu()
{
	if (currentGame.isEmpty())
	{
		win->actionPlay->setEnabled(false);
		return;
	}

	QString gameName = currentGame;
	GameInfo *gameInfo = mamegame->gamenameGameInfoMap[gameName];

	QPixmap pm;
	pm.loadFromData(gameInfo->icondata, "ico");
	QIcon icon(pm);

	win->actionPlay->setEnabled(true);
	win->actionPlay->setIcon(icon);
    win->actionPlay->setText(tr("Play %1").arg(gameInfo->description));

	win->actionSrcProperties->setText(tr("Properties for %1").arg(gameInfo->sourcefile));
}

void Gamelist::showHeaderContextMenu(const QPoint &p)
{
	QHeaderView *header = win->tvGameList->header();

	int currCol = header->logicalIndexAt(p);
	int sortCol = header->sortIndicatorSection();

	if (currCol != sortCol)
	{
		win->actionColSortAscending->setChecked(false);
		win->actionColSortDescending->setChecked(false);
	}
	else
	{
		if (header->sortIndicatorOrder() == Qt::AscendingOrder)
			win->actionColSortAscending->setChecked(true);
		else
			win->actionColSortDescending->setChecked(true);
	}
	
    headerMenu->popup(win->tvGameList->mapToGlobal(p));
}

void Gamelist::updateHeaderContextMenu()
{
	QHeaderView *header = win->tvGameList->header();

	win->actionColDescription->setChecked(header->isSectionHidden(0) ? false : true);
	win->actionColName->setChecked(header->isSectionHidden(1) ? false : true);
	win->actionColROMs->setChecked(header->isSectionHidden(2) ? false : true);
	win->actionColManufacturer->setChecked(header->isSectionHidden(3) ? false : true);
	win->actionColDriver->setChecked(header->isSectionHidden(4) ? false : true);
	win->actionColYear->setChecked(header->isSectionHidden(5) ? false : true);
	win->actionColCloneOf->setChecked(header->isSectionHidden(6) ? false : true);
}

void Gamelist::loadListXmlStarted()
{
	win->log("DEBUG: Gamelist::loadListXmlStarted()");
}

void Gamelist::loadListXmlReadyReadStandardOutput()
{
	QProcess *proc = (QProcess *)sender();
	QString buf = proc->readAllStandardOutput();
	
	//mamep: remove windows endl
	buf.replace(QString("\r"), QString(""));
	
	numTotalGames += buf.count("<game name=");
	mameOutputBuf += buf;

	win->logStatus(QString(tr("Loading listxml: %1 games")).arg(numTotalGames));
}

void Gamelist::loadListXmlFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	QProcess *proc = (QProcess *)sender();
	QTime elapsedTime;
	elapsedTime = elapsedTime.addMSecs(loadTimer.elapsed());

	procMan->procMap.remove(proc);
	procMan->procCount--;
	loadProc = NULL;

	parse();
//	win->poplog("PRE load ini ()");

	//chain
	loadDefaultIni();
//	win->poplog("end of gamelist->loadFin()");
}

void Gamelist::loadDefaultIni()
{
	mamegame->mameDefaultIni = "";
	QStringList args;
	args << "-showconfig" << "-noreadconfig";
	//patch inipath for mameplus
	if (mameOpts.contains("language"))
		args << "-inipath" << ".;ini";

	loadProc = procMan->process(procMan->start(mame_binary, args, FALSE));
	connect(loadProc, SIGNAL(readyReadStandardOutput()), this, SLOT(loadDefaultIniReadyReadStandardOutput()));
	connect(loadProc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(loadDefaultIniFinished(int, QProcess::ExitStatus)));
}

void Gamelist::loadDefaultIniReadyReadStandardOutput()
{
	QProcess *proc = (QProcess *)sender();
	QString buf = proc->readAllStandardOutput();
	
	mamegame->mameDefaultIni += buf;
}

void Gamelist::loadDefaultIniFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	QProcess *proc = (QProcess *)sender();

	procMan->procMap.remove(proc);
	procMan->procCount--;
	loadProc = NULL;

	optUtils->loadDefault(mamegame->mameDefaultIni);
	optUtils->loadTemplate();

	//reload gamelist. this is a chained call from loadListXmlFinished()
	init(true, GAMELIST_INIT_FULL);
	win->log("end of gamelist->loadDefFin()");
}

// extract a rom from the merged file
void Gamelist::extractMerged(QString mergedFileName, QString fileName)
{
	QString command = "bin/7z";
	QStringList args;
	args << "e" << "-y" << mergedFileName << fileName <<"-o" + QDir::tempPath();
	currentTempROM = QDir::tempPath() + "/" + fileName;

	loadProc = procMan->process(procMan->start(command, args, FALSE));
	connect(loadProc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(extractMergedFinished(int, QProcess::ExitStatus)));
}

// call the emulator after the rom has been extracted
void Gamelist::extractMergedFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	QProcess *proc = (QProcess *)sender();

	procMan->procMap.remove(proc);
	procMan->procCount--;
	loadProc = NULL;

	runMame(true);
}

// delete the extracted rom
void Gamelist::runMergedFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	QProcess *proc = (QProcess *)sender();

	procMan->procMap.remove(proc);
	procMan->procCount--;
	loadProc = NULL;

	QFile file(currentTempROM);
	file.remove();
	currentTempROM.clear();
}

void Gamelist::parse()
{
	win->log("DEBUG: Gamelist::prep parse()");

	ListXMLHandler handler(0);
	QXmlSimpleReader reader;
	reader.setContentHandler(&handler);
	reader.setErrorHandler(&handler);

	QXmlInputSource *pxmlInputSource = new QXmlInputSource();
	pxmlInputSource->setData (mameOutputBuf);

	win->log("DEBUG: Gamelist::start parse()");
	
	switchProgress(numTotalGames, tr("Parsing listxml"));
	reader.parse(*pxmlInputSource);
	switchProgress(-1, "");

	GameInfo *gameInfo, *gameInfo0, *gameInfo2;
	foreach (QString gameName, mamegame->gamenameGameInfoMap.keys())
	{
		gameInfo = mamegame->gamenameGameInfoMap[gameName];

		// restore previous audit results
		if (mamegame0 && mamegame0->gamenameGameInfoMap.contains(gameName))
		{
			gameInfo0 = mamegame0->gamenameGameInfoMap[gameName];
			gameInfo->available = gameInfo0->available;
		}

		// update clone list
		if (!gameInfo->cloneof.isEmpty())
		{
			gameInfo2 = mamegame->gamenameGameInfoMap[gameInfo->cloneof];
			gameInfo2->clones.insert(gameName);
		}
	}

	// restore previous console audit
	if (mamegame0)
	{
		foreach (QString gameName, mamegame0->gamenameGameInfoMap.keys())
		{
			gameInfo0 = mamegame0->gamenameGameInfoMap[gameName];

			if (gameInfo0->isExtRom && 
				//the console is supported by current mame version
				mamegame->gamenameGameInfoMap.contains(gameInfo0->romof))
			{
				gameInfo = new GameInfo(mamegame);
				gameInfo->description = gameInfo0->description;
				gameInfo->isBios = false;
				gameInfo->isExtRom = true;
				gameInfo->romof = gameInfo0->romof;
				gameInfo->sourcefile = gameInfo0->sourcefile;
				gameInfo->available = 1;
				mamegame->gamenameGameInfoMap[gameName] = gameInfo;
			}
		}

		delete mamegame0;
	}

	delete pxmlInputSource;
	mameOutputBuf.clear();
}

void Gamelist::filterTimer()
{
	searchTimer->start(200);
	searchTimer->setSingleShot(true);
}

void Gamelist::filterRegExpChanged()
{
	// multiple space-separated keywords
	QString text = win->lineEditSearch->text();
	// do not search less than 2 Latin chars
	if (text.count() == 1 && text.at(0).unicode() < 0x3000 /* CJK symbols start */)
		return;

	visibleGames.clear();
	text.replace(utils->spaceRegex, "*");

	//fixme: doesnt use filterregexp
	static QRegExp regExp("");
	// set it for a callback to refresh the list
	gameListPModel->searchText = text;	// must set before regExp
	gameListPModel->setFilterRegExp(regExp);
	win->tvGameList->expandAll();
	restoreGameSelection();
}

void Gamelist::filterRegExpChanged2(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
	QString filterText;

	// the selection could be empty
	if (!current)
		return;

	currentFolder= "";
	if (win->treeFolders->currentItem()->parent())
		currentFolder += win->treeFolders->currentItem()->parent()->text(0);
	currentFolder += "/" + win->treeFolders->currentItem()->text(0);

	/* hack to speed up sorting and filtering, don't know why. need more investigation
	  * filtering game desc fleld is extremely lengthy
	  * previous == NULL means this call is not from a signal
	  */
	if ( previous != NULL &&
		(currentFolder == "/" + folderList[FOLDER_ALLGAME] || 
		currentFolder == "/" + folderList[FOLDER_CONSOLE] ||
		visibleGames.count() > 10000))
	{
		win->log("hack to reinit list");
		init(true, GAMELIST_INIT_DIR);
		return;
	}

	visibleGames.clear();

	// update menu text
	QString folder;
	if (utils->isConsoleFolder())
		folder = currentFolder;
	else
		folder = tr("All Arcades");
	win->actionRefresh->setText(tr("Refresh").append(": ").append(folder));

	if (!current->parent())
	{
		QString folderName = current->text(0);
		filterText = "";

		if (folderName == folderList[FOLDER_ALLGAME])
			gameListPModel->setFilterRole(Qt::UserRole + FOLDER_ALLGAME);
		else if (folderName == folderList[FOLDER_ALLARC])
			gameListPModel->setFilterRole(Qt::UserRole + FOLDER_ALLARC);
		else if (folderName == folderList[FOLDER_AVAILABLE])
			gameListPModel->setFilterRole(Qt::UserRole + FOLDER_AVAILABLE);
		else if (folderName == folderList[FOLDER_UNAVAILABLE])
			gameListPModel->setFilterRole(Qt::UserRole + FOLDER_UNAVAILABLE);
		else if (folderName == folderList[FOLDER_CONSOLE])
			gameListPModel->setFilterRole(Qt::UserRole + FOLDER_CONSOLE);
		else if (folderName == folderList[FOLDER_BIOS])
			gameListPModel->setFilterRole(Qt::UserRole + FOLDER_BIOS);
		else
			gameListPModel->setFilterRole(Qt::UserRole + FOLDER_ALLGAME);
	}
	else
	{
		QString folderName = current->parent()->text(0);
		filterText = current->text(0);

		if (folderName == folderList[FOLDER_CONSOLE])
			gameListPModel->setFilterRole(Qt::UserRole + FOLDER_CONSOLE + MAX_FOLDERS);	//hack for console subfolders
		else if (folderName == folderList[FOLDER_MANUFACTURER])
			gameListPModel->setFilterRole(Qt::UserRole + FOLDER_MANUFACTURER);
		else if (folderName == folderList[FOLDER_YEAR])
			gameListPModel->setFilterRole(Qt::UserRole + FOLDER_YEAR);
		else if (folderName == folderList[FOLDER_SOURCE])
			gameListPModel->setFilterRole(Qt::UserRole + FOLDER_SOURCE);
		else if (folderName == folderList[FOLDER_BIOS])
			gameListPModel->setFilterRole(Qt::UserRole + FOLDER_BIOS + MAX_FOLDERS);	//hack for bios subfolders
		//fixme
		else
			gameListPModel->setFilterRole(Qt::UserRole + FOLDER_ALLGAME);
	}

	static QRegExp regExp("");
	// set it for a callback to refresh the list
	gameListPModel->filterText = filterText;	// must set before regExp
	gameListPModel->setFilterRegExp(regExp);

	//fixme: must have this, otherwise the list cannot be expanded properly
	qApp->processEvents();
	win->tvGameList->expandAll();
	restoreGameSelection();
}

void Gamelist::initFolders()
{
	folderList
		<< tr("All Games")
		<< tr("All Arcades")
		<< tr("Available Arcades")
		<< tr("Unavailable Arcades")
		<< tr("Consoles")
		<< tr("Manufacturer")
		<< tr("Year")
		<< tr("Driver")
		<< tr("BIOS")
		/*
		<< QT_TR_NOOP("CPU")
		<< QT_TR_NOOP("Sound")
		<< QT_TR_NOOP("Orientation")
		<< QT_TR_NOOP("Emulation Status")
		<< QT_TR_NOOP("Dumping Status")
		<< QT_TR_NOOP("Working")
		<< QT_TR_NOOP("Not working")
		<< QT_TR_NOOP("Orignals")
		<< QT_TR_NOOP("Clones")
		<< QT_TR_NOOP("Raster")
		<< QT_TR_NOOP("Vector")
		<< QT_TR_NOOP("Resolution")
		<< QT_TR_NOOP("FPS")
		<< QT_TR_NOOP("Save State")
		<< QT_TR_NOOP("Control Type")
		<< QT_TR_NOOP("Stereo")
		<< QT_TR_NOOP("CHD")
		<< QT_TR_NOOP("Samples")
		<< QT_TR_NOOP("Artwork")*/
		;

	QStringList consoleList, mftrList, yearList, srcList, biosList;
	GameInfo *gameInfo;
	foreach (QString gameName, mamegame->gamenameGameInfoMap.keys())
	{
		gameInfo = mamegame->gamenameGameInfoMap[gameName];
		if (!gameInfo->nameDeviceInfoMap.isEmpty())
			consoleList << gameName;

		if (gameInfo->isBios)
			biosList << gameName;

		if (!mftrList.contains(gameInfo->manufacturer))
			mftrList << gameInfo->manufacturer;

		QString year = gameInfo->year;
		if (year.isEmpty())
			year = "?";
		if (!yearList.contains(year))
			yearList << year;

		if (!srcList.contains(gameInfo->sourcefile))
			srcList << gameInfo->sourcefile;
	}

	consoleList.sort();
	mftrList.sort();
	yearList.sort();
	srcList.sort();
	biosList.sort();
	
	QIcon icoFolder(":/res/32x32/folder.png");
	QList<QTreeWidgetItem *> items;
	for (int i = 0; i < folderList.size(); i++)
	{
		items.append(new QTreeWidgetItem(win->treeFolders, QStringList(folderList[i])));

		win->treeFolders->insertTopLevelItems(0, items);
		items[i]->setIcon(0, icoFolder);

		if (i == FOLDER_CONSOLE)
			foreach (QString name, consoleList)
				items[i]->addChild(new QTreeWidgetItem(items[i], QStringList(name)));

		else if (i == FOLDER_MANUFACTURER)
			foreach (QString name, mftrList)
				items[i]->addChild(new QTreeWidgetItem(items[i], QStringList(name)));

		else if (i == FOLDER_YEAR)
			foreach (QString name, yearList)
				items[i]->addChild(new QTreeWidgetItem(items[i], QStringList(name)));

		else if (i == FOLDER_SOURCE)
			foreach (QString name, srcList)
				items[i]->addChild(new QTreeWidgetItem(items[i], QStringList(name)));

		else if (i == FOLDER_BIOS)
			foreach (QString name, biosList)
			{
				GameInfo *gameInfo = mamegame->gamenameGameInfoMap[name];
/*				QPixmap pm;
				pm.loadFromData(gameInfo->icondata, "ico");
				QIcon icon(pm);
*/
				QTreeWidgetItem *item = new QTreeWidgetItem(items[i], QStringList(name));
				item->setToolTip(0, gameInfo->description);
//				item->setIcon(0, icon);

				items[i]->addChild(item);
			}
	}

	connect(win->treeFolders, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
		this, SLOT(filterRegExpChanged2(QTreeWidgetItem *, QTreeWidgetItem *)));
}

void Gamelist::restoreFolderSelection()
{
	//if currentFolder has been set, it's a folder switching call
	if(!currentFolder.isEmpty())
		return;

	currentFolder = guiSettings.value("folder_current", "/" + folderList[0]).toString();
	int sep = currentFolder.indexOf("/");
	QString parentFolder = currentFolder.left(sep);
	QString subFolder = currentFolder.right(currentFolder.size() - sep - 1);

	if (parentFolder.isEmpty())
	{
		parentFolder = subFolder;
		subFolder = "";
	}

	QTreeWidgetItem *rootItem = win->treeFolders->invisibleRootItem();

	for (int i = 0; i < rootItem->childCount(); i++)
	{
		QTreeWidgetItem *subItem = rootItem->child(i);

		if (subItem->text(0) == parentFolder)
		{
			win->log(QString("tree lv1.gamecount %1").arg(mamegame->gamenameGameInfoMap.count()));
			if (subFolder.isEmpty())
			{
				win->treeFolders->setCurrentItem(subItem);
				return;
			}
			else
			{
				subItem->setExpanded(true);
				for (int j = 0; j < subItem->childCount(); j++)
				{
					QTreeWidgetItem *subsubItem = subItem->child(j);
					if (subsubItem->text(0) == subFolder)
					{
						win->treeFolders->setCurrentItem(subsubItem);
						win->log(QString("treeb.gamecount %1").arg(mamegame->gamenameGameInfoMap.count()));
						return;
					}
				}
			}
		}
	}
	//fallback
	win->treeFolders->setCurrentItem(rootItem->child(FOLDER_ALLARC));
}

void Gamelist::runMame(bool runMerged)
{
	//block multi mame session for now
	//if (procMan->procCount > 0)
	//	return;

	if (currentGame.isEmpty())
		return;

	QStringList args;

	GameInfo *gameInfo = mamegame->gamenameGameInfoMap[currentGame];

	// run console roms, add necessary params
	if (gameInfo->isExtRom)
	{
		// console system
		args << gameInfo->romof;
		// console device
		args << "-cart";

		QStringList paths = currentGame.split(".7z/");
		// run extracted rom
		if (runMerged)
		{
			// use temp rom name instead
			args << currentTempROM;
			loadProc = procMan->process(procMan->start(mame_binary, args, FALSE));
			// delete the extracted rom
			connect(loadProc, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(runMergedFinished(int, QProcess::ExitStatus)));
			return;
		}

		// extract merged rom
		if (currentGame.contains(".7z/"))
		{
			extractMerged(paths[0] + ".7z", paths[1]);
			return;
		}
	}

	args << currentGame;

	if (mameOpts.contains("language"))
		args << "-language" << language;

	procMan->process(procMan->start(mame_binary, args));
}


GameListSortFilterProxyModel::GameListSortFilterProxyModel(QObject *parent)
: QSortFilterProxyModel(parent)
{
}

bool GameListSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	bool result = true;
	bool isExtRom, isBIOS, isConsole, isSFZCH;
	QModelIndex indexGameDesc, indexGameName, index2;
	const QAbstractItemModel *srcModel = sourceModel();

	indexGameDesc = srcModel->index(sourceRow, COL_DESC, sourceParent);
	indexGameName = srcModel->index(sourceRow, COL_NAME, sourceParent);
	
	GameInfo *gameInfo = NULL;
	QString gameName = srcModel->data(indexGameName).toString();
	QString gameNamex = srcModel->data(indexGameName, Qt::UserRole).toString();

	if (mamegame->gamenameGameInfoMap.contains(gameName))
		gameInfo = mamegame->gamenameGameInfoMap[gameName];

	isSFZCH = gameName == "sfzch";
	isConsole = isSFZCH || gameInfo && !gameInfo->nameDeviceInfoMap.isEmpty();
	isBIOS = gameInfo->isBios;
	isExtRom = srcModel->data(indexGameDesc, Qt::UserRole + FOLDER_CONSOLE).toBool();

	// filter out BIOS and Console systems
	if (isConsole && !isExtRom && !isSFZCH)
		return false;

	if (!searchText.isEmpty())
	{
		QRegExp::PatternSyntax syntax = QRegExp::PatternSyntax(QRegExp::Wildcard);	
		QRegExp regExpSearch(searchText, Qt::CaseInsensitive, syntax);

		// apply search filter
		// return if any of a parent's clone matches
/*		if (!gameInfo->cloneof.isEmpty())
		{
			GameInfo *gameInfo2 = mamegame->gamenameGameInfoMap[gameInfo->cloneof];
//			gameInfo2->clones.contains(regExpSearch);

//			result = srcModel->data(indexGameDesc).toString().contains(regExpSearch) || 
//				gameName.contains(regExpSearch);
		}
*/
		result = srcModel->data(indexGameDesc).toString().contains(regExpSearch) || 
				 gameName.contains(regExpSearch);
	}

	const int role = filterRole();
	switch (role)
	{
	// apply folder filter
	case Qt::UserRole + FOLDER_ALLARC:
		result = result && !isExtRom && !isConsole && !isBIOS;
		break;

	case Qt::UserRole + FOLDER_AVAILABLE:
		result = result && gameInfo->available == 1
						&& !isExtRom && !isConsole && !isBIOS;
		break;

	case Qt::UserRole + FOLDER_UNAVAILABLE:
		index2 = srcModel->index(sourceRow, COL_ROM, sourceParent);
		result = result && srcModel->data(index2).toString() != "Yes"
						&& !isExtRom && !isConsole && !isBIOS;
		break;

	case Qt::UserRole + FOLDER_CONSOLE:
		result = result && isExtRom && !isBIOS;
		break;

	case Qt::UserRole + FOLDER_CONSOLE + MAX_FOLDERS:	//hack for console subfolders
		result = result && srcModel->data(indexGameName).toString() == filterText
						&& !isBIOS;
		break;

	case Qt::UserRole + FOLDER_MANUFACTURER:
		result = result && gameInfo->manufacturer == filterText && !isBIOS;
		break;

	case Qt::UserRole + FOLDER_YEAR:
	{
		QString year = gameInfo->year;
		if (year.isEmpty())
			year = "?";
		result = result && year == filterText && !isBIOS;
		break;
	}
	case Qt::UserRole + FOLDER_SOURCE:
		result = result && gameInfo->sourcefile == filterText;
		break;

	case Qt::UserRole + FOLDER_BIOS:
		result = result && isBIOS;
		break;

	case Qt::UserRole + FOLDER_BIOS + MAX_FOLDERS:	//hack for bios subfolders
		result = result && srcModel->data(indexGameDesc, Qt::UserRole + FOLDER_BIOS).toString() == filterText
						&& !isBIOS;
		break;

	case Qt::UserRole + FOLDER_ALLGAME:
		result = result && !isBIOS;
		break;

	default:
		// empty list otherwise
		result = false;
	}

	if (result)
		visibleGames.insert(isExtRom ? gameNamex : gameName);

	return result;
}

