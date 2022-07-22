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
#include "spdlog/spdlog.h"

#include <QComboBox>
#include <QIcon>
#include <QJsonDocument>

#include <SDL.h>
#include <SDL_image.h>

#include <filesystem>

namespace fs = std::filesystem;

Util::Util( Game* game ) :
	g( game )
{
}

Util::~Util()
{
}

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

QString Util::itemName( QString itemID )
{
	return S::s( "$ItemName_" + itemID );
}

QString Util::materialName( QString materialID )
{
	return S::s( "$MaterialName_" + materialID );
}

QString Util::materialType( QString materialID )
{
	return DB::select( "Type", "Materials", materialID ).toString();
}

std::string Util::requiredSkill( const std::string& jobID )
{
	auto dbjb = DB::job( jobID );
	if( dbjb )
	{
		return dbjb->SkillID.toStdString();
	}
	return "";
}

std::string Util::requiredMagicSkill( const std::string& spellID )
{
	return DB::select( "SkillID", "Spells", QString::fromStdString(spellID) ).toString().toStdString();
}

std::string Util::requiredTool( const std::string& jobID )
{
	auto dbjb = DB::job( jobID );
	if( dbjb )
	{
		return dbjb->RequiredToolItemID.toStdString();
	}
	return "";
}

int Util::requiredToolLevel( const std::string& jobID, Position pos )
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

int Util::requiredToolLevelByWallMaterial( Position pos )
{
	int level                   = 0;
	unsigned short wallMaterial = g->w()->getTile( pos ).wallMaterial;
	const auto wallMatSID       = DBH::materialSID( wallMaterial );

	QString wallMatType = Util::materialType( QString::fromStdString(wallMatSID) );

	if ( DBH::rowID( "MaterialToToolLevel", QString::fromStdString(wallMatSID) ) )
	{
		level = DB::select( "RequiredToolLevel", "MaterialToToolLevel", QString::fromStdString(wallMatSID) ).toInt();
	}
	else if ( DBH::rowID( "MaterialToToolLevel", wallMatType ) )
	{
		level = DB::select( "RequiredToolLevel", "MaterialToToolLevel", wallMatType ).toInt();
	}
	return level;
}
int Util::requiredToolLevelByFloorMaterial( Position pos )
{
	int level                    = 0;
	unsigned short floorMaterial = g->w()->getTile( pos ).floorMaterial;

	const auto floorMatSID = DBH::materialSID( floorMaterial );

	QString floorMatType = Util::materialType( QString::fromStdString(floorMatSID) );

	if ( DBH::rowID( "MaterialToToolLevel", QString::fromStdString(floorMatSID) ) )
	{
		level = DB::select( "RequiredToolLevel", "MaterialToToolLevel", QString::fromStdString(floorMatSID) ).toInt();
	}
	else if ( DBH::rowID( "MaterialToToolLevel", floorMatType ) )
	{
		level = DB::select( "RequiredToolLevel", "MaterialToToolLevel", floorMatType ).toInt();
	}
	return level;
}

int Util::toolLevel( unsigned int itemUID )
{
	int level        = 0;
	const auto material = g->inv()->materialSID( itemUID );

	QString materialType = Util::materialType( QString::fromStdString(material) );

	if ( DBH::rowID( "MaterialToToolLevel", QString::fromStdString(material) ) )
	{
		level = DB::select( "ToolLevel", "MaterialToToolLevel", QString::fromStdString(material) ).toInt();
	}
	else if ( DBH::rowID( "MaterialToToolLevel", materialType ) )
	{
		level = DB::select( "ToolLevel", "MaterialToToolLevel", materialType ).toInt();
	}
	return level;
}

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

absl::btree_set<QString> Util::itemsAllowedInContainer( unsigned int containerID )
{
	return Global::allowedInContainer.at( QString::fromStdString(g->inv()->itemSID( containerID )) );
}

bool Util::itemAllowedInContainer( unsigned int itemID, unsigned int containerID )
{
	return Global::allowedInContainer.at( QString::fromStdString( g->inv()->itemSID( containerID ) ) ).contains( QString::fromStdString( g->inv()->itemSID( itemID ) ) );
}

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

QString Util::carryContainerForItem( QString itemSID )
{
	return DB::select( "CarryContainer", "Items", itemSID ).toString();
}

int Util::capacity( QString containerSID )
{
	return DB::select( "Capacity", "Containers", containerSID ).toInt();
}

void Util::addFloatToVariant( QVariantMap& vm, QString key, float addValue )
{
	float value = vm.value( key ).toFloat();
	value += addValue;
	vm.insert( key, value );
}

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

