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
/** @file util.cpp
 *  @brief Implementation of the Util class.
 *
 *  Provides miscellaneous utility functions used throughout the game, including
 *  item/material name lookups, job requirement queries, container checks,
 *  tile serialization, color conversions, time calculations, sprite/image
 *  creation, material queries, position utilities, and serialization helpers.
 */
#include "util.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/global.h"
#include "../base/pathfinder.h"
#include "../game/game.h"
#include "../game/inventory.h"
#include "../game/object.h"
#include "../game/world.h"
#include "../gfx/sprite.h"
#include "../gfx/spritefactory.h"
#include "../gui/strings.h"

#include <QComboBox>
#include <QDebug>
#include <QRandomGenerator>
#include <QIcon>
#include <QJsonDocument>
#include <QPainter>

/** @brief Constructs a Util instance associated with the given Game.
 *  @param game Pointer to the owning Game instance.
 */
Util::Util( Game* game ) :
	g( game )
{
}

/** @brief Destructor. */
Util::~Util()
{
}

/** @brief Retrieves seed item IDs for plants matching a given type and material.
 *  @param plantType The plant type to filter by (e.g. "Tree", "Plant").
 *  @param material The material to match, or "any" to accept all materials.
 *  @return A list of seed item string IDs for matching plants.
 */
QStringList Util::seedItems( QString plantType, QString material )
{
	QStringList out;

	QList<QString> ids = DB::ids( "Plants", "Type", plantType );
	for ( const auto& id : ids )
	{
		QString mat = DB::select( "Material", "Plants", id ).toString();
		if ( mat == material || material == "any" )
		{
			out.push_back( DB::select( "SeedItemID", "Plants", id ).toString() );
		}
	}

	return out;
}

/** @brief Returns the localized display name for an item.
 *  @param itemID The string ID of the item.
 *  @return The translated item name.
 */
QString Util::itemName( QString itemID )
{
	return S::s( "$ItemName_" + itemID );
}

/** @brief Returns the localized display name for a material.
 *  @param materialID The string ID of the material.
 *  @return The translated material name.
 */
QString Util::materialName( QString materialID )
{
	return S::s( "$MaterialName_" + materialID );
}

/** @brief Looks up the type category of a material (e.g. "Stone", "Metal", "Soil").
 *  @param materialID The string ID of the material.
 *  @return The material type string from the database.
 */
QString Util::materialType( QString materialID )
{
	return DB::select( "Type", "Materials", materialID ).toString();
}

/** @brief Returns the skill ID required to perform a given job.
 *  @param jobID The string ID of the job.
 *  @return The required skill string ID, or empty string if the job is not found.
 */
QString Util::requiredSkill( QString jobID )
{
	auto dbjb = DB::job( jobID );
	if( dbjb )
	{
		return dbjb->SkillID;
	}
	return "";
}

/** @brief Returns the magic skill ID required to cast a given spell.
 *  @param spellID The string ID of the spell.
 *  @return The required skill string ID.
 */
QString Util::requiredMagicSkill( QString spellID )
{
	return DB::select( "SkillID", "Spells", spellID ).toString();
}

/** @brief Returns the tool item ID required to perform a given job.
 *  @param jobID The string ID of the job.
 *  @return The required tool item string ID, or empty string if the job is not found.
 */
QString Util::requiredTool( QString jobID )
{
	auto dbjb = DB::job( jobID );
	if( dbjb )
	{
		return dbjb->RequiredToolItemID;
	}
	return "";
}

/** @brief Determines the minimum tool level required to perform a job at a given position.
 *
 *  The required level depends on the job's method (ByWallMaterial, ByFloorMaterial,
 *  or ByPlantMaterial) and the material present at the tile position.
 *
 *  @param jobID The string ID of the job.
 *  @param pos The world position where the job will be performed.
 *  @return The minimum tool level required (0 if no special tool level is needed).
 */
int Util::requiredToolLevel( QString jobID, Position pos )
{
	auto dbjb = DB::job( jobID );
	if( !dbjb )
	{
		return 0;
	}
	QString method = dbjb->RequiredToolLevel;
	int level      = 0;

	if ( method == "ByWallMaterial" )
	{
		level = requiredToolLevelByWallMaterial( pos );

		if ( jobID == "DigHole" || jobID == "DigStairsDown" )
		{
			pos.z = qMax( 0, pos.z - 1 );
		}
		level = qMax( level, requiredToolLevelByWallMaterial( pos ) );
	}
	if ( method == "ByFloorMaterial" )
	{
		level = requiredToolLevelByFloorMaterial( pos );
	}
	if ( method == "ByPlantMaterial" )
	{
		level = 1;
	}
	return level;
}

/** @brief Determines the required tool level based on the wall material at a position.
 *  @param pos The world position to check.
 *  @return The tool level required to work on the wall material at this position.
 */
int Util::requiredToolLevelByWallMaterial( Position pos )
{
	int level                   = 0;
	unsigned short wallMaterial = g->w()->getTile( pos ).wallMaterial;
	QString wallMatSID          = DBH::materialSID( wallMaterial );

	QString wallMatType = Util::materialType( wallMatSID );

	if ( DBH::rowID( "MaterialToToolLevel", wallMatSID ) )
	{
		level = DB::select( "RequiredToolLevel", "MaterialToToolLevel", wallMatSID ).toInt();
	}
	else if ( DBH::rowID( "MaterialToToolLevel", wallMatType ) )
	{
		level = DB::select( "RequiredToolLevel", "MaterialToToolLevel", wallMatType ).toInt();
	}
	return level;
}
/** @brief Determines the required tool level based on the floor material at a position.
 *  @param pos The world position to check.
 *  @return The tool level required to work on the floor material at this position.
 */
