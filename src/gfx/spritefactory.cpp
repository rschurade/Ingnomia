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
#include "../base/containersHelper.h"
#include "../base/db.h"
#include "../base/gamestate.h"
#include "../base/io.h"
#include "../base/util.h"
#include "../fmt_formats.h"
#include "spdlog/spdlog.h"

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>

#include <SDL_image.h>

SpriteFactory::SpriteFactory()
{
	init();
}

SpriteFactory::~SpriteFactory()
{
	for( auto& sprite : m_sprites )
	{
		delete sprite;
	}
	m_sprites.clear();
	for( auto& def : m_spriteDefinitions | ranges::views::values )
	{
		delete def;
	}
	m_spriteDefinitions.clear();
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

	const auto tmp = DB::ids( "Seasons" );
	m_seasons.clear();
	for ( const auto& id : tmp ) {
		m_seasons.push_back( id.toStdString() );
	}

	auto matIds = DB::ids( "Materials" );
	for ( auto id : matIds )
	{
		m_materialTypes.insert_or_assign( id.toStdString(), DB::select( "Type", "Materials", id ).toString().toStdString() );
	}

	for ( auto color : DB::select2( "Color", "Materials", "Type", "Dye" ) )
	{
		m_colors.push_back( string2SDLColor( color.toString().toStdString() ) );
	}

	for ( auto color : DB::selectRows( "HairColors" ) )
	{
		m_hairColors.push_back( string2SDLColor( color.value( "Color" ).toString().toStdString() ) );
	}

	bool loaded = true;
	auto rows   = DB::selectRows( "BaseSprites" );
	for ( auto row : rows )
	{
		const auto tilesheet = row.value( "Tilesheet" ).toString().toStdString();
		if ( !m_pixmapSources.contains( tilesheet ) )
		{
			auto imgPath = fs::path( Global::cfg->get<std::string>( "dataPath" ) ) / "tilesheet" / tilesheet;
			SDL_Surface* pm;
			if ( !fs::exists(imgPath) )
			{
				pm = IMG_Load( tilesheet.c_str() );
				if ( !pm )
				{
					spdlog::debug( "SpriteFactory: failed to load '{}': {}", tilesheet, IMG_GetError() );
					return false;
				}
			} else {
				pm = IMG_Load( imgPath.c_str() );
			}
			m_pixmapSources.insert_or_assign( tilesheet, pm );
			/*
			if( tilesheet == "default.png" )
			{
				absl::btree_set<QString>unused;
				int count = 0;
				for( auto r : DB::selectRows( "BaseSprites" ) )
				{
					if( r.value( "Tilesheet"  ).toString() == "default.png" )
					if( DB::numRows( "Sprites", r.value( "ID" ).toString() ) == 0 )
						if( DB::numRows2( "Sprites", r.value( "ID" ).toString() ) == 0 )
							if( DB::numRows2( "Sprites_ByMaterialTypes", r.value( "ID" ).toString() ) == 0 )
								if( DB::numRows2( "Sprites_ByMaterials", r.value( "ID" ).toString() ) == 0 )
									if( DB::numRows2( "Sprites_Combine", r.value( "ID" ).toString() ) == 0 )
										if( DB::numRows2( "Sprites_Frames", r.value( "ID" ).toString() ) == 0 )
											if( DB::numRows2( "Sprites_Random", r.value( "ID" ).toString() ) == 0 )
												if( DB::numRows2( "Sprites_Rotations", r.value( "ID" ).toString() ) == 0 )
													if( DB::numRows2( "Sprites_Seasons", r.value( "ID" ).toString() ) == 0 )
														if( DB::numRows2( "Sprites_Seasons_Rotations", r.value( "ID" ).toString() ) == 0 )
														{
															qDebug() << count++ << r.value( "ID" ).toString() << r.value( "Tilesheet" ).toString();
															unused.insert( r.value( "ID" ).toString() );
														}
				}


				int width = pm.width();
				int height = pm.height();
				QPixmap newPM( width, height );
				
				newPM.fill( Qt::transparent);
				QPainter painter( &newPM );
				
				for( auto r : DB::selectRows( "BaseSprites" ) )
				{
					if( r.value( "Tilesheet" ).toString() == "default.png" )
					{
						if( !unused.contains( r.value( "ID" ).toString() ) )
						{
							auto p = r.value( "SourceRectangle" ).toString().split( " " );
							auto sp = pm.copy( p[0].toInt(), p[1].toInt(), 32, 36 );
							painter.drawPixmap( p[0].toInt(), p[1].toInt(), sp );
						}
					}
				}
				newPM.save( Global::cfg->get( "dataPath" ).toString() + "/tilesheet2/" + tilesheet );
			}
			*/
		}
		const auto spriteID = row.value( "ID" ).toString().toStdString();
		m_baseSprites.insert_or_assign( spriteID, extractPixmap( tilesheet, row ) );
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

	/* only for debug purpose
	QJsonArray ja;
	for ( auto sprite : spriteList )
	{
		QJsonValue jv = QJsonValue::fromVariant( sprite );
		ja.append( jv );
	}
	IO::saveFile( "spriteconv.json", ja );
	*/

	for ( auto row : spriteList )
	{
		m_spriteDefVMs.insert_or_assign( row.value( "ID" ).toString().toStdString(), row );
	}

	for ( auto row : spriteList )
	{
		DefNode* root = new DefNode;
		root->type    = "root";
		root->value   = row.value( "ID" ).toString().toStdString();
		m_numFrames   = 1;
		parseDef( root, row );
		root->numFrames = m_numFrames;
		if ( root->childs.empty() && root->baseSprite.empty() )
		{
			root->baseSprite = root->value;
		}
		m_spriteDefinitions.insert_or_assign( root->value, root );
	}

	createStandardSprites();

	return loaded;
}

void SpriteFactory::createStandardSprites()
{
	m_thoughtBubbleIDs.insert_or_assign( "Tired", createSprite( "StatusTired", { "None" } )->uID );
	m_thoughtBubbleIDs.insert_or_assign( "Sleeping", createSprite( "StatusSleeping", { "None" } )->uID );
	m_thoughtBubbleIDs.insert_or_assign( "Thirsty", createSprite( "ThoughtBubbleThirsty", { "None" } )->uID );
	m_thoughtBubbleIDs.insert_or_assign( "Hungry", createSprite( "ThoughtBubbleHungry", { "None" } )->uID );
	m_thoughtBubbleIDs.insert_or_assign( "Combat", createSprite( "ThoughtBubbleCombat", { "None" } )->uID );

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

SDL_Surface* SpriteFactory::extractPixmap( const std::string& sourcePNG, QVariantMap def )
{
	QString rect = def.value( "SourceRectangle" ).toString();

	QStringList rl = rect.split( " " );
	if ( rl.size() == 4 )
	{
		int x    = rl[0].toInt();
		int y    = rl[1].toInt();
		int dimX = rl[2].toInt();
		int dimY = rl[3].toInt();

		auto *dst = createPixmap( dimX, dimY );
		Uint32 maskMagenta = dst->format->Rmask | dst->format->Bmask;
		copyPixmapFrom( dst, m_pixmapSources.at(sourcePNG), x, y, dimX, dimY );

		return dst;
	}
	spdlog::debug( "***ERROR*** extractPixmap() for {}", def.value( "ID" ).toString().toStdString() );
	return nullptr;
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

Sprite* SpriteFactory::createSprite( const std::string& itemSID, const std::vector<std::string>& materialSIDs, const absl::btree_map<int, int>& random )
{
	QMutexLocker ml( &m_mutex );
	auto key = itemSID;
	m_randomNumbers.clear();
	if ( random.empty() )
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

			for ( auto r : m_randomNumbers | ranges::views::values )
			{
				key += "_";
				key += std::to_string( r );
			}
		}
	}
	Sprite* sprite = nullptr;
	if ( m_spriteIDs.contains( key ) )
	{
		sprite = m_sprites.value( m_spriteIDs.at( key ) );
	}
	else
	{
		sprite = createSpriteMaterial( itemSID, materialSIDs, key );

		if ( !sprite )
		{
			return m_sprites.value( m_spriteIDs.at( "SolidSelectionWall_Purple" ) );
		}

		sprite->uID = m_sprites.size();
		m_spriteIDs.insert_or_assign( key, m_sprites.size() );
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

	if ( itemSID.ends_with( "Wall" ) )
	{
		ml.unlock();
		createSprite( itemSID + "Short", materialSIDs, random );
	}

	return sprite;
}

// used for loading definitions, doesn't check if sprite exists as it will not exist and 
// doesn't create short walls because they are in the definition anyway
Sprite* SpriteFactory::createSprite2( const std::string& itemSID, const std::vector<std::string>& materialSIDs, const absl::btree_map<int, int>& random )
{
	auto key = itemSID;
	m_randomNumbers.clear();
	if ( random.empty() )
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

			for ( auto r : m_randomNumbers | ranges::views::values )
			{
				key += "_";
				key += std::to_string( r );
			}
		}
	}
	Sprite* sprite = nullptr;

	sprite = createSpriteMaterial( itemSID, materialSIDs, key );

	if ( !sprite )
	{
		return m_sprites.value( m_spriteIDs.at( "SolidSelectionWall_Purple" ) );
	}

	sprite->uID = m_sprites.size();
	m_spriteIDs.insert_or_assign( key, m_sprites.size() );
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
	return sprite;
}

