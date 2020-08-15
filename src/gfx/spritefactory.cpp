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
#include "spritefactory.h"

#include "../base/config.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/io.h"
#include "../base/util.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QPainter>
#include <QPixmap>

SpriteFactory::SpriteFactory()
{
}

SpriteFactory::~SpriteFactory()
{
}

bool SpriteFactory::init()
{
	//m_pixmapSources.clear();
	m_baseSprites.clear();

	m_sprites.clear();
	m_sprites.append( nullptr );

	m_spriteIDs.clear();
	m_creatureSpriteIDs.clear();

	m_pixelData.clear();

	m_randomNumbers.clear();

	m_spriteDefinitions.clear();
	m_spriteDefVMs.clear();

	m_spriteCreations.clear();

	m_colors.clear();
	m_hairColors.clear();

	m_seasons = DB::ids( "Seasons" );

	auto matIds = DB::ids( "Materials" );
	for ( auto id : matIds )
	{
		m_materialTypes.insert( id, DB::select( "Type", "Materials", id ).toString() );
	}

	for ( auto color : DB::select2( "Color", "Materials", "Type", "Dye" ) )
	{
		m_colors.push_back( Util::string2QColor( color.toString() ) );
	}

	for ( auto color : DB::selectRows( "HairColors" ) )
	{
		m_hairColors.push_back( Util::string2QColor( color.value( "Color" ).toString() ) );
	}

	bool loaded = true;
	auto rows   = DB::selectRows( "BaseSprites" );
	for ( auto row : rows )
	{
		QString tilesheet = row.value( "Tilesheet" ).toString();
		if ( !m_pixmapSources.contains( tilesheet ) )
		{
			QPixmap pm;
			loaded = pm.load( Config::getInstance().get( "dataPath" ).toString() + "/tilesheet/" + tilesheet );
			if ( !loaded )
			{
				loaded = pm.load( tilesheet );
				if ( !loaded )
				{
					qDebug() << "SpriteFactory: failed to load " << tilesheet;
					return false;
				}
			}
			m_pixmapSources.insert( tilesheet, pm );
		}
		m_baseSprites.insert( row.value( "ID" ).toString(), extractPixmap( tilesheet, row ) );
	}

	QList<QVariantMap> spriteList = DB::selectRows( "Sprites" );
	for ( auto& sprite : spriteList )
	{
		QString spriteID = sprite.value( "ID" ).toString();

		if ( sprite.value( "BaseSprite" ).toString().isEmpty() )
		{
			sprite.insert( "BaseSprite", spriteID );
		}
		if ( sprite.value( "Tint" ).toString().isEmpty() )
		{
			sprite.remove( "Tint" );
		}
		if ( sprite.value( "DefaultMaterial" ).toString().isEmpty() )
		{
			sprite.remove( "DefaultMaterial" );
		}

		if ( DB::numRows( "Sprites_ByMaterials", spriteID ) )
		{
			auto rows = DB::selectRows( "Sprites_ByMaterials", spriteID );
			QVariantList byMaterials;
			for ( auto entry : rows )
			{
				if ( entry.value( "BaseSprite" ).toString().isEmpty() )
				{
					entry.remove( "BaseSprite" );
				}
				if ( entry.value( "Sprite" ).toString().isEmpty() )
				{
					entry.remove( "Sprite" );
				}
				byMaterials.append( entry );
			}
			sprite.remove( "BaseSprite" );
			sprite.insert( "ByMaterials", byMaterials );
		}
		if ( DB::numRows( "Sprites_ByMaterialTypes", spriteID ) )
		{
			auto rows = DB::selectRows( "Sprites_ByMaterialTypes", spriteID );
			QVariantList byMaterialTypes;
			for ( auto entry : rows )
			{
				if ( entry.value( "BaseSprite" ).toString().isEmpty() )
				{
					entry.remove( "BaseSprite" );
				}
				if ( entry.value( "Sprite" ).toString().isEmpty() )
				{
					entry.remove( "Sprite" );
				}

				byMaterialTypes.append( entry );
			}
			sprite.remove( "BaseSprite" );
			sprite.insert( "ByMaterialTypes", byMaterialTypes );
		}
		if ( DB::numRows( "Sprites_Combine", spriteID ) )
		{
			auto rows = DB::selectRows( "Sprites_Combine", spriteID );
			QVariantList combine;
			for ( auto entry : rows )
			{
				QVariantMap cm;
				if ( entry.value( "BaseSprite" ).toString().isEmpty() )
				{
					cm.insert( "Sprite", entry.value( "Sprite" ).toString() );
				}
				else
				{
					cm.insert( "BaseSprite", entry.value( "BaseSprite" ).toString() );
				}
				if ( !entry.value( "Tint" ).toString().isEmpty() )
				{
					cm.insert( "Tint", entry.value( "Tint" ).toString() );
				}
				if ( !entry.value( "Offset" ).toString().isEmpty() )
				{
					cm.insert( "Offset", entry.value( "Offset" ).toString() );
				}

				combine.append( cm );
			}
			sprite.remove( "BaseSprite" );
			sprite.insert( "Combine", combine );
		}
		if ( DB::numRows( "Sprites_Frames", spriteID ) )
		{
			auto rows = DB::selectRows( "Sprites_Frames", spriteID );
			QVariantList frames;
			for ( auto entry : rows )
			{
				QVariantMap cm;
				cm.insert( "BaseSprite", entry.value( "BaseSprite" ).toString() );
				frames.append( cm );
			}
			sprite.remove( "BaseSprite" );
			sprite.insert( "Frames", frames );
		}
		if ( DB::numRows( "Sprites_Random", spriteID ) )
		{
			auto rows = DB::selectRows( "Sprites_Random", spriteID );
			QVariantList random;
			for ( auto entry : rows )
			{
				QVariantMap cm;
				if ( entry.value( "BaseSprite" ).toString().isEmpty() )
				{
					cm.insert( "Sprite", entry.value( "Sprite" ).toString() );
				}
				else
				{
					cm.insert( "BaseSprite", entry.value( "BaseSprite" ).toString() );
				}
				cm.insert( "Weight", entry.value( "Weight" ).toInt() );

				random.append( cm );
			}
			sprite.remove( "BaseSprite" );
			sprite.insert( "Random", random );
		}
		if ( DB::numRows( "Sprites_Rotations", spriteID ) )
		{
			auto rows = DB::selectRows( "Sprites_Rotations", spriteID );
			QVariantList rotations;
			for ( auto entry : rows )
			{
				QVariantMap cm;
				if ( entry.value( "BaseSprite" ).toString().isEmpty() )
				{
					cm.insert( "Sprite", entry.value( "Sprite" ).toString() );
				}
				else
				{
					cm.insert( "BaseSprite", entry.value( "BaseSprite" ).toString() );
				}
				if ( !entry.value( "Effect" ).toString().isEmpty() )
				{
					cm.insert( "Effect", entry.value( "Effect" ).toString() );
				}
				cm.insert( "Rotation", entry.value( "Rotation" ).toString() );

				rotations.append( cm );
			}
			sprite.remove( "BaseSprite" );
			sprite.insert( "Rotations", rotations );
		}
		if ( DB::numRows( "Sprites_Seasons", spriteID ) )
		{
			auto rows = DB::selectRows( "Sprites_Seasons", spriteID );
			QVariantList seasons;
			for ( auto entry : rows )
			{
				QVariantMap cm;
				if ( !entry.value( "BaseSprite" ).toString().isEmpty() )
				{
					cm.insert( "BaseSprite", entry.value( "BaseSprite" ).toString() );
				}
				cm.insert( "Season", entry.value( "Season" ) );

				if ( DB::numRows( "Sprites_Seasons_Rotations", spriteID + entry.value( "Season" ).toString() ) )
				{
					auto crows = DB::selectRows( "Sprites_Seasons_Rotations", spriteID + entry.value( "Season" ).toString() );
					QVariantList rots;
					for ( auto centry : crows )
					{
						rots.append( centry );
					}
					cm.insert( "Rotations", rots );
				}

				seasons.append( cm );
			}
			sprite.remove( "BaseSprite" );
			sprite.insert( "Seasons", seasons );
		}
	}

	QJsonArray ja;
	for ( auto sprite : spriteList )
	{
		QJsonValue jv = QJsonValue::fromVariant( sprite );
		ja.append( jv );
	}

	IO::saveFile( "spriteconv.json", ja );

	for ( auto row : spriteList )
	{
		m_spriteDefVMs.insert( row.value( "ID" ).toString(), row );
	}

	for ( auto row : spriteList )
	{
		DefNode* root = new DefNode;
		root->type    = "root";
		root->value   = row.value( "ID" ).toString();
		m_numFrames   = 1;
		parseDef( root, row );
		root->numFrames = m_numFrames;
		if ( root->childs.isEmpty() && root->baseSprite.isEmpty() )
		{
			root->baseSprite = root->value;
		}
		m_spriteDefinitions.insert( root->value, root );
	}

	createStandardSprites();

	return loaded;
}