int Util::requiredToolLevelByFloorMaterial( Position pos )
{
	int level                    = 0;
	unsigned short floorMaterial = g->w()->getTile( pos ).floorMaterial;

	QString floorMatSID = DBH::materialSID( floorMaterial );

	QString floorMatType = Util::materialType( floorMatSID );

	if ( DBH::rowID( "MaterialToToolLevel", floorMatSID ) )
	{
		level = DB::select( "RequiredToolLevel", "MaterialToToolLevel", floorMatSID ).toInt();
	}
	else if ( DBH::rowID( "MaterialToToolLevel", floorMatType ) )
	{
		level = DB::select( "RequiredToolLevel", "MaterialToToolLevel", floorMatType ).toInt();
	}
	return level;
}

/** @brief Returns the tool level of a specific item by its unique ID.
 *
 *  Looks up the item's material and queries the MaterialToToolLevel table.
 *
 *  @param itemUID The unique ID of the item in the inventory.
 *  @return The tool level of the item, or 0 if not found.
 */
int Util::toolLevel( unsigned int itemUID )
{
	int level        = 0;
	QString material = g->inv()->materialSID( itemUID );

	QString materialType = Util::materialType( material );

	if ( DBH::rowID( "MaterialToToolLevel", material ) )
	{
		level = DB::select( "ToolLevel", "MaterialToToolLevel", material ).toInt();
	}
	else if ( DBH::rowID( "MaterialToToolLevel", materialType ) )
	{
		level = DB::select( "ToolLevel", "MaterialToToolLevel", materialType ).toInt();
	}
	return level;
}

/** @brief Returns the tool level for a given material string ID.
 *
 *  Falls back to looking up by material type if the specific material is not found.
 *
 *  @param materialSID The string ID of the material.
 *  @return The tool level for this material, or 0 if not found.
 */
int Util::toolLevel( QString materialSID )
{
	if ( DBH::rowID( "MaterialToToolLevel", materialSID ) )
	{
		return DB::select( "ToolLevel", "MaterialToToolLevel", materialSID ).toInt();
	}
	else
	{
		QString materialType = Util::materialType( materialSID );
		if ( DBH::rowID( "MaterialToToolLevel", materialType ) )
		{
			return DB::select( "ToolLevel", "MaterialToToolLevel", materialType ).toInt();
		}
	}
	return 0;
}

/** @brief Returns the set of item string IDs allowed in a given container.
 *  @param containerID The unique ID of the container item.
 *  @return A set of item string IDs that may be stored in this container.
 */
QSet<QString> Util::itemsAllowedInContainer( unsigned int containerID )
{
	return Global::allowedInContainer.value( g->inv()->itemSID( containerID ) );
}

/** @brief Checks whether a specific item is allowed to be stored in a given container.
 *  @param itemID The unique ID of the item to check.
 *  @param containerID The unique ID of the container.
 *  @return True if the item type is allowed in this container type, false otherwise.
 */
bool Util::itemAllowedInContainer( unsigned int itemID, unsigned int containerID )
{
	return Global::allowedInContainer.value( g->inv()->itemSID( containerID ) ).contains( g->inv()->itemSID( itemID ) );
}

/** @brief Initializes the global mapping of which items are allowed in which containers.
 *
 *  Reads the AllowedContainers field from the Items database table and populates
 *  Global::allowedInContainer. Should be called once during game initialization.
 */
void Util::initAllowedInContainer()
{
	Global::allowedInContainer.clear();

	for ( const auto& itemSID : DB::ids( "Items" ) )
	{
		auto containerSID = DB::select( "AllowedContainers", "Items", itemSID ).toString();

		if ( !containerSID.isEmpty() )
		{
			Global::allowedInContainer[containerSID].insert( itemSID );
		}
	}
}

/** @brief Returns the carry container type for a given item.
 *  @param itemSID The string ID of the item.
 *  @return The string ID of the container used to carry this item, or empty if none.
 */
QString Util::carryContainerForItem( QString itemSID )
{
	return DB::select( "CarryContainer", "Items", itemSID ).toString();
}

/** @brief Returns the storage capacity of a container type.
 *  @param containerSID The string ID of the container.
 *  @return The maximum number of items the container can hold.
 */
int Util::capacity( QString containerSID )
{
	return DB::select( "Capacity", "Containers", containerSID ).toInt();
}

/** @brief Adds a float value to an existing float entry in a QVariantMap.
 *
 *  Retrieves the current value for the key, adds addValue to it, and stores the result.
 *
 *  @param vm The variant map to modify.
 *  @param key The key whose value should be incremented.
 *  @param addValue The float amount to add to the existing value.
 */
void Util::addFloatToVariant( QVariantMap& vm, QString key, float addValue )
{
	float value = vm.value( key ).toFloat();
	value += addValue;
	vm.insert( key, value );
}

/** @brief Serializes a Tile's fields into a delimited string for save files.
 *  @param tile The tile to serialize.
 *  @param seperator The delimiter string between fields.
 *  @return A string representation of the tile's properties.
 */
QString Util::tile2String( Tile& tile, QString seperator )
{
	QString out = QString::number( static_cast<quint64>( tile.flags ) );
	out += seperator;
	out += QString::number( tile.floorType );
	out += seperator;
	out += QString::number( tile.floorMaterial );
	out += seperator;
	out += QString::number( tile.wallType );
	out += seperator;
	out += QString::number( tile.wallMaterial );
	out += seperator;
	out += QString::number( tile.floorRotation );
	out += seperator;
	out += QString::number( tile.wallRotation );
	out += seperator;
	out += QString::number( tile.fluidLevel );
	out += seperator;
	if ( tile.wallSpriteUID )
	{
		out += QString::number( tile.wallSpriteUID );
	}
	else
	{
		out += QString::number( 0 );
	}
	out += seperator;
	if ( tile.floorSpriteUID )
	{
		out += QString::number( tile.floorSpriteUID );
	}
	else
	{
		out += QString::number( 0 );
	}
	return out;
}

/** @brief Deserializes a delimited string back into a Tile's fields.
 *  @param tile The tile to populate with deserialized data.
 *  @param data The serialized string (as produced by tile2String).
 *  @param seperator The delimiter string between fields.
 */