std::string Util::mapJoin( absl::flat_hash_map<int, int> data, const std::string& seperator )
{
	std::string out;
	if ( !data.empty() )
	{
		for ( const auto& rp : data )
		{
			out += fmt::format("{0}{2}{1}{2}", std::to_string( rp.first ), std::to_string( rp.second ), seperator);
		}
		out.pop_back();
	}
	return out;
}

absl::flat_hash_map<int, int> Util::mapSplit( QString data, QString seperator )
{
	absl::flat_hash_map<int, int> m;
	if ( !data.isEmpty() )
	{
		QStringList dl = data.split( seperator );
		for ( int i = 0; i < dl.size(); i += 2 )
		{
			m.insert_or_assign( dl[i].toInt(), dl[i + 1].toInt() );
		}
	}
	return m;
}

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

QPair<int, int> Util::pixelOffset( const Position& pos )
{
	return Util::pixelOffset( pos.x, pos.y, pos.z );
}

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

PixmapPtr Util::smallPixmap( Sprite* sprite, const std::string& season, int rotation )
{
	PixmapPtr result(nullptr, SDL_FreeSurface);

	if ( !sprite )
	{
		return result;
	}
	auto* pm = sprite->pixmap( season, 0, 0 );

	SDL_Surface* newImg = createPixmap( 32, 32 );

	int firstLine = 0;
	int lastLine  = 63;

	for ( int y = 0; y < 64; ++y )
	{
		bool found = false;
		for ( int x = 0; x < 32; ++x )
		{
			if ( pm->h <= y )
				spdlog::debug( "Util::smallPixmap 1 {}", sprite->sID );

			SDL_Color col = getPixelColor( pm, x, y );
			if ( col.r + col.g + col.b > 0 )
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
			if ( pm->h <= y )
				spdlog::debug( "Util::smallPixmap 2 {}", sprite->sID );
			auto col = getPixelColor( pm, x, y );
			if ( col.r + col.g + col.b > 0 )
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
	SDL_Color col1 { 0, 0, 0, 0 };
	SDL_FillRect(newImg, nullptr, 0x00000000);

	for ( int y = 0; y < 32 - yOff; ++y )
	{
		for ( int x = 0; x < 32; ++x )
		{
			if ( pm->h <= y )
				spdlog::debug( "Util::smallPixmap 3 {}", sprite->sID );
			const auto col = getPixelColor( pm, x, y + firstLine );
			setPixelColor( newImg, x, y + yOff, col );
		}
	}

	result.reset(newImg);
	return result;
}

QColor Util::colorInt2Color( unsigned int color )
{
	int a = color & 0xff;
	int b = ( color >> 8 ) & 0xff;
	int gg = ( color >> 16 ) & 0xff;
	int r = ( color >> 24 ) & 0xff;
	return QColor( r, gg, b, a );
}

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

QColor Util::string2QColor( QString color )
{
	QList<QString> csl = color.split( ' ' );
	if ( csl.size() == 4 )
	{
		return QColor( csl[0].toInt(), csl[1].toInt(), csl[2].toInt(), csl[3].toInt() );
	}
	return QColor();
}

QColor Util::variant2QColor( QVariant color )
{
	QList<QString> csl = color.toString().split( ' ' );
	if ( csl.size() == 4 )
	{
		return QColor( csl[0].toInt(), csl[1].toInt(), csl[2].toInt(), csl[3].toInt() );
	}
	return QColor();
}

unsigned int Util::createRawMaterialItem( Position pos, unsigned int materialID )
{
	//create items from the wall material
	std::string materialSID = "None";
	QString type        = "None";
	materialSID         = DBH::materialSID( materialID );
	type                = Util::materialType( QString::fromStdString(materialSID) );

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

int Util::ticksPerDayRandomized( int percentage )
{
	int maxOffset = qMax( 1, (int)( (float)Util::ticksPerDay / 100. ) * percentage );
	return Util::ticksPerDay + ( ( rand() % ( maxOffset * 2 ) ) - maxOffset );
}

unsigned int Util::reverseFib( unsigned int number )
{
	if ( number < Global::xpMod )
		return 1;

	double num = (double)number / Global::xpMod;

	//n(F) = Floor[ Log(F Sqrt(5) + 1/2)/Log(Phi)]
	return floor( log( num * 2.2360679775 + 0.5 ) / log( 1.6180339887 ) );
}

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

QString Util::randomMetal()
{
	QStringList metals = { "Copper", "Tin", "Malachite", "Iron", "Lead", "Silver", "Gold", "Platinum" };
	if ( metals.size() > 0 )
	{
		return metals.at( rand() % metals.size() );
	}
	return "";
}

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

QString Util::getRandomString( int length )
{
	const QString possibleCharacters( "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789" );
	srand( std::chrono::system_clock::now().time_since_epoch().count() );
	QString randomString;
	for ( int i = 0; i < length; ++i )
	{
		int index      = qrand() % possibleCharacters.length();
		QChar nextChar = possibleCharacters.at( index );
		randomString.append( nextChar );
	}
	return randomString;
}

QVariantList Util::uintList2Variant( const QList<unsigned int>& list )
{
	QVariantList out;
	for ( auto ui : list )
	{
		out.append( ui );
	}
	return out;
}

QList<unsigned int> Util::variantList2UInt( const QVariantList& vlist )
{
	QList<unsigned int> out;
	for ( const auto& vui : vlist )
	{
		out.append( vui.toUInt() );
	}
	return out;
}

QVariantList Util::positionList2Variant( const QList<Position>& list )
{
	QVariantList out;
	for ( const auto& pos : list )
	{
		out.append( pos.toString() );
	}
	return out;
}

QList<Position> Util::variantList2Position( const QVariantList& vlist )
{
	QList<Position> out;
	for ( const auto& vpos : vlist )
	{
		out.append( Position( vpos ) );
	}
	return out;
}

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

void Util::debugVM( QVariantMap vm, QString name )
{
	spdlog::debug( "{}", name.toStdString() );
	for ( const auto& key : vm.keys() )
	{
		qDebug() << key << ":" << vm.value( key ).toString();
	}
}

PixmapPtr Util::createWorkshopImage( const std::string& workshopID, const std::vector<std::string>& mats )
{
	PixmapPtr result(nullptr, SDL_FreeSurface);

	auto dbws = DB::workshop( workshopID );

	if( !dbws )
	{
		result.reset( createPixmap( 100, 100 ) );
		SDL_FillRect( result.get(), nullptr, 0x00000000 );
		return result;
	}

	if ( dbws->Icon )
	{
		const auto path = fs::path(Global::cfg->get<std::string>( "dataPath" )) / "xaml" / "buttons" / *dbws->Icon;
		result.reset( IMG_Load( path.c_str() ) );
		assert( result->w > 0 );
		return result;
	}
	const auto season = GameState::seasonString.toStdString();

	result.reset( createPixmap( 100, 100 ) );
	SDL_FillRect( result.get(), nullptr, 0x00000000 );

	int x0 = 34;
	int y0 = 0;

	for ( int y = 0; y < 3; ++y )
	{
		for ( int x = 0; x < 3; ++x )
		{
			unsigned char rot = 0;
			Sprite* sprite    = getSprite( x - 1, y - 1, dbws->components, rot, mats );
			if ( sprite )
			{
				const int px      = x0 + 16 * x - 16 * y;
				const int py      = y0 + 8 * y + 8 * x;

				auto* srcSurface = sprite->pixmap( season, rot, 0 );
				//painter.drawPixmap( px + sprite->xOffset, py + sprite->yOffset, sprite->pixmap( season, rot ) );
				copyPixmap( result.get(), srcSurface, px, py );
			}
		}
	}

	return result;
}

PixmapPtr Util::createItemImage( const std::string& itemID, const std::vector<std::string>& mats )
{
	auto vsprite = DB::select( "SpriteID", "Items", QString::fromStdString(itemID) );
	DBS::Workshop_Component component;

	std::vector<DBS::Workshop_Component> comps;
	comps.emplace_back( DBS::Workshop_Component {
		.Offset   = Position( 0, 0, 0 ),
		.SpriteID = vsprite.toString().toStdString(),
	} );

	const auto season = GameState::seasonString.toStdString();

	PixmapPtr result( createPixmap( 100, 100 ), SDL_FreeSurface );
	SDL_FillRect( result.get(), nullptr, 0x00000000 );

	int x0 = 34;
	int y0 = 0;

	for ( int y = 0; y < 3; ++y )
	{
		for ( int x = 0; x < 3; ++x )
		{
			unsigned char rot = 0;
			Sprite* sprite    = getSprite( x - 1, y - 1, comps, rot, mats );
			if ( sprite )
			{
				const int px      = x0 + 16 * x - 16 * y;
				const int py      = y0 + 8 * y + 8 * x;
				auto* srcSurface = sprite->pixmap( season, rot, 0 );
				//painter.drawPixmap( px + sprite->xOffset, py + sprite->yOffset, sprite->pixmap( season, rot ) );
				copyPixmap( result.get(), srcSurface, px, py );
			}
		}
	}
	return result;
}

PixmapPtr Util::createItemImage2( const std::string&itemID, const std::vector<std::string>& mats )
{
	auto spriteID = DB::select( "SpriteID", "Items", QString::fromStdString(itemID) ).toString();
	
	const auto season = GameState::seasonString.toStdString();

	PixmapPtr result(createPixmap( 32, 32 ), SDL_FreeSurface);
	SDL_FillRect( result.get(), nullptr, 0x00000000 );

	int x0 = 0;
	int y0 = 0;

	Sprite* sprite = g->sf()->createSprite( spriteID.toStdString(), mats ) ;
	if ( sprite )
	{
		auto* srcSurface = sprite->pixmap( season, 0, 0 );
		//painter.drawPixmap( px + sprite->xOffset, py + sprite->yOffset, sprite->pixmap( season, rot ) );
		copyPixmap( result.get(), srcSurface, 0, -16 );
	}
	return result;
}

PixmapPtr Util::createConstructionImage( const std::string& constructionID, const std::vector<std::string>& mats )
{
	auto sprites = DB::selectRows( "Constructions_Sprites", QString::fromStdString(constructionID) );
	// TODO: ranges::views::transform?
	std::vector<DBS::Constructions_Sprites> comp;
	for ( const auto& item : sprites ) {
		comp.push_back( DBS::Constructions_Sprites::from( item ) );
	}

	const auto season = GameState::seasonString.toStdString();

	PixmapPtr result(createPixmap( 100, 100 ), SDL_FreeSurface);
	SDL_FillRect( result.get(), nullptr, 0x00000000 );

	int x0 = 34;
	int y0 = 0;

	for ( int y = 0; y < 3; ++y )
	{
		for ( int x = 0; x < 3; ++x )
		{
			unsigned char rot = 0;
			Sprite* sprite    = getSprite( x - 1, y - 1, comp, rot, mats );
			if ( sprite )
			{
				const int px      = x0 + 16 * x - 16 * y;
				const int py      = y0 + 8 * y + 8 * x;

				auto* srcSurface = sprite->pixmap( season, rot, 0 );
				//painter.drawPixmap( px + sprite->xOffset, py + sprite->yOffset, sprite->pixmap( season, rot ) );
				copyPixmap( result.get(), srcSurface, px, py );
			}
		}
	}
	return result;
}

Sprite* Util::getSprite( int x, int y, const std::vector<DBS::Workshop_Component>& comps, unsigned char& rot, const std::vector<std::string>& mats )
{
	std::vector<std::string> materialIDs;
	int mid = 0;
	for ( const auto& cm : comps )
	{
		const auto& pos = cm.Offset;
		if ( pos.x == x && pos.y == y )
		{
			if ( cm.MaterialItem )
			{
				for ( const auto& mat : *cm.MaterialItem | ranges::views::split( '|' ) )
				{
					materialIDs.push_back( mats[std::stoi( mat | ranges::to<std::string>() )] );
				}
			}
			else
			{
				materialIDs.push_back( mats[mid] );
			}
			if ( cm.WallRotation )
			{
				const auto& wallRot = *cm.WallRotation;
				if ( wallRot == "FR" )
					rot = 0;
				else if ( wallRot == "FL" )
					rot = 1;
				else if ( wallRot == "BL" )
					rot = 2;
				else if ( wallRot == "BR" )
					rot = 3;
			}

			if ( cm.SpriteID )
			{
				return g->sf()->createSprite( *cm.SpriteID, materialIDs );
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

Sprite* Util::getSprite( int x, int y, const std::vector<DBS::Constructions_Sprites>& comps, unsigned char& rot, const std::vector<std::string>& mats )
{
	std::vector<std::string> materialIDs;
	int mid = 0;
	for ( const auto& cm : comps )
	{
		const auto& pos = cm.Offset;
		if ( pos.x == x && pos.y == y )
		{
			materialIDs.push_back( mats[mid] );

			return g->sf()->createSprite( cm.SpriteID, materialIDs );
		}
		++mid;
	}
	return nullptr;
}

QStringList Util::possibleMaterialsForItem( QString itemSID )
{
	QVariantMap row = DB::selectRow( "Items", itemSID );
	return possibleMaterials( row.value( "AllowedMaterials" ).toString(), row.value( "AllowedMaterialTypes" ).toString() );
}

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

void Util::createBufferForNoesisImage( const SDL_Surface* pm, std::vector<unsigned char>& buffer )
{
	int w = pm->w;
	int h = pm->h;

	buffer.resize( w * h * 4, 0 );

	for ( int x = 0; x < w; ++x )
	{
		for ( int y = 0; y < h; ++y )
		{
			auto color                    = getPixelColor( pm, x, y );
			buffer[( x + y * w ) * 4]     = color.r;
			buffer[( x + y * w ) * 4 + 1] = color.g;
			buffer[( x + y * w ) * 4 + 2] = color.b;
			buffer[( x + y * w ) * 4 + 3] = 255;
		}
	}
}