void SpriteFactory::addPixmapSource( QString name, QString path )
{
	if ( !m_pixmapSources.contains( name ) )
	{
		QPixmap pm;
		bool loaded = pm.load( path );
		if ( loaded )
		{
			m_pixmapSources.insert( name, pm );
		}
	}
}

void SpriteFactory::createStandardSprites()
{
	m_thoughtBubbleIDs.insert( "Tired", createSprite( "StatusTired", { "None" } )->uID );
	m_thoughtBubbleIDs.insert( "Sleeping", createSprite( "StatusSleeping", { "None" } )->uID );
	m_thoughtBubbleIDs.insert( "Thirsty", createSprite( "ThoughtBubbleThirsty", { "None" } )->uID );
	m_thoughtBubbleIDs.insert( "Hungry", createSprite( "ThoughtBubbleHungry", { "None" } )->uID );
	m_thoughtBubbleIDs.insert( "Combat", createSprite( "ThoughtBubbleCombat", { "None" } )->uID );

	createSprite( "SolidSelectionWall", { "Purple" } );

	Sprite* sprite         = createSprite( "WaterFloor", { "Water" } );
	Global::waterSpriteUID = sprite->uID;

	createSprite( "WaterRightWall_", { "Water" } );
	createSprite( "WaterLeftWall_", { "Water" } );

	Sprite* sprite2         = createSprite( "SolidSelectionWall", { "Grey" } );
	Global::undiscoveredUID = sprite2->uID;
	createSprite( "SolidSelectionFloor", { "Grey" } );
	createSprite( "RoughFloor", { "Dirt" } );

	// keep a number of sprite ID's for future sprites that are created at start
	// when adding a new sprite here decrease the buffer
	while ( m_sprites.size() < 31 )
	{
		m_sprites.append( nullptr );
	}
}

QPixmap SpriteFactory::extractPixmap( QString sourcePNG, QVariantMap def )
{
	QString rect = def.value( "SourceRectangle" ).toString();

	QStringList rl = rect.split( " " );
	if ( rl.size() == 4 )
	{
		int x    = rl[0].toInt();
		int y    = rl[1].toInt();
		int dimX = rl[2].toInt();
		int dimY = rl[3].toInt();

		QPixmap p = m_pixmapSources[sourcePNG].copy( x, y, dimX, dimY );
		p.setMask( p.createMaskFromColor( QColor( 255, 0, 255 ), Qt::MaskInColor ) );

		return p;
	}
	qDebug() << "***ERROR*** extractPixmap() for " << def.value( "ID" ).toString();
	return QPixmap();
}

unsigned char SpriteFactory::rotationToChar( const QString suffix )
{
	SpriteRotation rot = SpriteRotation::FR;
	if ( suffix == "FL" )
		rot = SpriteRotation::FL;
	else if ( suffix == "BL" )
		rot = SpriteRotation::BL;
	else if ( suffix == "BR" )
		rot = SpriteRotation::BR;
	return rot;
}

