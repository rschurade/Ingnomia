/*	
	This file is part of Ingnomia https://github.com/rschurade/Ingnomia
    Copyright (C) 2017-2020  Ralph Schurade, Ingnomia Team

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#pragma once

#include <QObject>
#include <QVariantMap>

struct StartingItem
{
	QString itemSID;
	QString mat1;
	QString mat2;
	int amount = 1;
};

struct StartingAnimal
{
	QString type;
	QString gender;
	int amount = 1;
};

struct CheckableItem
{
	QString sid;
	QString name;
	QString type;
	bool isChecked;
	int max;
};

class NewGameSettings : public QObject
{
	Q_OBJECT

public:
	NewGameSettings( QObject* parent = 0 );
	~NewGameSettings();

	void save();

	void setRandomName();
	void setRandomSeed();

	bool setKingdomName( QString value );
	bool setSeed( QString value );

	QString kingdomName()
	{
		return m_kingdomName;
	}
	QString seed()
	{
		return m_seed;
	}

	int worldSize()
	{
		return m_worldSize;
	}
	int zLevels()
	{
		return m_zLevels;
	}
	int ground()
	{
		return m_ground;
	}
	int flatness()
	{
		return m_flatness;
	}
	int oceanSize()
	{
		return m_oceanSize;
	}
	int rivers()
	{
		return m_rivers;
	}
	int riverSize()
	{
		return m_riverSize;
	}
	 int numGnomes()
	{
		return m_numGnomes;
	}
	int startZone()
	{
		return m_startZone;
	}
	int treeDensity()
	{
		return m_treeDensity;
	}
	int plantDensity()
	{
		return m_plantDensity;
	}
	bool isPeaceful()
	{
		return m_isPeaceful;
	}
	int globalMaxPerType()
	{
		return m_maxPerType;
	}
	int numWildAnimals()
	{
		return m_numWildAnimals;
	}
	int maxAnimalsPerType( QString type );
	

	bool setWorldSize( int value );
	bool setZLevels( int value );
	bool setGround( int value );
	bool setFlatness( int value );
	bool setOceanSize( int value );
	bool setRivers( int value );
	bool setRiverSize( int value );
	bool setNumGnomes( int value );
	bool setStartZone( int value );
	bool setTreeDensity( int value );
	bool setPlantDensity( int value );
	bool setPeaceful( bool value );
	bool setMaxPerType( int value );
	bool setNumWildAnimals( int value );

	void materialsForItem( QString item, QStringList& mats1, QStringList& mats2 );
	void addStartingItem( QString itemSID, QString mat1, QString mat2, int amount );
	void removeStartingItem( QString tag );

	void addStartingAnimal( QString type, QString gender, int amount );
	void removeStartingAnimal( QString tag );

	QList<StartingItem> startingItems()
	{
		return m_startingItems;
	}
	QList<StartingAnimal> startingAnimals()
	{
		return m_startingAnimals;
	}

	QVariantList trees();
	QVariantList plants();
	QVariantList animals();

	void setPreset( QString name );
	QStringList presetNames();
	QString addPreset();
	bool removePreset( QString name );
	bool savePreset( QVariantList items );
	bool onSavePreset();

	bool isChecked( QString sid );
	void setChecked( QString sid, bool value );

	void setAmount( QString sid, int value );

private:
	void loadEmbarkMap();
	void loadPresets();
	void saveUserPresets();

	void setStartingItems( QVariantList sil );
	void collectStartItems( QVariantList& sil );


	QString m_kingdomName;
	QString m_seed;

	int m_worldSize      = 100;
	int m_zLevels        = 130;
	int m_ground         = 100;
	int m_flatness       = 0;
	int m_oceanSize      = 0;
	int m_rivers         = 1;
	int m_riverSize      = 3;
	int m_numGnomes      = 8;
	int m_startZone      = 10;
	int m_treeDensity    = 50;
	int m_plantDensity   = 50;
	int m_numWildAnimals = 500;
	int m_maxPerType     = 1000;

	bool m_isPeaceful = true;

	QList<StartingItem> m_startingItems;
	QList<StartingAnimal> m_startingAnimals;

	QStringList materials( QString itemSID );

	QVariantList m_standardPresets;
	QList<QVariantMap> m_userPresets;

	QString m_selectedPreset;

	QMap<QString, CheckableItem> m_checkableItems;
};
