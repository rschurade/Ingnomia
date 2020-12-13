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

#include "../base/tile.h"
#include "../game/creature.h"

#include <QObject>

class Game;

struct GuiAnimal
{
	QString name;
	QString animalID;
	QString materialID; // = animalID usually
	QString harvestedItem;
	int totalcount  = 0;
	int maleCount   = 0;
	int femaleCount = 0;
	QPixmap sprite;
};
Q_DECLARE_METATYPE( GuiAnimal )

struct GuiPastureAnimal
{
	QString name;
	unsigned int id = 0;
	QString animalID;
	QString materialID; // = animalID usually
	Gender gender = Gender::UNDEFINED;
	bool isYoung = false;
	bool toButcher = false;
};
Q_DECLARE_METATYPE( GuiPastureAnimal )


struct GuiPlant
{
	QString name;
	QString plantID;
	QString seedID;
	int seedCount = 0;
	QString materialID; // = plantID usually
	QString harvestedItem;
	int itemCount  = 0;
	int plantCount = 0;
	QPixmap sprite;
};
Q_DECLARE_METATYPE( GuiPlant )

enum class AgriType
{
	Farm,
	Pasture,
	Grove
};
Q_DECLARE_METATYPE( AgriType )

struct GuiFarmInfo
{
	unsigned int ID = 0;

	QString name;
	int priority    = 1;
	int maxPriority = 1;
	bool suspended  = false;
	bool harvest    = true;
	QString plantType;

	int numPlots  = 0;
	int tilled    = 0;
	int planted   = 0;
	int cropReady = 0;

	GuiPlant product;
};
Q_DECLARE_METATYPE( GuiFarmInfo )

struct GuiPastureFoodItem
{
	QString itemSID;
	QString materialSID;
	QString name;
	bool checked = false;
	QPixmap sprite;
};

struct GuiPastureInfo
{
	unsigned int ID = 0;

	QString name;
	int priority    = 1;
	int maxPriority = 1;
	bool suspended  = false;
	bool harvest    = true;
	bool harvestHay = false;
	bool tame		= false;
	QString animalType;

	int numPlots   = 0;
	int numMale    = 0;
	int numFemale  = 0;
	int total      = 0;
	int maxNumber  = 0;
	int maxMale    = 0;
	int maxFemale  = 0;
	int animalSize = 0;

	int foodMax    = 0;
	int foodCurrent= 0;
	int hayMax     = 0;
	int hayCurrent = 0;

	QList<GuiPastureFoodItem> food;

	GuiAnimal product;
	QList<GuiPastureAnimal> animals;
};
Q_DECLARE_METATYPE( GuiPastureInfo )

struct GuiGroveInfo
{
	unsigned int ID = 0;

	QString name;
	int priority    = 1;
	int maxPriority = 1;
	bool suspended  = false;
	bool plantTrees = true;
	bool fellTrees  = false;
	bool pickFruits = true;
	QString treeType;

	int numPlots  = 0;
	int planted   = 0;
	int cropReady = 0;

	GuiPlant product;
};
Q_DECLARE_METATYPE( GuiGroveInfo )

class AggregatorAgri : public QObject
{
	Q_OBJECT

public:
	AggregatorAgri( QObject* parent = nullptr );
	~AggregatorAgri();

	void init( Game* game );

private:
	QPointer<Game> g;

	bool m_AgriDirty             = false;
	unsigned int m_currentTileID = 0;
	AgriType m_currentType       = AgriType::Farm;
	GuiFarmInfo m_farmInfo;
	GuiPastureInfo m_pastureInfo;
	GuiGroveInfo m_groveInfo;
	QList<GuiPlant> m_globalPlantInfo;
	QList<GuiAnimal> m_globalAnimalInfo;
	QList<GuiPlant> m_globalTreeInfo;

public slots:
	void onOpen( TileFlag designation, unsigned int tileID );
	void onUpdate( unsigned int id );
	void onUpdateFarm( unsigned int id );
	void onUpdatePasture( unsigned int id );
	void onUpdateGrove( unsigned int id );

	void onSetBasicOptions( AgriType type, unsigned int ID, QString name, int priority, bool suspended );
	void onSelectProduct( AgriType type, unsigned int designationID, QString productSID );
	void onSetHarvestOptions( AgriType type, unsigned int designationID, bool harvest, bool harvestHay, bool tame );
	void onSetGroveOptions( unsigned int groveID, bool pick, bool plant, bool fell );

	void onSetMaxMale( unsigned int pastureID, int max );
	void onSetMaxFemale( unsigned int pastureID, int max );

	void onRequestGlobalPlantInfo();
	void onRequestGlobalAnimalInfo();
	void onRequestGlobalTreeInfo();
	void onRequestProductInfo( AgriType type, unsigned int designationID );

	void onCloseWindow();

	void onSetButchering( unsigned int animalID, bool value );
	void onRequestPastureAnimalInfo( unsigned int pastureID );
	void onRequestPastureFoodInfo( unsigned int pastureID );

	void onSetFoodItemChecked( unsigned int pastureID, QString itemSID, QString materialSID, bool checked );

signals:
	void signalShowAgri( unsigned int id );
	void signalUpdateFarm( const GuiFarmInfo& info );
	void signalUpdatePasture( const GuiPastureInfo& info );
	void signalUpdateGrove( const GuiGroveInfo& info );
	void signalGlobalPlantInfo( const QList<GuiPlant>& info );
	void signalGlobalAnimalInfo( const QList<GuiAnimal>& info );
	void signalGlobalTreeInfo( const QList<GuiPlant>& info );
};