void Util::string2Tile( Tile& tile, QString data, QString seperator )
{
	QStringList dl = data.split( seperator );
	int i          = 0;
	if ( dl.size() == 10 )
	{
		tile.flags         = (TileFlag)dl[0].toUInt();
		tile.floorType     = (FloorType)dl[1].toUInt();
		tile.floorMaterial = dl[2].toUInt();
		tile.wallType      = (WallType)dl[3].toUInt();
		tile.wallMaterial  = dl[4].toUInt();
		tile.floorRotation = dl[5].toUInt();
		tile.wallRotation  = dl[6].toUInt();
		//tile.fluidLevel = dl[7].toUInt();
		tile.wallSpriteUID  = dl[8].toUInt();
		tile.floorSpriteUID = dl[9].toUInt();
	}
}

/** @brief Serializes an int-to-int map into a flat delimited string.
 *
 *  Keys and values are interleaved: key1<sep>val1<sep>key2<sep>val2...
 *
 *  @param data The map to serialize.
 *  @param seperator The delimiter string between entries.
 *  @return The serialized string, or empty if the map is empty.
 */
QString Util::mapJoin( QMap<int, int> data, QString seperator )
{
	QString out;
	if ( !data.isEmpty() )
	{
		for ( const auto& rp : data.toStdMap() )
		{
			out += QString::number( rp.first );
			out += seperator;
			out += QString::number( rp.second );
			out += seperator;
		}
		out.chop( 1 );
	}
	return out;
}

/** @brief Deserializes a delimited string back into an int-to-int map.
 *  @param data The serialized string (as produced by mapJoin).
 *  @param seperator The delimiter string between entries.
 *  @return The reconstructed map.
 */
QMap<int, int> Util::mapSplit( QString data, QString seperator )
{
	QMap<int, int> m;
	if ( !data.isEmpty() )
	{
		QStringList dl = data.split( seperator );
		for ( int i = 0; i < dl.size(); i += 2 )
		{
			m.insert( dl[i].toInt(), dl[i + 1].toInt() );
		}
	}
	return m;
}

/** @brief Computes the isometric pixel offset for a tile at the given grid coordinates.
 *  @param x The tile X coordinate.
 *  @param y The tile Y coordinate.
 *  @param z The tile Z coordinate (currently unused in the offset calculation).
 *  @return A pair of (xOffset, yOffset) in pixel space.
 */
QPair<int, int> Util::pixelOffset( const int x, const int y, const int z )
{
	int dimX = Global::dimX;
	int dimY = Global::dimY;
	int dimZ = Global::dimZ;
	/*
	int xOff = x * 32 - 16 * x + 16 * y;
	int yOff = -( y * 32 - ( 8 * y + 8 * x ) );
	*/
	int xOff = -16 * x + 16 * y;
	int yOff = -8 * x + -8 * y;

	return QPair<int, int>( xOff, yOff );
}

/** @brief Computes the isometric pixel offset for a tile at the given Position.
 *  @param pos The tile position.
 *  @return A pair of (xOffset, yOffset) in pixel space.
 */
QPair<int, int> Util::pixelOffset( const Position& pos )
{
	return Util::pixelOffset( pos.x, pos.y, pos.z );
}

/** @brief Converts a rotation direction string to a numeric rotation value.
 *  @param rot The rotation string: "FR" (0), "FL" (1), "BL" (2), or "BR" (3).
 *  @return The numeric rotation value (0-3), defaulting to 0 for "FR" or unrecognized input.
 */
unsigned char Util::rotString2Char( QString rot )
{
	if ( rot == "FL" )
		return 1;
	else if ( rot == "BL" )
		return 2;
	else if ( rot == "BR" )
		return 3;
	return 0;
}

/** @brief Creates a 32x32 thumbnail pixmap from a sprite, vertically centered.
 *
 *  Scans the sprite's image to find the vertical extent of non-black content,
 *  then copies and centers it into a 32x32 output image.
 *
 *  @param sprite Pointer to the source Sprite (may be null).
 *  @param season The current season string for sprite variant selection.
 *  @param rotation The rotation index for the sprite.
 *  @return A 32x32 QPixmap thumbnail, or an empty pixmap if sprite is null.
 */
QPixmap Util::smallPixmap( Sprite* sprite, QString season, int rotation )
{
	if ( !sprite )
	{
		return QPixmap();
	}
	QPixmap pm = sprite->pixmap( season, 0, 0 );

	QImage img = pm.toImage();
	QImage newImg( 32, 32, QImage::Format::Format_RGBA8888 );

	int firstLine = 0;
	int lastLine  = 63;

	for ( int y = 0; y < 64; ++y )
	{
		bool found = false;
		for ( int x = 0; x < 32; ++x )
		{
			if ( img.height() <= y )
				qDebug() << "Util::smallPixmap 1" << sprite->sID;
			QColor col = img.pixelColor( x, y );
			if ( col.red() + col.green() + col.blue() > 0 )
			{
				firstLine = y;
				found     = true;
				break;
			}
		}
		if ( found )
			break;
	}
	for ( int y = 63; y >= firstLine; --y )
	{
		bool found = false;
		for ( int x = 0; x < 32; ++x )
		{
			if ( img.height() <= y )
				qDebug() << "Util::smallPixmap 2" << sprite->sID;
			QColor col = img.pixelColor( x, y );
			if ( col.red() + col.green() + col.blue() > 0 )
			{
				lastLine = y;
				found    = true;
				break;
			}
		}
		if ( found )
			break;
	}

	int numLines = lastLine - firstLine + 1;
	int yOff     = qMax( 0, ( 32 - numLines ) ) / 2;
	QColor col1( 0, 0, 0, 0 );
	for ( int y = 0; y < 32; ++y )
	{
		for ( int x = 0; x < 32; ++x )
		{
			newImg.setPixelColor( x, y, col1 );
		}
	}
	for ( int y = 0; y < 32 - yOff; ++y )
	{
		for ( int x = 0; x < 32; ++x )
		{
			if ( img.height() <= y )
				qDebug() << "Util::smallPixmap 3" << sprite->sID;
			QColor col = img.pixelColor( x, y + firstLine );
			newImg.setPixelColor( x, y + yOff, col );
		}
	}

	QPixmap newPM = QPixmap::fromImage( newImg );
	return newPM;
}

