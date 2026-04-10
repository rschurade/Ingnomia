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
/** @file aggregatoragri.h
 *  @brief Data transfer types and aggregator that collates farm/pasture/grove state for the
 *         Agriculture XAML window. Lives on the GUI side and marshals game-thread data into
 *         Qt signals the Noesis view models can consume.
 */
#pragma once

#include "../base/tile.h"
#include "../game/creature.h"

#include <QObject>

class Game;

/// @brief Summary of a single animal species for global animal counts.
struct GuiAnimal
{
	QString name;           ///< Localised display name.
	QString animalID;       ///< Animal species string ID.
	QString materialID;     ///< Usually equal to animalID.
	QString harvestedItem;  ///< Item produced when butchered.
	int totalcount  = 0;    ///< Total live individuals of this species.
	int maleCount   = 0;    ///< Number of males.
	int femaleCount = 0;    ///< Number of females.
	QPixmap sprite;         ///< Preview sprite for the GUI.
};
Q_DECLARE_METATYPE( GuiAnimal )

/// @brief One animal currently assigned to a pasture, as shown in the pasture roster.
struct GuiPastureAnimal
{
	QString name;                      ///< Display name.
	unsigned int id = 0;               ///< Creature UID.
	QString animalID;                  ///< Species string ID.
	QString materialID;                ///< Usually equal to animalID.
	Gender gender = Gender::UNDEFINED; ///< Male / female / undefined.
	bool isYoung = false;              ///< True for non-adult animals.
	bool toButcher = false;            ///< True if the animal is flagged for butchering.
};
Q_DECLARE_METATYPE( GuiPastureAnimal )


/// @brief Plant species summary for farm/grove product lists.
struct GuiPlant
{
	QString name;           ///< Localised display name.
	QString plantID;        ///< Plant species string ID.
	QString seedID;         ///< Seed item string ID.
	int seedCount = 0;      ///< Available seeds in stockpiles.
	QString materialID;     ///< Usually equal to plantID.
	QString harvestedItem;  ///< Item harvested from this plant.
	int itemCount  = 0;     ///< Count of harvested items in storage.
	int plantCount = 0;     ///< Live plants of this species on the map.
	QPixmap sprite;         ///< Preview sprite for the GUI.
};
Q_DECLARE_METATYPE( GuiPlant )

/// @brief Discriminator for the three kinds of agricultural designation.
enum class AgriType
{
	Farm,    ///< Tilled plots for crops.
	Pasture, ///< Fenced area for animals.
	Grove    ///< Planted tree area.
};
Q_DECLARE_METATYPE( AgriType )

/// @brief Full state of one farm designation as shown in the Agriculture GUI.
struct GuiFarmInfo
{
	unsigned int ID = 0;    ///< Farm designation UID.

	QString name;           ///< Display name.
	int priority    = 1;    ///< Current priority index.
	int maxPriority = 1;    ///< Priority range upper bound.
	bool suspended  = false;///< True if work on this farm is paused.
	bool harvest    = true; ///< True if crops should be harvested.
	QString plantType;      ///< Selected crop plant ID.

	int numPlots  = 0;      ///< Total tiles in this farm.
	int tilled    = 0;      ///< Tiles currently tilled.
	int planted   = 0;      ///< Tiles currently planted.
	int cropReady = 0;      ///< Tiles with ready-to-harvest crops.

	GuiPlant product;       ///< Current crop product details.
};
Q_DECLARE_METATYPE( GuiFarmInfo )

/// @brief Food type allowed in a pasture, with allow/deny checkbox state.
struct GuiPastureFoodItem
{
	QString itemSID;        ///< Food item string ID.
	QString materialSID;    ///< Food material string ID.
	QString name;           ///< Localised name.
	bool checked = false;   ///< True if the pasture accepts this food.
	QPixmap sprite;         ///< Preview sprite.
};