Sprite* SpriteFactory::createAnimalSprite( const std::string& spriteSID, const absl::btree_map<int, int>& random )
{
	QMutexLocker ml( &m_mutex );
	auto key = spriteSID;
	m_randomNumbers.clear();
	if ( random.empty() )
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

		for ( auto r : m_randomNumbers | ranges::views::values )
		{
			key += "_";
			key += std::to_string( r );
		}
	}

	Sprite* sprite = nullptr;
	if ( m_spriteIDs.contains( key ) )
	{
		sprite = m_sprites.value( m_spriteIDs.at( key ) );
	}
	else
	{
		sprite = createSpriteMaterial( spriteSID, { "None" }, key );

		sprite->uID = m_sprites.size();
		m_spriteIDs.insert_or_assign( key, m_sprites.size() );
		m_sprites.append( sprite );

		addPixmapToPixelData( sprite );

		m_creatureTextureAdded = true;

		SpriteCreation sc { spriteSID, { "None" }, m_randomNumbers, sprite->uID, 1 };
		m_spriteCreations.push_back( sc );
	}

	return sprite;
}

bool SpriteFactory::containsRandom( const std::string& itemSID, const std::vector<std::string>& materialSIDs )
{
	auto spriteSID = DBH::spriteID( itemSID );
	if ( spriteSID.empty() )
	{
		spriteSID = itemSID;
	}
	return DBH::spriteIsRandom( spriteSID );
}