Sprite* SpriteFactory::createSprite( const QString itemSID, QStringList materialSIDs, const QMap<int, int>& random )
{
#if DEBUG
	for ( auto& matString : materialSIDs )
	{
		if ( matString == "None" )
		{
			qDebug() << "#" << itemSID << materialSIDs;
		}
	}
#endif
	QString key = itemSID;
	m_randomNumbers.clear();
	if ( random.isEmpty() )
	{
		if ( containsRandom( itemSID, materialSIDs ) )
		{
			key = createSpriteMaterialDryRun( itemSID, materialSIDs );
		}
		else
		{
			for ( auto mat : materialSIDs )
			{
				key += "_";
				key += mat;
			}
		}
	}
	else
	{
		m_randomNumbers = random;

		for ( auto mat : materialSIDs )
		{
			key += "_";
			key += mat;
		}

		if ( containsRandom( itemSID, materialSIDs ) )
		{

			for ( auto r : m_randomNumbers )
			{
				key += "_";
				key += QString::number( r );
			}
		}
	}
	Sprite* sprite = nullptr;
	if ( m_spriteIDs.contains( key ) )
	{
		sprite = m_sprites.value( m_spriteIDs.value( key ) );
	}
	else
	{
		bool isClient = Config::getInstance().get( "IsClient" ).toBool();
		if ( !isClient )
		{
			sprite = createSpriteMaterial( itemSID, materialSIDs, key );

			if ( !sprite )
			{
				return m_sprites.value( m_spriteIDs.value( "SolidSelectionWall_Purple" ) );
			}

			sprite->uID = m_sprites.size();
			m_spriteIDs.insert( key, m_sprites.size() );
			m_sprites.append( sprite );

			addPixmapToPixelData( sprite );

			m_textureAdded = true;
			SpriteCreation sc { itemSID, materialSIDs, m_randomNumbers, sprite->uID };
			m_spriteCreations.push_back( sc );

			if ( sprite->anim )
			{
				m_sprites.append( nullptr );
				m_sprites.append( nullptr );
				m_sprites.append( nullptr );
			}
		}
		QString ncd = itemSID + ";" + materialSIDs.join( '_' ) + ";";
		if ( random.isEmpty() )
		{
			GameState::addChange2( NetworkCommand::CREATESPRITE, ncd );
		}
		else
		{
			ncd += Util::mapJoin( random );
			GameState::addChange2( NetworkCommand::CREATESPRITERANDOM, ncd );
		}
		if ( isClient )
		{
			return nullptr;
		}
	}

	if ( itemSID.endsWith( "Wall" ) )
	{
		createSprite( itemSID + "Short", materialSIDs, random );
	}
	/*
	if( itemSID.startsWith( "GrassSoilU" ) )
	{
		QImage img_ = sprite->pixmap( "Spring", 0 ).toImage();
		img_.save( "d:/tmp/gnome/" + key +  ".png", "PNG" );
	}
	*/

	return sprite;
}

Sprite* SpriteFactory::createSprite2( const QString itemSID, QStringList materialSIDs, const QMap<int, int>& random )
{
	QString key = itemSID;
	m_randomNumbers.clear();
	if ( random.isEmpty() )
	{
		if ( containsRandom( itemSID, materialSIDs ) )
		{
			key = createSpriteMaterialDryRun( itemSID, materialSIDs );
		}
		else
		{
			for ( auto mat : materialSIDs )
			{
				key += "_";
				key += mat;
			}
		}
	}
	else
	{
		m_randomNumbers = random;

		for ( auto mat : materialSIDs )
		{
			key += "_";
			key += mat;
		}

		if ( containsRandom( itemSID, materialSIDs ) )
		{

			for ( auto r : m_randomNumbers )
			{
				key += "_";
				key += QString::number( r );
			}
		}
	}
	Sprite* sprite = nullptr;

	bool isClient = Config::getInstance().get( "IsClient" ).toBool();

	sprite = createSpriteMaterial( itemSID, materialSIDs, key );

	if ( !sprite )
	{
		return m_sprites.value( m_spriteIDs.value( "SolidSelectionWall_Purple" ) );
	}

	sprite->uID = m_sprites.size();
	m_spriteIDs.insert( key, m_sprites.size() );
	m_sprites.append( sprite );

	addPixmapToPixelData( sprite );

	m_textureAdded = true;
	SpriteCreation sc { itemSID, materialSIDs, m_randomNumbers, sprite->uID };
	m_spriteCreations.push_back( sc );

	if ( sprite->anim )
	{
		m_sprites.append( nullptr );
		m_sprites.append( nullptr );
		m_sprites.append( nullptr );
	}

	QString ncd = itemSID + ";" + materialSIDs.join( '_' ) + ";";
	if ( random.isEmpty() )
	{
		GameState::addChange2( NetworkCommand::CREATESPRITE, ncd );
	}
	else
	{
		ncd += Util::mapJoin( random );
		GameState::addChange2( NetworkCommand::CREATESPRITERANDOM, ncd );
	}

	return sprite;
}

Sprite* SpriteFactory::createAnimalSprite( const QString spriteSID, const QMap<int, int>& random )
{
	QString key = spriteSID;
	m_randomNumbers.clear();
	if ( random.isEmpty() )
	{
		if ( containsRandom( spriteSID, { "None" } ) )
		{
			key = createSpriteMaterialDryRun( spriteSID, { "None" } );
		}
	}
	else
	{
		m_randomNumbers = random;

		key += "_None";

		for ( auto r : m_randomNumbers )
		{
			key += "_";
			key += QString::number( r );
		}
	}

	Sprite* sprite = nullptr;
	if ( m_spriteIDs.contains( key ) )
	{
		sprite = m_sprites.value( m_spriteIDs.value( key ) );
	}
	else
	{
		sprite = createSpriteMaterial( spriteSID, { "None" }, key );

		sprite->uID = m_sprites.size();
		m_spriteIDs.insert( key, m_sprites.size() );
		m_sprites.append( sprite );

		addPixmapToPixelData( sprite );

		m_creatureTextureAdded = true;

		SpriteCreation sc { spriteSID, { "None" }, m_randomNumbers, sprite->uID, 1 };
		m_spriteCreations.push_back( sc );
	}

	return sprite;
}

Sprite* SpriteFactory::createSpriteNetwork( const QString itemSID, const QStringList materialSIDs, const QMap<int, int>& random )
{
	return nullptr;
}

bool SpriteFactory::containsRandom( const QString itemSID, const QStringList materialSIDs )
{
	QString spriteSID = DBH::spriteID( itemSID );
	if ( spriteSID.isEmpty() )
	{
		spriteSID = itemSID;
	}
	return DBH::spriteIsRandom( spriteSID );
}

bool SpriteFactory::createSpriteDefinition( QString spriteID )
{
	return true;
}