/// @brief Full state of one pasture designation as shown in the Agriculture GUI.
struct GuiPastureInfo
{
	unsigned int ID = 0;     ///< Pasture designation UID.

	QString name;            ///< Display name.
	int priority    = 1;     ///< Current priority index.
	int maxPriority = 1;     ///< Priority range upper bound.
	bool suspended  = false; ///< True if work on this pasture is paused.
	bool harvest    = true;  ///< True if animals should be harvested for products.
	bool harvestHay = false; ///< True if hay should be harvested from grass.
	bool tame		= false; ///< True if wild animals should be tamed.
	QString animalType;      ///< Selected species string ID.

	int numPlots   = 0;      ///< Total tiles in this pasture.
	int numMale    = 0;      ///< Current number of male animals.
	int numFemale  = 0;      ///< Current number of female animals.
	int total      = 0;      ///< Total animals currently housed.
	int maxNumber  = 0;      ///< Maximum animals the pasture can hold.
	int maxMale    = 0;      ///< Upper cap on males (user setting).
	int maxFemale  = 0;      ///< Upper cap on females (user setting).
	int animalSize = 0;      ///< Tiles occupied by a single animal.

	int foodMax    = 0;      ///< Food trough capacity.
	int foodCurrent= 0;      ///< Food currently in trough.
	int hayMax     = 0;      ///< Hay storage capacity.
	int hayCurrent = 0;      ///< Hay currently stored.

	QList<GuiPastureFoodItem> food; ///< Per-food-type allow list.

	GuiAnimal product;              ///< Current species product details.
	QList<GuiPastureAnimal> animals;///< Roster of individual animals in this pasture.
};
Q_DECLARE_METATYPE( GuiPastureInfo )

/// @brief Full state of one grove designation as shown in the Agriculture GUI.
struct GuiGroveInfo
{
	unsigned int ID = 0;      ///< Grove designation UID.

	QString name;             ///< Display name.
	int priority    = 1;      ///< Current priority index.
	int maxPriority = 1;      ///< Priority range upper bound.
	bool suspended  = false;  ///< True if work on this grove is paused.
	bool plantTrees = true;   ///< True if saplings should be planted.
	bool fellTrees  = false;  ///< True if mature trees should be felled.
	bool pickFruits = true;   ///< True if fruit should be picked.
	QString treeType;         ///< Selected tree species string ID.

	int numPlots  = 0;        ///< Total tiles in this grove.
	int planted   = 0;        ///< Tiles currently planted.
	int cropReady = 0;        ///< Tiles with ready fruit/wood.

	GuiPlant product;         ///< Current tree product details.
};
Q_DECLARE_METATYPE( GuiGroveInfo )

/// @brief Bridges the Agriculture XAML window with the game-side farm/pasture/grove managers.
///        Handles user actions (priority, product, harvest toggles, butchering) and emits
///        refreshed Gui*Info payloads back to the view models.
class AggregatorAgri : public QObject
{
	Q_OBJECT

public:
	AggregatorAgri( QObject* parent = nullptr );
	~AggregatorAgri();

	void init( Game* game );

private:
	QPointer<Game> g;                       ///< Game instance (weak ownership).

	bool m_AgriDirty             = false;   ///< Unused dirty flag reserved for batch updates.
	unsigned int m_currentTileID = 0;       ///< Currently selected designation tile.
	AgriType m_currentType       = AgriType::Farm; ///< Kind of designation currently shown.
	GuiFarmInfo m_farmInfo;                 ///< Cached farm state for the open window.
	GuiPastureInfo m_pastureInfo;           ///< Cached pasture state for the open window.
	GuiGroveInfo m_groveInfo;               ///< Cached grove state for the open window.
	QList<GuiPlant> m_globalPlantInfo;      ///< Global plant availability list.
	QList<GuiAnimal> m_globalAnimalInfo;    ///< Global animal availability list.
	QList<GuiPlant> m_globalTreeInfo;       ///< Global tree availability list.

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