std::string SpriteFactory::createSpriteMaterialDryRun( const std::string& itemSID, const std::vector<std::string>& materialSIDs )
{
	auto spriteSID = DBH::spriteID( itemSID );
	if ( spriteSID.empty() )
	{
		// every item needs a SpriteID
		//spdlog::debug( "***ERROR*** item {} has no SpriteID entry.", itemSID );
		spriteSID = itemSID;
	}

	if ( !m_spriteDefinitions.contains( spriteSID ) )
	{
		spdlog::error( "***ERROR*** sprite definition \'{}\" for item \"{}\" doesn't exist.", spriteSID, itemSID );
		//abort();
	}
	DefNode* dn = m_spriteDefinitions.at( spriteSID );
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
	auto key = itemSID;
	for ( auto mat : materialSIDs )
	{
		key += "_";
		key += mat;
	}
	for ( auto r : m_randomNumbers | ranges::views::values )
	{
		key += "_";
		key += std::to_string( r );
	}
	return key;
}

Sprite* SpriteFactory::createSpriteMaterial( const std::string& itemSID, const std::vector<std::string>& materialSIDs, const std::string& key )
{
	auto spriteSID = DBH::spriteID( itemSID );
	if ( spriteSID.empty() )
	{
		// every item needs a SpriteID
		//spdlog::debug( "***ERROR*** item {} has no SpriteID entry.", itemSID );
		spriteSID = itemSID;
	}

	if ( !m_spriteDefinitions.contains( spriteSID ) )
	{
		spdlog::error( "***ERROR*** sprite definition '{}' for item '{}' doesn't exist.", spriteSID, itemSID );
		//abort();
	}
	DefNode* dn    = m_spriteDefinitions.at( spriteSID );
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
		sprite->anim          = DBH::spriteHasAnim( spriteSID );
	}
	return sprite;
}

void SpriteFactory::parseDef( DefNode* parent, QVariantMap def )
{
	if ( def.contains( "Sprite" ) )
	{
		const auto spriteID = def.value( "Sprite" ).toString().toStdString();
		if ( !spriteID.empty() )
		{
			QVariant tint   = def.value( "Tint" );
			QVariant effect = def.value( "Effect" );
			QVariant offset = def.value( "Offset" );

			def             = m_spriteDefVMs.at( spriteID );
			if ( tint.isValid() )
			{
				def.insert( "Tint", tint );
			}
			if ( effect.isValid() )
			{
				def.insert( "Effect", effect );
			}
		}
	}

	parent->effect          = def.value( "Effect" ).toString().toStdString();
	parent->offset          = def.value( "Offset" ).toString();
	parent->tint            = def.value( "Tint" ).toString().toStdString();
	parent->baseSprite      = def.value( "BaseSprite" ).toString().toStdString();
	parent->defaultMaterial = def.value( "DefaultMaterial" ).toString().toStdString();
	parent->hasTransp       = def.value( "HasTransp" ).toBool();

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
			child->value    = item.toMap().value( "MaterialID" ).toString().toStdString();
			parent->childs.insert_or_assign( child->value, child );
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
			child->value    = item.toMap().value( "MaterialType" ).toString().toStdString();
			parent->childs.insert_or_assign( child->value, child );
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
			child->value    = item.toMap().value( "Season" ).toString().toStdString();
			parent->childs.insert_or_assign( child->value, child );
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
			child->value    = item.toMap().value( "Rotation" ).toString().toStdString();
			parent->childs.insert_or_assign( child->value, child );
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
			parent->childs.insert_or_assign( "Frame" + std::to_string( index++ ), child );
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
		parent->childs.insert_or_assign( "RandomNode", randomNode );

		for ( auto item : def.value( "Random" ).toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = randomNode->depth + 1;
			child->childPos = randomNode->childPos + QString::number( randomNode->childs.size() + 1 );
			child->type     = "Random";
			randomNode->childs.insert_or_assign( "Random" + std::to_string( index++ ), child );
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
		parent->childs.insert_or_assign( "CombineNode", combineNode );

		for ( auto item : def.value( "Combine" ).toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = parent->depth + 1;
			child->childPos = combineNode->childPos + QString::number( combineNode->childs.size() + 1 );
			child->type     = "Combine";
			combineNode->childs.insert_or_assign( "Combine" + std::to_string( index++ ), child );
			parseDef( child, item.toMap() );
		}
	}
}

