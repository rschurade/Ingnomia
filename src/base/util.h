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

#include "../base/position.h"
#include "SDL_util.h"
#include "dbstructs.h"

#include <QGridLayout>
#include <QLayoutItem>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QToolButton>
#include <QVariant>
#include <QtGlobal>

class QComboBox;

class Game;
class ObjectItem;
struct Tile;

class Util
{
public:
	Util( Game* game );
	~Util();

	QStringList seedItems( QString plantType, QString material = "any" );

	QString itemName( QString itemID );
	QString materialName( QString materialID );

	QString materialType( QString materialID );

	std::string requiredSkill( const std::string& jobID );
	std::string requiredMagicSkill( const std::string& spellID );
	std::string requiredTool( const std::string& jobID );
	int requiredToolLevel( const std::string& jobID, Position pos );
	int requiredToolLevelByWallMaterial( Position pos );
	int requiredToolLevelByFloorMaterial( Position pos );
	int toolLevel( unsigned int itemUID );
	int toolLevel( QString materialSID );

	absl::btree_set<QString> itemsAllowedInContainer( unsigned int container );
	bool itemAllowedInContainer( unsigned int item, unsigned int container );
	void initAllowedInContainer();
	QString carryContainerForItem( QString itemSID );
	int capacity( QString containerSID );

	void addFloatToVariant( QVariantMap& vm, QString key, float addValue );

	QString tile2String( Tile& tile, QString seperator = "_" );
	void string2Tile( Tile& tile, QString data, QString seperator = "_" );
	QString mapJoin( absl::btree_map<int, int>, QString seperator = "_" );
	absl::btree_map<int, int> mapSplit( QString data, QString seperator = "_" );

	QPair<int, int> pixelOffset( const int x, const int y, const int z );
	QPair<int, int> pixelOffset( const Position& pos );

	unsigned char rotString2Char( QString rot );

	QColor colorInt2Color( unsigned int color );
	unsigned int string2Color( QString color );
	QColor string2QColor( QString color );
	QColor variant2QColor( QVariant color );

	QString randomMetal();
	QString randomMetalSliver( QString sourceMaterial );

	int ticksPerMinute = 10;
	int minutesPerHour = 60;
	int hoursPerDay = 24;
	int ticksPerDay = 14400;
	int daysPerSeason = 12;

	int ticksPerDayRandomized( int percentage );

	quint64 nextSeasonChangeTick();

	unsigned int createRawMaterialItem( Position pos, unsigned int materialID );

	unsigned int reverseFib( unsigned int number );

	Position reachableBorderPos( Position pos, bool& found );
	Position borderPos( bool& found );

	QList<Position> neighbors8( Position pos );

	QToolButton* createToolButton( QIcon icon, QString text, QString toolTip );

	QString getRandomString( int length );

	static QVariantList uintList2Variant( const QList<unsigned int>& list );
	QList<unsigned int> variantList2UInt( const QVariantList& vlist );

	QVariantList positionList2Variant( const QList<Position>& list );
	QList<Position> variantList2Position( const QVariantList& vlist );

	QVariantList pairList2Variant( const QList<QPair<QString, QString>>& plist );
	QList<QPair<QString, QString>> variantList2Pair( const QVariantList& plist );

	void setIndexFromText( QComboBox* cb, QString text );

	QString addDyeMaterial( QString sourceMaterial, QString dyeMaterial );

	void debugVM( QVariantMap vm, QString name );

	PixmapPtr smallPixmap( Sprite* sprite, const std::string& season, int rotation );
	PixmapPtr createWorkshopImage( const std::string& workshopID, const std::vector<std::string>& m_mats );
	PixmapPtr createItemImage( const std::string& constructionID, const std::vector<std::string>& mats );
	PixmapPtr createItemImage2( const std::string& constructionID, const std::vector<std::string>& mats );
	PixmapPtr createConstructionImage( const std::string& constructionID, const std::vector<std::string>& mats );
	Sprite* getSprite( int x, int y, const std::vector<DBS::Workshop_Component>& comps, unsigned char& rot, const std::vector<std::string>& mats );
	Sprite* getSprite( int x, int y, const std::vector<DBS::Constructions_Sprites>& comps, unsigned char& rot, const std::vector<std::string>& mats );

	void createBufferForNoesisImage( const SDL_Surface* pm, std::vector<unsigned char>& buffer );

	QStringList possibleMaterialsForItem( QString itemSID );
	QStringList possibleMaterials( QString allowedMaterials, QString allowedMaterialTypes );

private:
	QPointer<Game> g;
};