QString SpriteFactory::createSpriteMaterialDryRun( const QString itemSID, const QStringList materialSIDs )
{
	QString spriteSID = DBH::spriteID( itemSID );
	if ( spriteSID.isEmpty() )
	{
		// every item needs a SpriteID
		//qDebug() << "***ERROR*** item " << itemSID << " has no SpriteID entry.";
		spriteSID = itemSID;
	}

	if ( !m_spriteDefinitions.contains( spriteSID ) )
	{
		qDebug() << "***ERROR*** sprite definition " << spriteSID << " for item " << itemSID << " doesn't exist.";
		//exit( 0 );
	}
	DefNode* dn = m_spriteDefinitions.value( spriteSID );
	if ( dn )
	{
		for ( auto season : m_seasons )
		{
			for ( int i = 0; i < dn->numFrames; ++i )
			{
				getBaseSpriteDryRun( dn, itemSID, materialSIDs, season, "FR", i );
				getBaseSpriteDryRun( dn, itemSID, materialSIDs, season, "FL", i );
				getBaseSpriteDryRun( dn, itemSID, materialSIDs, season, "BL", i );
				getBaseSpriteDryRun( dn, itemSID, materialSIDs, season, "BR", i );
			}
		}
	}
	QString key = itemSID;
	for ( auto mat : materialSIDs )
	{
		key += "_";
		key += mat;
	}
	for ( auto r : m_randomNumbers )
	{
		key += "_";
		key += QString::number( r );
	}
	return key;
}

Sprite* SpriteFactory::createSpriteMaterial( const QString itemSID, const QStringList materialSIDs, const QString key )
{
	QString spriteSID = DBH::spriteID( itemSID );
	if ( spriteSID.isEmpty() )
	{
		// every item needs a SpriteID
		//qDebug() << "***ERROR*** item " << itemSID << " has no SpriteID entry.";
		spriteSID = itemSID;
	}

	if ( !m_spriteDefinitions.contains( spriteSID ) )
	{
		qDebug() << "***ERROR*** sprite definition " << spriteSID << " for item " << itemSID << " doesn't exist.";
		//exit( 0 );
	}
	DefNode* dn    = m_spriteDefinitions.value( spriteSID );
	Sprite* sprite = nullptr;
	if ( dn )
	{
		m_offset = "";

		sprite = getBaseSprite( dn, itemSID, materialSIDs );

		if ( !m_offset.isEmpty() )
		{
			QStringList osl = m_offset.split( " " );
			if ( osl.size() == 2 )
			{
				sprite->xOffset = osl[0].toInt();
				sprite->yOffset = osl[1].toInt();
			}
		}
		else
		{
			sprite->xOffset = 0;
			sprite->yOffset = 0;
		}

		sprite->opacity       = m_opacity;
		sprite->randomNumbers = m_randomNumbers;
		sprite->anim          = DB::select( "Anim", "Sprites", spriteSID ).toBool();
	}
	return sprite;
}

void SpriteFactory::parseDef( DefNode* parent, QVariantMap def )
{
	if ( def.contains( "Sprite" ) )
	{
		QString spriteID = def.value( "Sprite" ).toString();
		QVariant tint    = def.value( "Tint" );
		QVariant effect  = def.value( "Effect" );
		QVariant offset  = def.value( "Offset" );
		def              = m_spriteDefVMs.value( spriteID );
		if ( tint.isValid() )
		{
			def.insert( "Tint", tint );
		}
		if ( effect.isValid() )
		{
			def.insert( "Effect", effect );
		}
	}

	parent->effect          = def.value( "Effect" ).toString();
	parent->offset          = def.value( "Offset" ).toString();
	parent->tint            = def.value( "Tint" ).toString();
	parent->baseSprite      = def.value( "BaseSprite" ).toString();
	parent->defaultMaterial = def.value( "DefaultMaterial" ).toString();

	/*
	if( def.contains( "ByItems" ) )
	{
		for( auto item : def.value( "ByItems" ).toList() )
		{
			DefNode* child = new DefNode;
			child->depth = parent->depth + 1;
			child->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
			child->type = "ByItem";
			child->value = item.toMap().value( "ItemID" ).toString();
			parent->childs.insert( child->value, child );
			parseDef( child, item.toMap() );
		}
	}
	*/
	if ( def.contains( "ByMaterials" ) )
	{
		for ( auto item : def.value( "ByMaterials" ).toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = parent->depth + 1;
			child->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
			child->type     = "ByMaterial";
			child->value    = item.toMap().value( "MaterialID" ).toString();
			parent->childs.insert( child->value, child );
			parseDef( child, item.toMap() );
		}
	}
	if ( def.contains( "ByMaterialTypes" ) )
	{
		for ( auto item : def.value( "ByMaterialTypes" ).toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = parent->depth + 1;
			child->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
			child->type     = "ByMaterialType";
			child->value    = item.toMap().value( "MaterialType" ).toString();
			parent->childs.insert( child->value, child );
			parseDef( child, item.toMap() );
		}
	}
	if ( def.contains( "Seasons" ) )
	{
		for ( auto item : def.value( "Seasons" ).toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = parent->depth + 1;
			child->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
			child->type     = "Season";
			child->value    = item.toMap().value( "Season" ).toString();
			parent->childs.insert( child->value, child );
			parseDef( child, item.toMap() );
		}
	}
	if ( def.contains( "Rotations" ) )
	{
		for ( auto item : def.value( "Rotations" ).toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = parent->depth + 1;
			child->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
			child->type     = "Rotation";
			child->value    = item.toMap().value( "Rotation" ).toString();
			parent->childs.insert( child->value, child );
			parseDef( child, item.toMap() );
		}
	}
	if ( def.contains( "Frames" ) )
	{
		int index   = 0;
		m_numFrames = def.value( "Frames" ).toList().size();
		for ( auto item : def.value( "Frames" ).toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = parent->depth + 1;
			child->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
			child->type     = "Frame";
			parent->childs.insert( "Frame" + QString::number( index++ ), child );
			parseDef( child, item.toMap() );
		}
	}
	if ( def.contains( "Random" ) )
	{
		int index            = 0;
		DefNode* randomNode  = new DefNode;
		randomNode->type     = "RandomNode";
		randomNode->depth    = parent->depth + 1;
		randomNode->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
		parent->childs.insert( "RandomNode", randomNode );

		for ( auto item : def.value( "Random" ).toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = randomNode->depth + 1;
			child->childPos = randomNode->childPos + QString::number( randomNode->childs.size() + 1 );
			child->type     = "Random";
			randomNode->childs.insert( "Random" + QString::number( index++ ), child );
			randomNode->randomWeights.push_back( item.toMap().value( "Weight" ).toInt() );
			parseDef( child, item.toMap() );
		}
	}

	if ( def.contains( "Combine" ) )
	{
		int index             = 0;
		DefNode* combineNode  = new DefNode;
		combineNode->type     = "CombineNode";
		combineNode->depth    = parent->depth + 1;
		combineNode->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
		parent->childs.insert( "CombineNode", combineNode );

		for ( auto item : def.value( "Combine" ).toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = parent->depth + 1;
			child->childPos = combineNode->childPos + QString::number( combineNode->childs.size() + 1 );
			child->type     = "Combine";
			combineNode->childs.insert( "Combine" + QString::number( index++ ), child );
			parseDef( child, item.toMap() );
		}
	}
}