Sprite* SpriteFactory::getBaseSprite( const DefNode* node, const std::string& itemSID, const std::vector<std::string>& materialSIDs, int materialID )
{
	if ( !node->offset.isEmpty() )
	{
		m_offset = node->offset;
	}
	materialID          = std::min( materialID, (int)(materialSIDs.size() - 1) );
	const auto materialSID = materialSIDs[materialID];
	if ( !node->baseSprite.empty() )
	{
		SDL_Surface* pm      = m_baseSprites.at( node->baseSprite );
		SpritePixmap* sprite = new SpritePixmap( pm, m_offset.toStdString() );
		sprite->applyEffect( node->effect );
		sprite->applyTint( node->tint, materialSID );
		sprite->hasTransp = node->hasTransp;
		return sprite;
	}

	if ( node->childs.size() && node->childs.begin()->second->type == "Rotation" )
	{
		SpriteRotations* sr = new SpriteRotations;

		sr->m_sprites.push_back( getBaseSprite( node->childs.at( "FR" ), itemSID, materialSIDs ) );
		sr->m_sprites.push_back( getBaseSprite( node->childs.at( "FL" ), itemSID, materialSIDs ) );
		sr->m_sprites.push_back( getBaseSprite( node->childs.at( "BL" ), itemSID, materialSIDs ) );
		sr->m_sprites.push_back( getBaseSprite( node->childs.at( "BR" ), itemSID, materialSIDs ) );

		sr->applyEffect( node->effect );
		sr->applyTint( node->tint, materialSID );
		sr->hasTransp = node->hasTransp;
		return sr;
	}
	if ( node->childs.size() && node->childs.begin()->second->type == "Season" )
	{
		SpriteSeasons* ss = new SpriteSeasons;
		for ( auto child : node->childs | ranges::views::values )
		{
			ss->m_sprites.insert_or_assign( child->value, getBaseSprite( child, itemSID, materialSIDs ) );
		}
		ss->applyEffect( node->effect );
		ss->applyTint( node->tint, materialSID );
		ss->hasTransp = node->hasTransp;
		return ss;
	}
	if ( DefNode * temp; maps::try_at( node->childs, itemSID, temp ) )
	{
		return getBaseSprite( temp, itemSID, materialSIDs );
	}
	if ( node->childs.size() && node->childs.begin()->second->type == "Frame" )
	{
		SpriteFrames* sf = new SpriteFrames;
		for ( auto child : node->childs | ranges::views::values )
		{
			sf->m_sprites.push_back( getBaseSprite( child, itemSID, materialSIDs ) );
		}
		sf->applyEffect( node->effect );
		sf->applyTint( node->tint, materialSID );
		sf->hasTransp = node->hasTransp;
		return sf;
	}

	if ( node->type == "CombineNode" && node->childs.size() > 1 )
	{
		Sprite* s = getBaseSprite( node->childs.at("Combine0"), itemSID, materialSIDs, materialID );

		for ( int i = 1; i < node->childs.size(); ++i )
		{
			Sprite* s2 = getBaseSprite( node->childs.at("Combine" + std::to_string( i )), itemSID, materialSIDs, materialID + i );
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
		s->hasTransp = node->hasTransp;
		return s;
	}
	if ( node->type == "RandomNode" )
	{
		int randomNumber = m_randomNumbers.at( node->childPos.toInt() );
		const auto &childKey    = "Random" + std::to_string( randomNumber );
		const auto &nodeChild       = node->childs.at(childKey);
		Sprite* rs       = getBaseSprite( nodeChild, itemSID, materialSIDs );
		rs->applyEffect( nodeChild->effect );
		rs->applyTint( nodeChild->tint, materialSID );
		rs->hasTransp = node->hasTransp;
		return rs;
	}

	if ( materialSIDs.size() )
	{
		DefNode *temp;

		auto materialType = getMaterialType( materialSID );
		if ( maps::try_at( node->childs, materialType, temp ) )
		{
			Sprite* pm = getBaseSprite( temp, itemSID, materialSIDs );
			pm->applyEffect( node->effect );
			pm->applyTint( node->tint, materialSID );
			pm->hasTransp = node->hasTransp;
			return pm;
		}

		const auto& materialSID = *materialSIDs.begin();
		if ( maps::try_at( node->childs, materialSID, temp ) )
		{
			Sprite* pm = getBaseSprite( temp, itemSID, materialSIDs );
			pm->applyEffect( node->effect );
			pm->applyTint( node->tint, materialSID );
			pm->hasTransp = node->hasTransp;
			return pm;
		}
		// if no other hit
		if ( node->childs.size() )
		{
			if ( !node->defaultMaterial.empty() && maps::try_at( node->childs, node->defaultMaterial, temp ) )
			{
				Sprite* pm = getBaseSprite( temp, itemSID, materialSIDs );
				pm->applyEffect( node->effect );
				pm->applyTint( node->tint, materialSID );
				pm->hasTransp = node->hasTransp;
				return pm;
			}
			else
			{
				Sprite* pm = getBaseSprite( node->childs.begin()->second, itemSID, materialSIDs, materialID );
				pm->applyEffect( node->effect );
				pm->applyTint( node->tint, materialSID );
				pm->hasTransp = node->hasTransp;
				return pm;
			}
		}
	}

	auto* pm     = m_baseSprites.at( "SolidSelectionWall" );
	auto* sprite = new SpritePixmap( pm );
	sprite->applyTint( "Material", "JobPurple" );
	return sprite;
}

void SpriteFactory::getBaseSpriteDryRun( const DefNode* node, const std::string& itemSID, const std::vector<std::string>& materialSIDs, const std::string& season, const std::string& rotation, const int animFrame )
{
	if ( !node->baseSprite.empty() )
	{
		return;
	}

	const auto &childs = node->childs;

	DefNode *temp;
	if ( maps::try_at( childs, rotation, temp ) )
	{
		getBaseSpriteDryRun( temp, itemSID, materialSIDs, season, rotation, animFrame );
		return;
	}
	if ( maps::try_at( childs, season, temp ) )
	{
		getBaseSpriteDryRun( temp, itemSID, materialSIDs, season, rotation, animFrame );
		return;
	}
	if ( maps::try_at( childs, itemSID, temp ) )
	{
		getBaseSpriteDryRun( temp, itemSID, materialSIDs, season, rotation, animFrame );
		return;
	}
	const auto frameKey = "Frame" + std::to_string( animFrame );
	if ( maps::try_at( childs, frameKey, temp ) )
	{
		getBaseSpriteDryRun( temp, itemSID, materialSIDs, season, rotation, animFrame );
		return;
	}

	if ( node->type == "CombineNode" && childs.size() > 1 )
	{
		getBaseSpriteDryRun( childs.at("Combine0"), itemSID, materialSIDs, season, rotation, animFrame );

		for ( int i = 1; i < childs.size(); ++i )
		{
			getBaseSpriteDryRun( childs.at("Combine" + std::to_string( i )), itemSID, materialSIDs, season, rotation, animFrame );
		}
		return;
	}
	if ( node->type == "RandomNode" )
	{
		QList<int> weights = node->randomWeights;
		//spdlog::debug( "{}", weights.toStdString() );
		int randomNumber = 0;
		if ( !m_randomNumbers.contains( node->childPos.toInt() ) )
		{
			if ( weights.contains( 0 ) )
			{
				randomNumber = rand() % childs.size();
				m_randomNumbers.insert_or_assign( node->childPos.toInt(), randomNumber );
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
						m_randomNumbers.insert_or_assign( node->childPos.toInt(), i );
						break;
					}
				}
			}
		}
		randomNumber = m_randomNumbers.at( node->childPos.toInt() );

		getBaseSpriteDryRun( childs.at("Random" + std::to_string( randomNumber )), itemSID, materialSIDs, season, rotation, animFrame );
		return;
	}

	if ( !materialSIDs.empty() )
	{
		const auto materialSID = *materialSIDs.begin();
		auto materialType = getMaterialType( materialSID );
		if ( maps::try_at( childs, materialType, temp ) )
		{
			getBaseSpriteDryRun( temp, itemSID, materialSIDs, season, rotation, animFrame );
			return;
		}

		if ( maps::try_at( childs, materialSID, temp ) )
		{
			getBaseSpriteDryRun( temp, itemSID, materialSIDs, season, rotation, animFrame );
			return;
		}
		// if no other hit
		if ( !childs.empty() )
		{
			getBaseSpriteDryRun( childs.begin()->second, itemSID, materialSIDs, season, rotation, animFrame );
			return;
		}
	}
}