/** @brief Converts a packed 32-bit RGBA integer to a QColor.
 *
 *  Byte layout (high to low): R, G, B, A.
 *
 *  @param color The packed RGBA color value.
 *  @return The corresponding QColor.
 */
QColor Util::colorInt2Color( unsigned int color )
{
	int a = color & 0xff;
	int b = ( color >> 8 ) & 0xff;
	int gg = ( color >> 16 ) & 0xff;
	int r = ( color >> 24 ) & 0xff;
	return QColor( r, gg, b, a );
}

/** @brief Parses a space-separated "R G B [A]" string into a packed 32-bit RGBA integer.
 *  @param color The color string (e.g. "255 128 0" or "255 128 0 200").
 *  @return The packed RGBA color value (alpha defaults to 255 if omitted).
 */
unsigned int Util::string2Color( QString color )
{
	QStringList clist = color.split( " " );
	unsigned char r, gg, b;
	r = gg = b       = 0;
	unsigned char a = 255;
	if ( clist.size() > 2 )
	{
		r = clist[0].toInt();
		gg = clist[1].toInt();
		b = clist[2].toInt();
	}
	if ( clist.size() > 3 )
	{
		a = clist[3].toInt();
	}

	return r * 16777216 + gg * 65536 + b * 256 + a;
}

/** @brief Parses a space-separated "R G B A" string into a QColor.
 *  @param color The color string with exactly 4 space-separated components.
 *  @return The corresponding QColor, or a default QColor if the format is invalid.
 */
QColor Util::string2QColor( QString color )
{
	QList<QString> csl = color.split( ' ' );
	if ( csl.size() == 4 )
	{
		return QColor( csl[0].toInt(), csl[1].toInt(), csl[2].toInt(), csl[3].toInt() );
	}
	return QColor();
}

/** @brief Converts a QVariant containing a space-separated "R G B A" string to a QColor.
 *  @param color The QVariant holding the color string.
 *  @return The corresponding QColor, or a default QColor if the format is invalid.
 */
QColor Util::variant2QColor( QVariant color )
{
	QList<QString> csl = color.toString().split( ' ' );
	if ( csl.size() == 4 )
	{
		return QColor( csl[0].toInt(), csl[1].toInt(), csl[2].toInt(), csl[3].toInt() );
	}
	return QColor();
}

/** @brief Creates a raw material item at a position based on the material type.
 *
 *  Determines the raw item type (RawSoil, RawStone, RawCoal, RawOre, RawGem)
 *  from the material's type category and creates the corresponding inventory item.
 *
 *  @param pos The world position where the item should be created.
 *  @param materialID The numeric material ID to look up.
 *  @return The unique ID of the newly created item, or 0 if the material type is unrecognized.
 */
unsigned int Util::createRawMaterialItem( Position pos, unsigned int materialID )
{
	//create items from the wall material
	QString materialSID = "None";
	QString type        = "None";
	materialSID         = DBH::materialSID( materialID );
	type                = Util::materialType( materialSID );

	if ( type == "Soil" )
	{
		return g->inv()->createItem( pos, "RawSoil", materialSID );
	}
	else if ( type == "Sand" )
	{
		return g->inv()->createItem( pos, "RawSoil", materialSID );
	}
	else if ( type == "Clay" )
	{
		return g->inv()->createItem( pos, "RawSoil", materialSID );
	}
	else if ( type == "Stone" )
	{
		return g->inv()->createItem( pos, "RawStone", materialSID );
	}
	else if ( type == "Coal" )
	{
		return g->inv()->createItem( pos, "RawCoal", materialSID );
	}
	else if ( type == "Metal" )
	{
		return g->inv()->createItem( pos, "RawOre", materialSID );
	}
	else if ( type == "Gem" )
	{
		return g->inv()->createItem( pos, "RawGem", materialSID );
	}
	return 0;
}

/** @brief Returns a randomized tick count for one day, varied by a percentage.
 *
 *  The result is ticksPerDay plus or minus a random offset within the given percentage.
 *
 *  @param percentage The maximum deviation percentage (e.g. 10 means +/-10%).
 *  @return The randomized tick count for one day.
 */
int Util::ticksPerDayRandomized( int percentage )
{
	int maxOffset = qMax( 1, (int)( (float)Util::ticksPerDay / 100. ) * percentage );
	return Util::ticksPerDay + ( ( rand() % ( maxOffset * 2 ) ) - maxOffset );
}

/** @brief Computes the inverse Fibonacci index for an XP value.
 *
 *  Uses the closed-form formula n(F) = floor(log(F * sqrt(5) + 0.5) / log(phi))
 *  to determine which Fibonacci number the given value corresponds to, scaled
 *  by Global::xpMod.
 *
 *  @param number The XP value to convert.
 *  @return The Fibonacci index (effectively the level), minimum 1.
 */
unsigned int Util::reverseFib( unsigned int number )
{
	if ( number < Global::xpMod )
		return 1;

	double num = (double)number / Global::xpMod;

	//n(F) = Floor[ Log(F Sqrt(5) + 1/2)/Log(Phi)]
	return floor( log( num * 2.2360679775 + 0.5 ) / log( 1.6180339887 ) );
}

/** @brief Finds a random border position that is reachable from a given position.
 *
 *  Picks a random map edge and position, projects it down to ground level,
 *  then verifies it is walkable, has no fluid, and is path-connected to fromPos.
 *
 *  @param fromPos The position from which reachability is checked.
 *  @param found Set to true if a valid reachable border position was found.
 *  @return The border position (valid only if found is true).
 */