Sprite* SpriteFactory::getBaseSprite( const DefNode* node, const QString itemSID, const QStringList materialSIDs, int materialID )
{
	if ( !node->offset.isEmpty() )
	{
		m_offset = node->offset;
	}
	materialID          = qMin( materialID, materialSIDs.size() - 1 );
	QString materialSID = materialSIDs[materialID];
	if ( !node->baseSprite.isEmpty() )
	{
		QPixmap pm           = m_baseSprites.value( node->baseSprite );
		SpritePixmap* sprite = new SpritePixmap( pm, m_offset );
		sprite->applyEffect( node->effect );
		sprite->applyTint( node->tint, materialSID );
		return sprite;
	}

	if ( node->childs.size() && node->childs.first()->type == "Rotation" )
	{
		SpriteRotations* sr = new SpriteRotations;

		sr->m_sprites.push_back( getBaseSprite( node->childs.value( "FR" ), itemSID, materialSIDs ) );
		sr->m_sprites.push_back( getBaseSprite( node->childs.value( "FL" ), itemSID, materialSIDs ) );
		sr->m_sprites.push_back( getBaseSprite( node->childs.value( "BL" ), itemSID, materialSIDs ) );
		sr->m_sprites.push_back( getBaseSprite( node->childs.value( "BR" ), itemSID, materialSIDs ) );

		sr->applyEffect( node->effect );
		sr->applyTint( node->tint, materialSID );
		return sr;
	}
	if ( node->childs.size() && node->childs.first()->type == "Season" )
	{
		SpriteSeasons* ss = new SpriteSeasons;
		for ( auto child : node->childs )
		{
			ss->m_sprites.insert( child->value, getBaseSprite( child, itemSID, materialSIDs ) );
		}
		ss->applyEffect( node->effect );
		ss->applyTint( node->tint, materialSID );
		return ss;
	}
	if ( node->childs.contains( itemSID ) )
	{
		return getBaseSprite( node->childs[itemSID], itemSID, materialSIDs );
	}
	if ( node->childs.size() && node->childs.first()->type == "Frame" )
	{
		SpriteFrames* sf = new SpriteFrames;
		for ( auto child : node->childs )
		{
			sf->m_sprites.push_back( getBaseSprite( child, itemSID, materialSIDs ) );
		}
		sf->applyEffect( node->effect );
		sf->applyTint( node->tint, materialSID );
		return sf;
	}

	if ( node->type == "CombineNode" && node->childs.size() > 1 )
	{
		Sprite* s = getBaseSprite( node->childs["Combine0"], itemSID, materialSIDs, materialID );

		for ( int i = 1; i < node->childs.size(); ++i )
		{
			Sprite* s2 = getBaseSprite( node->childs["Combine" + QString::number( i )], itemSID, materialSIDs, materialID + i );
			for ( auto season : m_seasons )
			{
				s->combine( s2, season, 0, 0 );
				s->combine( s2, season, 1, 0 );
				s->combine( s2, season, 2, 0 );
				s->combine( s2, season, 3, 0 );

				s->combine( s2, season, 0, 1 );
				s->combine( s2, season, 1, 1 );
				s->combine( s2, season, 2, 1 );
				s->combine( s2, season, 3, 1 );

				s->combine( s2, season, 0, 2 );
				s->combine( s2, season, 1, 2 );
				s->combine( s2, season, 2, 2 );
				s->combine( s2, season, 3, 2 );

				s->combine( s2, season, 0, 3 );
				s->combine( s2, season, 1, 3 );
				s->combine( s2, season, 2, 3 );
				s->combine( s2, season, 3, 3 );
			}
		}
		return s;
	}
	if ( node->type == "RandomNode" )
	{
		int randomNumber = m_randomNumbers.value( node->childPos.toInt() );
		Sprite* rs       = getBaseSprite( node->childs["Random" + QString::number( randomNumber )], itemSID, materialSIDs );
		rs->applyEffect( node->childs["Random" + QString::number( randomNumber )]->effect );
		rs->applyTint( node->childs["Random" + QString::number( randomNumber )]->tint, materialSID );
		return rs;
	}

	if ( materialSIDs.size() )
	{
		QString materialType = getMaterialType( materialSID );
		if ( node->childs.contains( materialType ) )
		{
			Sprite* pm = getBaseSprite( node->childs[materialType], itemSID, materialSIDs );
			pm->applyEffect( node->effect );
			pm->applyTint( node->tint, materialSID );
			return pm;
		}

		QString materialSID = *materialSIDs.begin();
		if ( node->childs.contains( materialSID ) )
		{
			Sprite* pm = getBaseSprite( node->childs[materialSID], itemSID, materialSIDs );
			pm->applyEffect( node->effect );
			pm->applyTint( node->tint, materialSID );
			return pm;
		}
		// if no other hit
		if ( node->childs.size() )
		{
			if ( !node->defaultMaterial.isEmpty() && node->childs.contains( node->defaultMaterial ) )
			{
				Sprite* pm = getBaseSprite( node->childs[node->defaultMaterial], itemSID, materialSIDs );
				pm->applyEffect( node->effect );
				pm->applyTint( node->tint, materialSID );
				return pm;
			}
			else
			{
				Sprite* pm = getBaseSprite( node->childs.first(), itemSID, materialSIDs, materialID );
				pm->applyEffect( node->effect );
				pm->applyTint( node->tint, materialSID );
				return pm;
			}
		}
	}

	QPixmap pm           = m_baseSprites.value( "SolidSelectionWall" );
	SpritePixmap* sprite = new SpritePixmap( pm );
	sprite->applyTint( "Material", "JobPurple" );
	return sprite;
}