int SpriteFactory::numFrames( const DefNode* node, const std::string& itemSID, const std::vector<std::string>& materialSIDs, const std::string& season, const std::string& rotation )
{
	return 1;
}

std::string SpriteFactory::getMaterialType( const std::string& materialSID )
{
	return m_materialTypes.at( materialSID );
}

Sprite* SpriteFactory::getSprite( const int id )
{
	QMutexLocker ml( &m_mutex );
	if ( m_sprites.size() > id )
	{
		return m_sprites[id];
	}
	return nullptr;
}

void SpriteFactory::printDebug()
{
	for ( auto si : m_spriteIDs )
	{
		spdlog::debug( "{} - {}", si.first, si.second );
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
			if ( sc.uID != static_cast<unsigned int>( m_sprites.size() ) )
			{
				std::vector<std::string> materialIDs;
				for ( const auto& item : sc.materialSIDs ) {
					materialIDs.push_back(item);
				}

				if ( sc.uID < static_cast<unsigned int>( m_sprites.size() ) )
				{
					spdlog::warn( "## Loosing sprite {} {} {} {}", sc.uID, m_sprites.size(), sc.itemSID, materialIDs );
					continue;
				}
				else
				{
					spdlog::debug( "## Missing sprite before {} {} {} {}", sc.uID, m_sprites.size(), sc.itemSID, materialIDs );
					while ( sc.uID > static_cast<unsigned int>( m_sprites.size() ) )
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
					m_creatureSpriteIDs.insert_or_assign( sc.creatureID, sc.uID );
					m_sprites.append( nullptr );
				}
			}
			else
			{
				createSprite2( sc.itemSID, sc.materialSIDs, sc.random );
			}
		}
	}
	spdlog::debug( "Used{}array textures", m_texesUsed );
	m_spriteCreations = scl;
}