Position Util::reachableBorderPos( Position fromPos, bool& found )
{
	int randPos = qMax( 2, rand() % ( Global::dimX - 2 ) );

	Position pos( randPos, randPos, Global::dimZ - 1 );
	int border = rand() % 4;
	switch ( border )
	{
		case 0: //north
			pos.y = 1;
			break;
		case 1: //east
			pos.x = Global::dimX - 2;
			break;
		case 2: //south
			pos.y = Global::dimX - 2;
			break;
		case 3: // west
			pos.x = 1;
			break;
		default:
			break;
	}
	g->w()->getFloorLevelBelow( pos, false );

	if ( g->w()->fluidLevel( pos ) == 0 )
	{
		if ( g->pf()->checkConnectedRegions( pos, fromPos ) )
		{
			found = true;
			return pos;
		}
	}
	return pos;
}

/** @brief Finds a walkable position on the map border.
 *
 *  Starts at a random edge position and walks the perimeter until a walkable
 *  ground-level tile is found, or until all edge positions have been tried.
 *
 *  @param found Set to true if a walkable border position was found.
 *  @return The walkable border position (valid only if found is true).
 */
Position Util::borderPos( bool& found )
{
	Position pos;
	int border = rand() % 4;
	switch ( border )
	{
		case 0: //north
			pos.x = qMax( 2, rand() % ( Global::dimX - 2 ) );
			pos.y = 1;
			break;
		case 1: //east
			pos.x = Global::dimX - 2;
			pos.y = qMax( 2, rand() % ( Global::dimY - 2 ) );
			break;
		case 2: //south
			pos.x = qMax( 2, rand() % ( Global::dimX - 2 ) );
			pos.y = Global::dimY - 2;
			break;
		case 3: // west
			pos.x = 1;
			pos.y = qMax( 2, rand() % ( Global::dimY - 2 ) );
			break;
		default:
			break;
	}
	const size_t maxTries = ( Global::dimX - 4 ) * ( Global::dimY - 4 );
	size_t nTries         = 0;
	// walk the edge till a legal position is found
	while ( ++nTries < maxTries )
	{
		pos.z = Global::dimZ - 1;
		// project position onto ground
		g->w()->getFloorLevelBelow( pos, false );
		if ( g->w()->isWalkableGnome(pos) )
		{
			found = true;
			return pos;
		}
		switch (border)
		{
			case 0:
				if ( pos.x >= Global::dimX - 2 )
				{
					border = 1;
					++pos.y;
				}
				else
				{
					++pos.x;
				}
				break;
			case 1:
				if ( pos.y >= Global::dimY - 2 )
				{
					border = 2;
					--pos.x;
				}
				else
				{
					++pos.y;
				}
				break;
			case 2:
				if ( pos.x <= 2 )
				{
					border = 3;
					--pos.y;
				}
				else
				{
					--pos.x;
				}
				break;
			case 3:
				if ( pos.y <= 2 )
				{
					border = 0;
					++pos.x;
				}
				else
				{
					--pos.y;
				}
				break;
		}
	}
}

/** @brief Returns the 8 neighboring positions surrounding a given position (same Z level).
 *  @param pos The center position.
 *  @return A list of 8 positions in the cardinal and diagonal directions.
 */
QList<Position> Util::neighbors8( Position pos )
{
	QList<Position> out;
	int x = pos.x;
	int y = pos.y;
	int z = pos.z;

	out.push_back( Position( x - 1, y - 1, z ) );
	out.push_back( Position( x - 1, y, z ) );
	out.push_back( Position( x - 1, y + 1, z ) );
	out.push_back( Position( x, y - 1, z ) );
	out.push_back( Position( x, y + 1, z ) );
	out.push_back( Position( x + 1, y - 1, z ) );
	out.push_back( Position( x + 1, y, z ) );
	out.push_back( Position( x + 1, y + 1, z ) );

	return out;
}

/** @brief Calculates the game tick at which the next season change will occur.
 *  @return The absolute tick number of the next season transition.
 */
quint64 Util::nextSeasonChangeTick()
{
	// TODO make this mod safe
	int minute            = ( GameState::minute == 60 ) ? 0 : GameState::minute;
	int hour              = ( GameState::hour == 24 ) ? 0 : GameState::hour;
	int ticksToNextHour   = ( Util::minutesPerHour - minute ) * Util::ticksPerMinute;
	int ticksToNextDay    = ( Util::hoursPerDay - ( hour + 1 ) ) * Util::ticksPerMinute * Util::minutesPerHour;
	int ticksToNextSeason = ( Util::daysPerSeason - ( GameState::day ) ) * Util::ticksPerMinute * Util::minutesPerHour * Util::hoursPerDay;

	//qDebug() << "#" << Util::minutesPerHour << minute << " - " << Util::hoursPerDay << hour << " - " <<  Util::daysPerSeason << Global::day;
	//qDebug() << "##" << ticksToNextHour << ticksToNextDay << ticksToNextSeason;

	return GameState::tick + ticksToNextHour + ticksToNextDay + ticksToNextSeason;
}

/** @brief Returns a random metal type string from a fixed list of common metals.
 *  @return A metal string ID (e.g. "Copper", "Iron", "Gold"), or empty if the list is empty.
 */
QString Util::randomMetal()
{
	QStringList metals = { "Copper", "Tin", "Malachite", "Iron", "Lead", "Silver", "Gold", "Platinum" };
	if ( metals.size() > 0 )
	{
		return metals.at( rand() % metals.size() );
	}
	return "";
}

/** @brief Returns a weighted-random metal type based on the source material's distribution.
 *
 *  Reads the probability weights from the RandomMetals database table for the
 *  given source material and selects a metal type using a cumulative random roll.
 *
 *  @param sourceMaterial The string ID of the source material (e.g. an ore type).
 *  @return The selected metal string ID, defaulting to "Copper" if no match is found.
 */
