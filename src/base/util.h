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

#include <QGridLayout>
#include <QLayoutItem>
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <QToolButton>
#include <QVariant>
#include <QtGlobal>

class QComboBox;

class ObjectItem;
struct Tile;

class Util
{
public:
	static QStringList seedItems( QString plantType, QString material = "any" );

	static QString itemName( QString itemID );
	static QString materialName( QString materialID );

	static QString materialType( QString materialID );

	static QString requiredSkill( QString jobID );
	static QString requiredMagicSkill( QString spellID );
	static QString requiredTool( QString jobID );
	static int requiredToolLevel( QString jobID, Position pos );
	static int requiredToolLevelByWallMaterial( Position pos );
	static int requiredToolLevelByFloorMaterial( Position pos );
	static int toolLevel( unsigned int itemUID );
	static int toolLevel( QString materialSID );

	static QSet<QString> itemsAllowedInContainer( unsigned int container );
	static bool itemAllowedInContainer( unsigned int item, unsigned int container );
	static void initAllowedInContainer();
	static QString carryContainerForItem( QString itemSID );
	static int capacity( QString containerSID );

	static void addFloatToVariant( QVariantMap& vm, QString key, float addValue );

	static QString tile2String( Tile& tile, QString seperator = "_" );
	static void string2Tile( Tile& tile, QString data, QString seperator = "_" );
	static QString mapJoin( QMap<int, int>, QString seperator = "_" );
	static QMap<int, int> mapSplit( QString data, QString seperator = "_" );

	static QPair<int, int> pixelOffset( const int x, const int y, const int z );
	static QPair<int, int> pixelOffset( const Position& pos );

	static unsigned char rotString2Char( QString rot );

	static QColor colorInt2Color( unsigned int color );
	static unsigned int string2Color( QString color );
	static QColor string2QColor( QString color );
	static QColor variant2QColor( QVariant color );

	static QString randomMetal();
	static QString randomMetalSliver( QString sourceMaterial );

	static int ticksPerMinute;
	static int minutesPerHour;
	static int hoursPerDay;
	static int ticksPerDay;
	static int daysPerSeason;

	static int ticksPerDayRandomized( int percentage );

	static quint64 nextSeasonChangeTick();

	static unsigned int createRawMaterialItem( Position pos, unsigned int materialID );

	static unsigned int reverseFib( unsigned int number );

	static Position reachableBorderPos( Position pos, bool& found );
	static Position borderPos( bool& found );

	static QList<Position> neighbors8( Position pos );

	static QToolButton* createToolButton( QIcon icon, QString text, QString toolTip );

	static QString getRandomString( int length );

	static QVariantList uintList2Variant( const QList<unsigned int>& list );
	static QList<unsigned int> variantList2UInt( const QVariantList& vlist );

	static QVariantList positionList2Variant( const QList<Position>& list );
	static QList<Position> variantList2Position( const QVariantList& vlist );

	static QVariantList pairList2Variant( const QList<QPair<QString, QString>>& plist );
	static QList<QPair<QString, QString>> variantList2Pair( const QVariantList& plist );

	static void setIndexFromText( QComboBox* cb, QString text );

	static QString addDyeMaterial( QString sourceMaterial, QString dyeMaterial );

	static void debugVM( QVariantMap vm, QString name );

	static QPixmap smallPixmap( Sprite* sprite, QString season, int rotation );
	static QPixmap createWorkshopImage( const QString& workshopID, const QStringList& m_mats );
	static QPixmap createItemImage( const QString& constructionID, const QStringList& mats );
	static QPixmap createItemImage2( const QString& constructionID, const QStringList& mats );
	static QPixmap createConstructionImage( const QString& constructionID, const QStringList& mats );
	static Sprite* getSprite( int x, int y, const QList<QVariantMap>& comps, unsigned char& rot, const QStringList& mats );

	static void createBufferForNoesisImage( const QPixmap& pm, std::vector<unsigned char>& buffer );

	static QStringList possibleMaterialsForItem( QString itemSID );
	static QStringList possibleMaterials( QString allowedMaterials, QString allowedMaterialTypes );

private:
	Util();
	~Util();
};