void SpriteFactory::getBaseSpriteDryRun( const DefNode* node, const QString itemSID, const QStringList materialSIDs, const QString season, const QString rotation, const int animFrame )
{
	QString materialSID = *materialSIDs.begin();
	if ( !node->baseSprite.isEmpty() )
	{
		return;
	}

	if ( node->childs.contains( rotation ) )
	{
		getBaseSpriteDryRun( node->childs[rotation], itemSID, materialSIDs, season, rotation, animFrame );
		return;
	}
	if ( node->childs.contains( season ) )
	{
		getBaseSpriteDryRun( node->childs[season], itemSID, materialSIDs, season, rotation, animFrame );
		return;
	}
	if ( node->childs.contains( itemSID ) )
	{
		getBaseSpriteDryRun( node->childs[itemSID], itemSID, materialSIDs, season, rotation, animFrame );
		return;
	}
	if ( node->childs.contains( "Frame" + QString::number( animFrame ) ) )
	{
		getBaseSpriteDryRun( node->childs["Frame" + QString::number( animFrame )], itemSID, materialSIDs, season, rotation, animFrame );
		return;
	}

	if ( node->type == "CombineNode" && node->childs.size() > 1 )
	{
		getBaseSpriteDryRun( node->childs["Combine0"], itemSID, materialSIDs, season, rotation, animFrame );

		for ( int i = 1; i < node->childs.size(); ++i )
		{
			getBaseSpriteDryRun( node->childs["Combine" + QString::number( i )], itemSID, materialSIDs, season, rotation, animFrame );
		}
		return;
	}
	if ( node->type == "RandomNode" )
	{
		QList<int> weights = node->randomWeights;
		//qDebug() << weights;
		int randomNumber = 0;
		if ( !m_randomNumbers.contains( node->childPos.toInt() ) )
		{
			if ( weights.contains( 0 ) )
			{
				randomNumber = rand() % node->childs.size();
				m_randomNumbers.insert( node->childPos.toInt(), randomNumber );
			}
			else
			{
				int sum = 0;
				for ( auto w : weights )
					sum += w;
				int ran   = rand() % sum;
				int total = 0;
				for ( int i = 0; i < weights.size(); ++i )
				{
					total += weights[i];
					if ( ran < total )
					{
						m_randomNumbers.insert( node->childPos.toInt(), i );
						break;
					}
				}
			}
		}
		randomNumber = m_randomNumbers.value( node->childPos.toInt() );

		getBaseSpriteDryRun( node->childs["Random" + QString::number( randomNumber )], itemSID, materialSIDs, season, rotation, animFrame );
		return;
	}

	if ( materialSIDs.size() )
	{
		QString materialType = getMaterialType( materialSID );
		if ( node->childs.contains( materialType ) )
		{
			getBaseSpriteDryRun( node->childs[materialType], itemSID, materialSIDs, season, rotation, animFrame );
			return;
		}

		QString materialSID = *materialSIDs.begin();
		if ( node->childs.contains( materialSID ) )
		{
			getBaseSpriteDryRun( node->childs[materialSID], itemSID, materialSIDs, season, rotation, animFrame );
			return;
		}
		// if no other hit
		if ( node->childs.size() )
		{
			getBaseSpriteDryRun( node->childs.first(), itemSID, materialSIDs, season, rotation, animFrame );
			return;
		}
	}

	return;
}

int SpriteFactory::numFrames( const DefNode* node, const QString itemSID, const QStringList materialSIDs, const QString season, const QString rotation )
{
	return 1;
}

QString SpriteFactory::getMaterialType( const QString materialSID )
{
	return m_materialTypes.value( materialSID );
}

Sprite* SpriteFactory::getSprite( const int id )
{
	if ( m_sprites.size() > id )
	{
		return m_sprites[id];
	}
	return nullptr;
}

void SpriteFactory::printDebug()
{
	for ( auto si : m_spriteIDs.toStdMap() )
	{
		qDebug() << si.first << " - " << si.second;
	}
}

void SpriteFactory::createSprites( QList<SpriteCreation> scl )
{
	//m_sprites.clear();
	//m_spriteIDs.clear();

	//createStandardSprites();

	for ( auto sc : scl )
	{
		if ( sc.uID > 30 )
		{
			if ( sc.uID != m_sprites.size() )
			{
				if ( sc.uID < m_sprites.size() )
				{
					qWarning() << "## Loosing sprite " << sc.uID << m_sprites.size() << sc.itemSID << sc.materialSIDs;
					continue;
				}
				else
				{
					qDebug() << "## Missing sprite before " << sc.uID << m_sprites.size() << sc.itemSID << sc.materialSIDs;
					while ( sc.uID > m_sprites.size() )
					{
						// Sprite was probably lost due to a deleted definition, ID won't be reused
						m_sprites.append( nullptr );
					}
				}
			}

			if ( sc.creatureID )
			{
				if ( sc.creatureID == 1 )
				{
					createAnimalSprite( sc.itemSID, sc.random );
				}
				else
				{
					m_creatureSpriteIDs.insert( sc.creatureID, sc.uID );
					m_sprites.append( nullptr );
				}
			}
			else
			{
				createSprite2( sc.itemSID, sc.materialSIDs, sc.random );
			}
		}
	}
	qDebug() << "Used" << m_texesUsed << "array textures";
	m_spriteCreations = scl;
}

void SpriteFactory::addPixmapToPixelData( Sprite* sprite )
{
	QString season = GameState::seasonString;
	if( season.isEmpty() )
	{
		season = "Spring";
	}
	if ( sprite->anim )
	{
		for ( int frame = 0; frame < 4; ++frame )
		{
			int tex     = ( sprite->uID + frame ) / 512;
			m_texesUsed = qMax( m_texesUsed, tex + 1 );
			int id      = ( sprite->uID + frame ) % 512;

			if ( m_pixelData.size() < tex + 1 )
			{
				int maxArrayTextures = Config::getInstance().get( "MaxArrayTextures" ).toInt();
				int bytes            = 32 * 64 * 4 * maxArrayTextures;

				for ( int i = 0; i < 32; ++i )
				{
					QVector<uint8_t> data( bytes );
					m_pixelData.push_back( data );
				}
			}

			for ( int i = 0; i < 4; ++i )
			{
				auto pm    = sprite->pixmap( season, i, frame );
				QImage img = pm.toImage();

				int startIndex = 8192 * ( 4 * id + i );

				if ( img.height() == 64 && img.width() == 32 )
				{
					auto data = m_pixelData[tex].data();
					for ( int y = 0; y < 64; ++y )
					{
						for ( int x = 0; x < 32; ++x )
						{
							QColor col = img.pixelColor( x, y );
							const size_t baseIndex = startIndex + ( x * 4 + 128 * y );
							data[baseIndex + 0]    = col.red();
							data[baseIndex + 1]    = col.green();
							data[baseIndex + 2]    = col.blue();
							data[baseIndex + 3]    = col.alpha();
						}
					}
				}
			}
		}
	}
	else
	{
		int tex     = sprite->uID / 512;
		m_texesUsed = qMax( m_texesUsed, tex + 1 );
		int id      = sprite->uID % 512;

		if ( m_pixelData.size() < tex + 1 )
		{
			int maxArrayTextures = Config::getInstance().get( "MaxArrayTextures" ).toInt();
			int bytes            = 32 * 64 * 4 * maxArrayTextures;

			for ( int i = 0; i < 32; ++i )
			{
				QVector<uint8_t> data( bytes );
				m_pixelData.push_back( data );
			}
		}

		for ( int i = 0; i < 4; ++i )
		{
			auto pm    = sprite->pixmap( season, i, 0 );
			QImage img = pm.toImage();

			int startIndex = 8192 * ( 4 * id + i );

			if ( img.height() == 64 && img.width() == 32 )
			{
				for ( int y = 0; y < 64; ++y )
				{
					for ( int x = 0; x < 32; ++x )
					{
						QColor col = img.pixelColor( x, y );
						m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y ), col.red() );
						m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y + 1 ), col.green() );
						m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y + 2 ), col.blue() );
						m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y + 3 ), col.alpha() );
					}
				}
			}
		}
	}
}