QString Util::randomMetalSliver( QString sourceMaterial )
{
	auto row = DB::selectRow( "RandomMetals", sourceMaterial );

	auto ra = rand() % 100;

	QStringList metals = { "Copper", "Tin", "Malachite", "Iron", "Lead", "Silver", "Gold", "Platinum" };
	int sum            = 0;
	for ( const auto& metal : metals )
	{
		sum += row.value( metal ).toInt();
		if ( ra < sum )
		{
			return metal;
		}
	}

	return "Copper";
}

/** @brief Creates and configures a QToolButton with an icon, optional text, and tooltip.
 *  @param icon The icon to display on the button.
 *  @param text Optional text label shown below the icon; if empty, a larger icon-only button is created.
 *  @param toolTip Optional tooltip text for the button.
 *  @return A newly allocated QToolButton (caller takes ownership).
 */
QToolButton* Util::createToolButton( QIcon icon, QString text, QString toolTip )
{
	QToolButton* button = new QToolButton;
	button->setIcon( icon );

	if ( text.isEmpty() )
	{
		button->setIconSize( QSize( 50, 50 ) );
		button->setMinimumSize( 56, 56 );
		button->setMaximumSize( 56, 56 );
	}
	else
	{
		button->setIconSize( QSize( 32, 32 ) );
		button->setText( text );
		button->setToolButtonStyle( Qt::ToolButtonTextUnderIcon );
		button->setFixedHeight( 56 );
		//button->setMinimumWidth( 56 );
		button->setMaximumWidth( 120 );
	}
	if ( !toolTip.isEmpty() )
	{
		button->setToolTip( toolTip );
	}

	return button;
}

/** @brief Generates a random alphanumeric string of the specified length.
 *  @param length The desired length of the random string.
 *  @return A string of random upper/lowercase letters and digits.
 */
QString Util::getRandomString( int length )
{
	const QString possibleCharacters( "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" );
	srand( std::chrono::system_clock::now().time_since_epoch().count() );
	QString randomString;
	for ( int i = 0; i < length; ++i )
	{
		int index      = QRandomGenerator::global()->bounded( possibleCharacters.length() );
		QChar nextChar = possibleCharacters.at( index );
		randomString.append( nextChar );
	}
	return randomString;
}

/** @brief Converts a list of unsigned ints to a QVariantList for serialization.
 *  @param list The unsigned int list to convert.
 *  @return A QVariantList where each element wraps one unsigned int.
 */
QVariantList Util::uintList2Variant( const QList<unsigned int>& list )
{
	QVariantList out;
	for ( auto ui : list )
	{
		out.append( ui );
	}
	return out;
}

/** @brief Converts a QVariantList back to a list of unsigned ints.
 *  @param vlist The QVariantList to convert (each element should be convertible to unsigned int).
 *  @return A list of unsigned int values.
 */
QList<unsigned int> Util::variantList2UInt( const QVariantList& vlist )
{
	QList<unsigned int> out;
	for ( const auto& vui : vlist )
	{
		out.append( vui.toUInt() );
	}
	return out;
}

/** @brief Converts a list of Positions to a QVariantList of their string representations.
 *  @param list The Position list to convert.
 *  @return A QVariantList where each element is a Position serialized to string.
 */
QVariantList Util::positionList2Variant( const QList<Position>& list )
{
	QVariantList out;
	for ( const auto& pos : list )
	{
		out.append( pos.toString() );
	}
	return out;
}

/** @brief Converts a QVariantList of position strings back to a list of Positions.
 *  @param vlist The QVariantList to convert (each element should be a Position string).
 *  @return A list of Position objects.
 */
QList<Position> Util::variantList2Position( const QVariantList& vlist )
{
	QList<Position> out;
	for ( const auto& vpos : vlist )
	{
		out.append( Position( vpos ) );
	}
	return out;
}

/** @brief Converts a list of string pairs to a flat QVariantList for serialization.
 *
 *  Pairs are stored interleaved: first1, second1, first2, second2, ...
 *
 *  @param plist The list of string pairs to convert.
 *  @return A QVariantList with alternating first/second values.
 */
QVariantList Util::pairList2Variant( const QList<QPair<QString, QString>>& plist )
{
	QVariantList out;
	for ( const auto& pair : plist )
	{
		out.append( pair.first );
		out.append( pair.second );
	}
	return out;
}

/** @brief Converts a flat QVariantList back to a list of string pairs.
 *  @param plist The QVariantList with alternating first/second values.
 *  @return A list of string pairs reconstructed from the flat list.
 */
QList<QPair<QString, QString>> Util::variantList2Pair( const QVariantList& plist )
{
	QList<QPair<QString, QString>> out;

	if ( plist.size() > 1 )
	{
		for ( int i = 0; i < plist.size(); i += 2 )
		{
			QPair<QString, QString> entry( plist[i].toString(), plist[i + 1].toString() );
			out.append( entry );
		}
	}

	return out;
}

/** @brief Sets a QComboBox's current index to the item matching the given text.
 *
 *  Searches through all items in the combo box and selects the first match.
 *  Signals are blocked during the index change to prevent unwanted side effects.
 *
 *  @param cb Pointer to the QComboBox to update.
 *  @param text The text to search for among the combo box items.
 */
void Util::setIndexFromText( QComboBox* cb, QString text )
{
	for ( int i = 0; i < cb->count(); ++i )
	{
		if ( cb->itemText( i ) == text )
		{
			cb->blockSignals( true );
			cb->setCurrentIndex( i );
			cb->blockSignals( false );
			return;
		}
	}
}

/** @brief Creates a new dyed material by combining a source material with a dye.
 *
 *  If the combined material does not already exist in the database, a new material
 *  row is created with the dye's color applied to the source material, and a
 *  localized name is registered.
 *
 *  @param sourceMaterial The string ID of the base material to dye.
 *  @param dyeMaterial The string ID of the dye material providing the color.
 *  @return The string ID of the resulting dyed material.
 */