void SpriteFactory::addPixmapToPixelData( Sprite* sprite )
{
	auto season = GameState::seasonString.toStdString();
	if( season.empty() )
	{
		season = "Spring";
	}
	if ( sprite->anim )
	{
		for ( int frame = 0; frame < 4; ++frame )
		{
			int tex     = ( sprite->uID + frame ) / 512;
			m_texesUsed = std::max( m_texesUsed, tex + 1 );
			int id      = ( sprite->uID + frame ) % 512;

			if ( m_pixelData.size() < tex + 1 )
			{
				int maxArrayTextures = Global::cfg->get<int>( "MaxArrayTextures" );
				int bytes            = 32 * 64 * 4 * maxArrayTextures;

				for ( int i = 0; i < 32; ++i )
				{
					std::vector<uint8_t> data( bytes );
					m_pixelData.push_back( data );
				}
			}

			for ( int i = 0; i < 4; ++i )
			{
				auto pm    = sprite->pixmap( season, i, frame );

				int startIndex = 8192 * ( 4 * id + i );

				if ( pm->h == 64 && pm->w == 32 )
				{
					SDL_LockSurface( pm );
					auto data = m_pixelData[tex].data();
					for ( int y = 0; y < 64; ++y )
					{
						for ( int x = 0; x < 32; ++x )
						{
							const auto col         = getPixelColor( pm, x, y );
							const size_t baseIndex = startIndex + ( x * 4 + 128 * y );
							data[baseIndex + 0]    = col.r;
							data[baseIndex + 1]    = col.g;
							data[baseIndex + 2]    = col.b;
							data[baseIndex + 3]    = col.a;
						}
					}
					SDL_UnlockSurface( pm );
				}
			}
		}
	}
	else
	{
		int tex     = sprite->uID / 512;
		m_texesUsed = std::max( m_texesUsed, tex + 1 );
		int id      = sprite->uID % 512;

		if ( m_pixelData.size() < tex + 1 )
		{
			int maxArrayTextures = Global::cfg->get<int>( "MaxArrayTextures" );
			int bytes            = 32 * 64 * 4 * maxArrayTextures;

			for ( int i = 0; i < 32; ++i )
			{
				std::vector<uint8_t> data( bytes );
				m_pixelData.push_back( data );
			}
		}

		for ( int i = 0; i < 4; ++i )
		{
			auto pm    = sprite->pixmap( season, i, 0 );

			int startIndex = 8192 * ( 4 * id + i );

			if ( pm->h == 64 && pm->w == 32 )
			{
				SDL_LockSurface( pm );
				for ( int y = 0; y < 64; ++y )
				{
					for ( int x = 0; x < 32; ++x )
					{
						const auto col = getPixelColor( pm, x, y );
						m_pixelData[tex][startIndex + ( x * 4 + 128 * y )]     = col.r;
						m_pixelData[tex][startIndex + ( x * 4 + 128 * y + 1 )] = col.g;
						m_pixelData[tex][startIndex + ( x * 4 + 128 * y + 2 )] = col.b;
						m_pixelData[tex][startIndex + ( x * 4 + 128 * y + 3 )] = col.a;
					}
				}
				SDL_UnlockSurface( pm );
			}
		}
	}
}