void SpriteFactory::addPixmapToPixelData32( Sprite* sprite )
{
	int tex        = sprite->uID / 512;
	int id         = sprite->uID % 512;
	QString season = GameState::seasonString;

	for ( int i = 0; i < 4; ++i )
	{
		auto pm    = sprite->pixmap( season, i, 0 );
		QImage img = pm.toImage();

		int startIndex = 8192 * ( 4 * id + i );

		for ( int x = 0; x < 32; ++x )
		{
			for ( int y = 0; y < 16; ++y )
			{
				m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y ), 0 );
				m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y + 1 ), 0 );
				m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y + 2 ), 0 );
				m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y + 3 ), 0 );
			}

			for ( int y = 16; y < 48; ++y )
			{
				QColor col = img.pixelColor( x, y - 16 );
				m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y ), col.red() );
				m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y + 1 ), col.green() );
				m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y + 2 ), col.blue() );
				m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y + 3 ), col.alpha() );
			}
			for ( int y = 48; y < 64; ++y )
			{
				m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y ), 0 );
				m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y + 1 ), 0 );
				m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y + 2 ), 0 );
				m_pixelData[tex].replace( startIndex + ( x * 4 + 128 * y + 3 ), 0 );
			}
		}
	}
}

void SpriteFactory::addEmptyRows( int startIndex, int rows, QVector<uint8_t>& pixelData )
{
	for ( int y = 0; y < rows; ++y )
	{
		for ( int x = 0; x < 32; ++x )
		{
			pixelData.replace( startIndex + ( x * 4 + 128 * y ), 0 );
			pixelData.replace( startIndex + ( x * 4 + 128 * y + 1 ), 0 );
			pixelData.replace( startIndex + ( x * 4 + 128 * y + 2 ), 0 );
			pixelData.replace( startIndex + ( x * 4 + 128 * y + 3 ), 0 );
		}
	}
}

bool SpriteFactory::textureAdded()
{
#if 1
	bool out       = m_textureAdded;
	m_textureAdded = false;
	return out;
#else
	if ( GameState::tick % 10 == 0 )
	{
		bool out       = m_textureAdded;
		m_textureAdded = false;
		return out;
	}
	return false;
#endif
}

bool SpriteFactory::creatureTextureAdded()
{
#if 1
	bool out               = m_creatureTextureAdded;
	m_creatureTextureAdded = false;
	return out;
#else
	if ( GameState::tick % 10 == 0 )
	{
		bool out               = m_creatureTextureAdded;
		m_creatureTextureAdded = false;
		return out;
	}
	return false;
#endif
}

void SpriteFactory::forceUpdate()
{
	for ( auto sprite : m_sprites )
	{
		if ( sprite )
		{
			addPixmapToPixelData( sprite );
		}
	}
	m_textureAdded = true;
}

QPixmap SpriteFactory::getTintedBaseSprite( QString baseSprite, QString material )
{
	if ( baseSprite.isEmpty() )
	{
		return m_baseSprites["EmptyWall"];
	}

	QPixmap pm = m_baseSprites[baseSprite];
	tintPixmap( pm, Util::string2QColor( DB::select( "Color", "Materials", material ).toString() ) );
	return pm;
}