QString Util::addDyeMaterial( QString sourceMaterial, QString dyeMaterial )
{
	QString targetMaterial = sourceMaterial + dyeMaterial;
	if ( !DB::select( "rowid", "Materials", targetMaterial ).toInt() )
	{
		auto sourceRow = DB::selectRow( "Materials", sourceMaterial );
		auto dyeRow    = DB::selectRow( "Materials", dyeMaterial );
		sourceRow.insert( "Color", dyeRow.value( "Color" ) );
		sourceRow.insert( "ID", targetMaterial );

		DB::addRow( "Materials", sourceRow );
		//DB::addTranslation( "$MaterialName_" + targetMaterial, S::s( "$MaterialName_" + sourceMaterial ) + " " + S::s( "$MaterialName_" + dyeMaterial ) );
		//strings are cached on game start, so no insert in DB but strings map
		Strings::getInstance().insertString( "$MaterialName_" + targetMaterial, S::s( "$MaterialName_" + dyeMaterial ) + " " + S::s( "$MaterialName_" + sourceMaterial ) );

		GameState::addedMaterials.append( sourceRow );
		
		GameState::addedTranslations.insert( "$MaterialName_" + targetMaterial, S::s( "$MaterialName_" + dyeMaterial ) + " " + S::s( "$MaterialName_" + sourceMaterial ) );
	}
	return targetMaterial;
}

/** @brief Prints all key-value pairs of a QVariantMap to the debug output.
 *  @param vm The variant map to dump.
 *  @param name A label printed before the map contents for identification.
 */
void Util::debugVM( QVariantMap vm, QString name )
{
	qDebug() << name;
	for ( const auto& key : vm.keys() )
	{
		qDebug() << key << ":" << vm.value( key ).toString();
	}
}

/** @brief Creates a 100x100 isometric preview image of a workshop.
 *
 *  If the workshop has a custom icon, that is loaded directly. Otherwise, the
 *  workshop's component sprites are composited in an isometric 3x3 grid.
 *
 *  @param workshopID The string ID of the workshop.
 *  @param mats The list of material string IDs for each component.
 *  @return A 100x100 QPixmap showing the workshop preview.
 */
QPixmap Util::createWorkshopImage( const QString& workshopID, const QStringList& mats )
{
	auto dbws = DB::workshop( workshopID );

	if( !dbws )
	{
		QPixmap pm( 100, 100 );
		pm.fill( QColor( 0, 0, 0, 0 ) );
		return pm;
	}

	if ( !dbws->Icon.isEmpty() )
	{
		const auto path = Global::cfg->get( "dataPath" ).toString() + "/xaml/buttons/" + dbws->Icon;
		QPixmap pm( path );
		assert( pm.width() > 0 );
		return pm;
	}
	QString season = GameState::seasonString;

	auto coms = DB::selectRows( "Workshops_Components", workshopID );

	QPixmap pm( 100, 100 );
	pm.fill( QColor( 0, 0, 0, 0 ) );

	QPainter painter( &pm );
	int x0 = 34;
	int y0 = 0;

	for ( int y = 0; y < 3; ++y )
	{
		for ( int x = 0; x < 3; ++x )
		{
			const int px      = x0 + 16 * x - 16 * y;
			const int py      = y0 + 8 * y + 8 * x;
			unsigned char rot = 0;
			Sprite* sprite    = getSprite( x - 1, y - 1, coms, rot, mats );
			if ( sprite )
			{
				//painter.drawPixmap( px + sprite->xOffset, py + sprite->yOffset, sprite->pixmap( season, rot ) );
				painter.drawPixmap( px, py, sprite->pixmap( season, rot, 0 ) );
			}
		}
	}
	return pm;
}

/** @brief Creates a 100x100 isometric preview image of an item.
 *  @param itemID The string ID of the item.
 *  @param mats The list of material string IDs for sprite creation.
 *  @return A 100x100 QPixmap showing the item preview.
 */
QPixmap Util::createItemImage( const QString& itemID, const QStringList& mats )
{
	auto vsprite = DB::select( "SpriteID", "Items", itemID );
	QVariantMap component;
	component.insert( "SpriteID", vsprite );
	component.insert( "Offset", "0 0 0" );

	QList<QVariantMap> comps;
	comps.append( component );

	QString season = GameState::seasonString;

	QPixmap pm( 100, 100 );
	pm.fill( QColor( 0, 0, 0, 0 ) );

	QPainter painter( &pm );
	int x0 = 34;
	int y0 = 0;

	for ( int y = 0; y < 3; ++y )
	{
		for ( int x = 0; x < 3; ++x )
		{
			const int px      = x0 + 16 * x - 16 * y;
			const int py      = y0 + 8 * y + 8 * x;
			unsigned char rot = 0;
			Sprite* sprite    = getSprite( x - 1, y - 1, comps, rot, mats );
			if ( sprite )
			{
				//painter.drawPixmap( px + sprite->xOffset, py + sprite->yOffset, sprite->pixmap( season, rot ) );
				painter.drawPixmap( px, py, sprite->pixmap( season, rot, 0 ) );
			}
		}
	}
	return pm;
}

/** @brief Creates a 32x32 preview image of an item (single-tile variant).
 *  @param itemID The string ID of the item.
 *  @param mats The list of material string IDs for sprite creation.
 *  @return A 32x32 QPixmap showing the item sprite.
 */
QPixmap Util::createItemImage2( const QString& itemID, const QStringList& mats )
{
	auto spriteID = DB::select( "SpriteID", "Items", itemID ).toString();
	
	QString season = GameState::seasonString;

	QPixmap pm( 32, 32 );
	pm.fill( QColor( 0, 0, 0, 0 ) );

	QPainter painter( &pm );
	int x0 = 0;
	int y0 = 0;

	Sprite* sprite = g->sf()->createSprite( spriteID, mats ) ;
	if ( sprite )
	{
		//painter.drawPixmap( px + sprite->xOffset, py + sprite->yOffset, sprite->pixmap( season, rot ) );
		painter.drawPixmap( 0, -16, sprite->pixmap( season, 0, 0 ) );
	}
	return pm;
}