void SpriteFactory::addPixmapToPixelData32( Sprite* sprite )
{
	int tex        = sprite->uID / 512;
	int id         = sprite->uID % 512;
	const auto season = GameState::seasonString.toStdString();

	for ( int i = 0; i < 4; ++i )
	{
		auto pm    = sprite->pixmap( season, i, 0 );

		SDL_LockSurface( pm );

		int startIndex = 8192 * ( 4 * id + i );

		for ( int x = 0; x < 32; ++x )
		{
			for ( int y = 0; y < 16; ++y )
			{
				m_pixelData[tex][startIndex + ( x * 4 + 128 * y )] = 0;
				m_pixelData[tex][startIndex + ( x * 4 + 128 * y + 1 )] = 0;
				m_pixelData[tex][startIndex + ( x * 4 + 128 * y + 2 )] = 0;
				m_pixelData[tex][startIndex + ( x * 4 + 128 * y + 3 )] = 0;
			}

			for ( int y = 16; y < 48; ++y )
			{
				const auto col = getPixelColor( pm, x, y - 16 );
				m_pixelData[tex][startIndex + ( x * 4 + 128 * y )] = col.r;
				m_pixelData[tex][startIndex + ( x * 4 + 128 * y + 1 )] = col.g;
				m_pixelData[tex][startIndex + ( x * 4 + 128 * y + 2 )] = col.b;
				m_pixelData[tex][startIndex + ( x * 4 + 128 * y + 3 )] = col.a;
			}
			for ( int y = 48; y < 64; ++y )
			{
				m_pixelData[tex][startIndex + ( x * 4 + 128 * y )] = 0;
				m_pixelData[tex][startIndex + ( x * 4 + 128 * y + 1 )] = 0;
				m_pixelData[tex][startIndex + ( x * 4 + 128 * y + 2 )] = 0;
				m_pixelData[tex][startIndex + ( x * 4 + 128 * y + 3 )] = 0;
			}
		}

		SDL_UnlockSurface( pm );
	}
}