Sprite* SpriteFactory::setCreatureSprite( const unsigned int creatureUID, QVariantList components, QVariantList componentsBack, bool isDead )
{
	QPixmap pmfr( 32, 32 );
	pmfr.fill( QColor( 0, 0, 0, 0 ) );
	QPainter painter( &pmfr );
	//qDebug() << " =================================================";
	for ( auto vcm : components )
	{
		auto cm = vcm.toMap();
		if ( cm.value( "Hidden" ).toBool() )
			continue;

		QString baseSprite = cm.value( "BaseSprite" ).toString();

		//if( !baseSprite.isEmpty() ) qDebug() << baseSprite << cm.value( "Tint" ).toString() << cm.value( "Material" ).toString();

		if ( cm.value( "HasBase" ).toBool() )
		{
			painter.drawPixmap( 0, 0, m_baseSprites[baseSprite + "Base"] );
		}

		QString tint = cm.value( "Tint" ).toString();
		bool isHair  = cm.value( "IsHair" ).toBool();
		if ( tint.isEmpty() )
		{
			painter.drawPixmap( 0, 0, m_baseSprites[baseSprite] );
		}
		else
		{
			if ( tint == "Material" )
			{
				auto pm = getTintedBaseSprite( baseSprite, cm.value( "Material" ).toString() );
				painter.drawPixmap( 0, 0, pm );
			}
			else
			{
				bool ok;
				int colorInt = tint.toInt( &ok );
				if ( ok )
				{
					QPixmap pm = m_baseSprites[baseSprite];
					if ( isHair )
					{
						tintPixmap( pm, m_hairColors[colorInt] );
					}
					else
					{
						tintPixmap( pm, m_colors[colorInt] );
					}
					painter.drawPixmap( 0, 0, pm );
				}
			}
		}
	}
	QImage img   = pmfr.toImage();
	QPixmap pmfl = QPixmap::fromImage( img.mirrored( true, false ) );

	QPixmap pmbr( 32, 32 );
	pmbr.fill( QColor( 0, 0, 0, 0 ) );
	QPainter painter2( &pmbr );
	//qDebug() << "---------------------------------";
	for ( auto vcm : componentsBack )
	{
		auto cm = vcm.toMap();
		if ( cm.value( "Hidden" ).toBool() )
			continue;

		QString baseSprite = cm.value( "BaseSprite" ).toString();

		//if( !baseSprite.isEmpty() ) qDebug() << baseSprite << cm.value( "Tint" ).toString() << cm.value( "Material" ).toString();

		if ( cm.value( "HasBase" ).toBool() )
		{
			painter2.drawPixmap( 0, 0, m_baseSprites[baseSprite + "Base"] );
		}
		QString tint = cm.value( "Tint" ).toString();
		bool isHair  = cm.value( "IsHair" ).toBool();
		if ( tint.isEmpty() )
		{
			painter2.drawPixmap( 0, 0, m_baseSprites[baseSprite] );
		}
		else
		{
			if ( tint == "Material" )
			{
				auto pm = getTintedBaseSprite( baseSprite, cm.value( "Material" ).toString() );
				painter2.drawPixmap( 0, 0, pm );
			}
			else
			{
				bool ok;
				int colorInt = tint.toInt( &ok );
				if ( ok )
				{
					QPixmap pm = m_baseSprites[baseSprite];
					if ( isHair )
					{
						tintPixmap( pm, m_hairColors[colorInt] );
					}
					else
					{
						tintPixmap( pm, m_colors[colorInt] );
					}
					painter2.drawPixmap( 0, 0, pm );
				}
			}
		}
	}
	img          = pmbr.toImage();
	QPixmap pmbl = QPixmap::fromImage( img.mirrored( true, false ) );

	SpritePixmap* sfr = nullptr;
	SpritePixmap* sfl = nullptr;
	SpritePixmap* sbl = nullptr;
	SpritePixmap* sbr = nullptr;

	if ( isDead )
	{
		sfr = new SpritePixmap( pmfr.transformed( QTransform().rotate( 90 ) ) );
		sfl = new SpritePixmap( pmfl.transformed( QTransform().rotate( 90 ) ) );
		sbl = new SpritePixmap( pmbl.transformed( QTransform().rotate( 90 ) ) );
		sbr = new SpritePixmap( pmbr.transformed( QTransform().rotate( 90 ) ) );
	}
	else
	{
		sfr = new SpritePixmap( pmfr );
		sfl = new SpritePixmap( pmfl );
		sbl = new SpritePixmap( pmbl );
		sbr = new SpritePixmap( pmbr );
	}

	SpriteRotations* sr = new SpriteRotations;

	sr->m_sprites.push_back( sfr );
	sr->m_sprites.push_back( sfl );
	sr->m_sprites.push_back( sbl );
	sr->m_sprites.push_back( sbr );

	if ( m_creatureSpriteIDs.contains( creatureUID ) )
	{
		Sprite* oldSprite = m_sprites[m_creatureSpriteIDs.value( creatureUID )];
		if ( oldSprite )
		{
			delete oldSprite;
		}

		sr->uID = m_creatureSpriteIDs.value( creatureUID );
		m_sprites.replace( sr->uID, sr );
	}
	else
	{
		sr->uID = m_sprites.size();
		m_creatureSpriteIDs.insert( creatureUID, sr->uID );
		m_sprites.append( sr );

		SpriteCreation sc { "Creature", { "None" }, m_randomNumbers, sr->uID, creatureUID };
		m_spriteCreations.push_back( sc );
	}

	addPixmapToPixelData32( sr );

	m_creatureTextureAdded = true;

	/*
	QString season = GameState::season" ).toString();
	QImage img_ = sr->pixmap( season, 0, 0 ).toImage();
	img_.save( "d:/tmp/gnome/gnome" + QString::number( creatureUID ) + "_" + QString::number( 0 ) + ".png", "PNG" );
	img_ = sr->pixmap( season, 1, 0 ).toImage();
	img_.save( "d:/tmp/gnome/gnome" + QString::number( creatureUID ) + "_" + QString::number( 1 ) + ".png", "PNG" );
	img_ = sr->pixmap( season, 2, 0 ).toImage();
	img_.save( "d:/tmp/gnome/gnome" + QString::number( creatureUID ) + "_" + QString::number( 2 ) + ".png", "PNG" );
	img_ = sr->pixmap( season, 3, 0 ).toImage();
	img_.save( "d:/tmp/gnome/gnome" + QString::number( creatureUID ) + "_" + QString::number( 3 ) + ".png", "PNG" );
	*/
	return sr;
}

Sprite* SpriteFactory::getCreatureSprite( const unsigned int id, unsigned int& spriteID )
{
	if ( m_creatureSpriteIDs.contains( id ) )
	{
		spriteID = m_creatureSpriteIDs[id];
		return m_sprites[m_creatureSpriteIDs[id]];
	}
	return nullptr;
}

void SpriteFactory::tintPixmap( QPixmap& pm, QColor color )
{
	QImage img = pm.toImage();

	for ( int x = 0; x < img.size().width(); ++x )
	{
		for ( int y = 0; y < img.size().height(); ++y )
		{
			QColor col = img.pixelColor( x, y );
			col.setRedF( qMin( 1., col.redF() * color.redF() ) );
			col.setGreenF( qMin( 1., col.greenF() * color.greenF() ) );
			col.setBlueF( qMin( 1., col.blueF() * color.blueF() ) );
			col.setAlphaF( qMin( 1., col.alphaF() * color.alphaF() ) );
			img.setPixelColor( x, y, col );
		}
	}
	pm = QPixmap::fromImage( img );
}

QStringList SpriteFactory::pixmaps()
{
	return m_pixmapSources.keys();
}

QPixmap SpriteFactory::pixmap( QString name )
{
	if ( m_pixmapSources.contains( name ) )
	{
		return m_pixmapSources[name];
	}
	qDebug() << "Pixmap " << name << " doesn't exist";
	return QPixmap( 32, 32 );
}

QPixmap SpriteFactory::baseSprite( QString id )
{
	if ( m_baseSprites.contains( id ) )
	{
		return m_baseSprites[id];
	}
	qDebug() << "Base sprite " << id << " doesn't exist";
	return QPixmap( 32, 32 );
}