/** @brief Creates a 100x100 isometric preview image of a construction.
 *  @param constructionID The string ID of the construction type.
 *  @param mats The list of material string IDs for each component sprite.
 *  @return A 100x100 QPixmap showing the construction preview.
 */
QPixmap Util::createConstructionImage( const QString& constructionID, const QStringList& mats )
{
	auto sprites = DB::selectRows( "Constructions_Sprites", constructionID );

	QString season = GameState::seasonString;

	QPixmap pm( 100, 100 );
	pm.fill( QColor( 0, 0, 0, 0 ) );

	QPainter painter( &pm );
	int x0 = 34;
	int y0 = 0;

	for ( int y = 0; y < 3; ++y )
	{
		for ( int x = 0; x < 3; ++x )
		{
			const int px      = x0 + 16 * x - 16 * y;
			const int py      = y0 + 8 * y + 8 * x;
			unsigned char rot = 0;
			Sprite* sprite    = getSprite( x - 1, y - 1, sprites, rot, mats );
			if ( sprite )
			{
				//painter.drawPixmap( px + sprite->xOffset, py + sprite->yOffset, sprite->pixmap( season, rot ) );
				painter.drawPixmap( px, py, sprite->pixmap( season, rot, 0 ) );
			}
		}
	}
	return pm;
}

/** @brief Finds and creates the sprite for a component at a given offset within a multi-tile object.
 *
 *  Searches the component list for one whose offset matches (x, y), resolves its
 *  materials and rotation, and creates the sprite via the SpriteFactory.
 *
 *  @param x The X offset to match within the component list.
 *  @param y The Y offset to match within the component list.
 *  @param comps The list of component variant maps (each with SpriteID, Offset, etc.).
 *  @param rot Output parameter set to the component's rotation value (0-3).
 *  @param mats The list of material string IDs to apply.
 *  @return Pointer to the created Sprite, or nullptr if no component matches or has no sprite.
 */
Sprite* Util::getSprite( int x, int y, const QList<QVariantMap>& comps, unsigned char& rot, const QStringList& mats )
{
	QStringList materialIDs;
	int mid = 0;
	for ( const auto& cm : comps )
	{
		Position pos( cm.value( "Offset" ).toString() );
		if ( pos.x == x && pos.y == y )
		{
			if ( !cm.value( "MaterialItem" ).toString().isEmpty() )
			{
				for ( const auto& mat : cm.value( "MaterialItem" ).toString().split( "|" ) )
				{
					materialIDs.push_back( mats[mat.toInt()] );
				}
			}
			else
			{
				materialIDs.push_back( mats[mid] );
			}
			if ( !cm.value( "WallRotation" ).toString().isEmpty() )
			{
				QString wallRot = cm.value( "WallRotation" ).toString();
				if ( wallRot == "FR" )
					rot = 0;
				else if ( wallRot == "FL" )
					rot = 1;
				else if ( wallRot == "BL" )
					rot = 2;
				else if ( wallRot == "BR" )
					rot = 3;
			}

			if ( !cm.value( "SpriteID" ).toString().isEmpty() )
			{
				return g->sf()->createSprite( cm.value( "SpriteID" ).toString(), materialIDs );
			}
			else
			{
				return nullptr;
			}
		}
		++mid;
	}
	return nullptr;
}

/** @brief Returns all materials that can be used to craft a given item.
 *  @param itemSID The string ID of the item to query.
 *  @return A list of material string IDs allowed for this item.
 */
QStringList Util::possibleMaterialsForItem( QString itemSID )
{
	QVariantMap row = DB::selectRow( "Items", itemSID );
	return possibleMaterials( row.value( "AllowedMaterials" ).toString(), row.value( "AllowedMaterialTypes" ).toString() );
}

/** @brief Returns all material IDs matching the given allowed materials and/or material types.
 *
 *  Combines explicitly listed material IDs (pipe-separated) with all materials
 *  belonging to the specified material types (also pipe-separated).
 *
 *  @param allowedMaterials Pipe-separated list of specific material string IDs, or empty.
 *  @param allowedMaterialTypes Pipe-separated list of material type names, or empty.
 *  @return A combined list of all matching material string IDs.
 */
QStringList Util::possibleMaterials( QString allowedMaterials, QString allowedMaterialTypes )
{
	QStringList out;

	if ( !allowedMaterials.isEmpty() )
	{
		for ( const auto& mat : allowedMaterials.split( "|" ) )
		{
			out.append( mat );
		}
	}
	if ( !allowedMaterialTypes.isEmpty() )
	{
		for ( const auto& type : allowedMaterialTypes.split( "|" ) )
		{
			for ( const auto& mat : DB::ids( "Materials", "Type", type ) )
			{
				out.append( mat );
			}
		}
	}
	return out;
}

/** @brief Converts a QPixmap into an RGBA byte buffer suitable for Noesis GUI texture creation.
 *
 *  The output buffer is laid out as width * height * 4 bytes in row-major order
 *  with R, G, B, A channels. Alpha is always set to 255.
 *
 *  @param pm The source pixmap to convert.
 *  @param buffer Output vector resized and filled with RGBA pixel data.
 */
void Util::createBufferForNoesisImage( const QPixmap& pm, std::vector<unsigned char>& buffer )
{
	int w = pm.width();
	int h = pm.height();

	buffer.resize( w * h * 4, 0 );
	QImage img = pm.toImage();

	for ( int x = 0; x < w; ++x )
	{
		for ( int y = 0; y < h; ++y )
		{
			auto color                    = img.pixelColor( x, y );
			buffer[( x + y * w ) * 4]     = color.red();
			buffer[( x + y * w ) * 4 + 1] = color.green();
			buffer[( x + y * w ) * 4 + 2] = color.blue();
			buffer[( x + y * w ) * 4 + 3] = 255;
		}
	}
}