void SpriteFactory::addEmptyRows( int startIndex, int rows, std::vector<uint8_t>& pixelData )
{
	for ( int y = 0; y < rows; ++y )
	{
		for ( int x = 0; x < 32; ++x )
		{
			pixelData[startIndex + ( x * 4 + 128 * y )] = 0;
			pixelData[startIndex + ( x * 4 + 128 * y + 1 )] = 0;
			pixelData[startIndex + ( x * 4 + 128 * y + 2 )] = 0;
			pixelData[startIndex + ( x * 4 + 128 * y + 3 )] = 0;
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

SDL_Surface* SpriteFactory::getTintedBaseSprite( const std::optional<std::string>& baseSprite, const std::string& material )
{
	if ( !baseSprite || baseSprite->empty() )
	{
		return m_baseSprites["EmptyWall"];
	}

	auto* pm     = m_baseSprites.at(*baseSprite);
	auto* result = createPixmap( pm->w, pm->h );

	copyPixmap( result, pm, 0, 0 );
	tintPixmap( result, string2SDLColor( DBH::materialColor( material ) ) );
	return result;
}

Sprite* SpriteFactory::setCreatureSprite( const unsigned int creatureUID, const std::vector<DBS::Creature_Parts>& components, const std::vector<DBS::Creature_Parts>& componentsBack, bool isDead )
{
	QMutexLocker ml( &m_mutex );
	auto pmfr = createPixmap( 32, 32 );
	SDL_FillRect( pmfr, nullptr, 0x00000000 );

	//spdlog::debug(" =================================================");
	for ( auto cm : components )
	{
		if ( cm.Hidden )
			continue;

		const auto& baseSprite = cm.BaseSprite;

		//if( !baseSprite.empty() ) qDebug() << baseSprite << cm.value( "Tint" ).toString() << cm.value( "Material" ).toString();

		if ( cm.HasBase )
		{
			assert(baseSprite && "Sprite with \"Base\" should have \"BaseSprite\" defined");
			copyPixmap( pmfr, m_baseSprites.at(*baseSprite + "Base"), 0, 0 );
		}

		const auto& tint = cm.Tint;
		const auto& isHair  = cm.IsHair;
		if ( std::get_if<std::monostate>( &tint ) )
		{
			assert(baseSprite && "Sprite without \"Tint\" should have \"BaseSprite\" defined");
			copyPixmap( pmfr, m_baseSprites.at(*baseSprite), 0, 0 );
		}
		else if ( const auto* str = std::get_if<std::string>( &tint ) )
		{
			if ( *str == "Material" )
			{
				auto pm = getTintedBaseSprite( baseSprite, cm.Material );
				copyPixmap( pmfr, pm, 0, 0 );
			}
			else
			{
				spdlog::debug( "Got an unexpected tint: {}", *str );
			}
		}
		else if ( const auto* colorInt = std::get_if<int>( &tint ) )
		{
			if (baseSprite)
			{
				auto* pm = m_baseSprites.at(*baseSprite);
				copyPixmap( pmfr, pm, 0, 0 );
				const auto color = isHair ? m_hairColors[*colorInt] : m_colors[*colorInt];
				tintPixmap( pmfr, color );
			}
		}
	}
	auto *pmfl = clonePixmap(pmfr);
	flipPixmap( pmfl, true, false );

	auto* pmbr = createPixmap( 32, 32 );
	SDL_FillRect(pmbr, nullptr, 0x00000000);

	//spdlog::debug("---------------------------------");
	for ( auto cm : componentsBack )
	{
		if ( cm.Hidden )
			continue;

		const auto& baseSprite = cm.BaseSprite;

		//if( !baseSprite.empty() ) qDebug() << baseSprite << cm.value( "Tint" ).toString() << cm.value( "Material" ).toString();

		if ( cm.HasBase )
		{
			assert(baseSprite && "Sprite with \"Base\" should have \"BaseSprite\" defined");
			copyPixmap( pmbr, m_baseSprites.at(*baseSprite + "Base"), 0, 0 );
		}

		const auto& tint = cm.Tint;
		const auto& isHair  = cm.IsHair;
		if ( std::get_if<std::monostate>( &tint ) )
		{
			assert(baseSprite && "Sprite without \"Tint\" should have \"BaseSprite\" defined");
			copyPixmap( pmbr, m_baseSprites.at(*baseSprite), 0, 0 );
		}
		else if ( const auto* str = std::get_if<std::string>( &tint ) )
		{
			if ( *str == "Material" )
			{
				auto pm = getTintedBaseSprite( baseSprite, cm.Material );
				copyPixmap( pmbr, pm, 0, 0 );
			}
			else
			{
				spdlog::debug( "Got an unexpected tint: {}", *str );
			}
		}
		else if ( const auto* colorInt = std::get_if<int>( &tint ) )
		{
			if (baseSprite)
			{
				auto* pm = m_baseSprites.at(*baseSprite);
				copyPixmap( pmbr, pm, 0, 0 );
				const auto color = isHair ? m_hairColors[*colorInt] : m_colors[*colorInt];
				tintPixmap( pmbr, color );
			}
		}
	}

	auto *pmbl = clonePixmap(pmbr);
	flipPixmap( pmbl, true, false );

	SpritePixmap* sfr = nullptr;
	SpritePixmap* sfl = nullptr;
	SpritePixmap* sbl = nullptr;
	SpritePixmap* sbr = nullptr;

	if ( isDead )
	{
		sfr = new SpritePixmap( rotatePixmap90Clone( pmfr ) );
		sfl = new SpritePixmap( rotatePixmap90Clone( pmfl ) );
		sbl = new SpritePixmap( rotatePixmap90Clone( pmbl ) );
		sbr = new SpritePixmap( rotatePixmap90Clone( pmbr ) );
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

	if ( auto it = m_creatureSpriteIDs.find( creatureUID ); it != m_creatureSpriteIDs.end() )
	{
		Sprite* oldSprite = m_sprites[it->second];
		if ( oldSprite )
		{
			delete oldSprite;
		}

		sr->uID = it->second;
		m_sprites.replace( sr->uID, sr );
	}
	else
	{
		sr->uID = m_sprites.size();
		m_creatureSpriteIDs.insert_or_assign( creatureUID, sr->uID );
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
	QMutexLocker ml( &m_mutex );
	if ( m_creatureSpriteIDs.contains( id ) )
	{
		spriteID = m_creatureSpriteIDs[id];
		return m_sprites[m_creatureSpriteIDs[id]];
	}
	return nullptr;
}

SDL_Surface* SpriteFactory::pixmap( const std::string& name )
{
	if ( m_pixmapSources.contains( name ) )
	{
		return m_pixmapSources[name];
	}
	spdlog::debug( "Pixmap {} doesn't exist", name );
	throw std::runtime_error( "Pixmap missing" );
//	return QPixmap( 32, 32 );
}

SDL_Surface* SpriteFactory::baseSprite( const std::string& id )
{
	if ( m_baseSprites.contains( id ) )
	{
		return m_baseSprites.at(id);
	}
	spdlog::debug( "Base sprite {} doesn't exist", id );
	throw std::runtime_error( "Pixmap missing" );
//	return QPixmap( 32, 32 );
}

unsigned int SpriteFactory::thoughtBubbleID( QString sid )
{
	QMutexLocker ml( &m_mutex );
	return m_thoughtBubbleIDs.at( sid );
}

int SpriteFactory::texesUsed()
{
	QMutexLocker ml( &m_mutex );
	return m_texesUsed;
}

unsigned int SpriteFactory::size()
{
	QMutexLocker ml( &m_mutex );
	return m_spriteIDs.size();
}

std::vector<uint8_t> SpriteFactory::pixelData( int index )
{
	QMutexLocker ml( &m_mutex );
	return m_pixelData[index];
}