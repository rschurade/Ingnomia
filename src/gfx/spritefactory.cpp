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
#include "../gfx/constants.h"
#include "../fmt_formats.h"
#include "spdlog/spdlog.h"

#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>

#include <SDL_image.h>

enum class DefNodeType {
	Unknown, Root,
	ByItem, ByMaterial, ByMaterialType,
	Season, Rotation, Frame, RandomNode, Random,
	CombineNode, Combine
};

static int _textureSize, _texNumSpritesX, _texNumSpritesY, _spritesPerTex;

struct DefNode
{
	DefNode()
	{
		//parent = 0;

		type       = DefNodeType::Unknown;
		offset     = "";
		effect     = "";
		tint       = "";
		value      = "";
		baseSprite = "";
		depth      = 0;
		childPos   = "1";
		numFrames  = 1;
	};

	~DefNode()
	{
		for( auto& child : childs | ranges::views::values )
		{
			delete child;
		}
	}

	DefNodeType type;
	QString offset;
	std::string effect;
	std::string tint;
	std::string value;
	std::string baseSprite;
	std::string defaultMaterial;
	int depth;
	int numFrames;
	QString childPos;
	QList<int> randomWeights;
	bool rot90 = false;
	bool hasTransp = false;

	//DefNode* parent;
	absl::flat_hash_map<std::string, DefNode*> childs;
};

SpriteFactory::SpriteFactory()
{
	_textureSize = Global::cfg->get<int>( "TextureSize" );
	_texNumSpritesX = _textureSize / SpriteWidth;
	_texNumSpritesY = _textureSize / SpriteHeight;
	_spritesPerTex = (_textureSize / SpriteWidth) * (_textureSize / SpriteHeight);

	init();
}

SpriteFactory::~SpriteFactory()
{
	for ( const auto& entry : m_pixelData )
	{
		SDL_FreeSurface(entry.second);
	}
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
	for ( const auto& id : matIds )
	{
		m_materialTypes.insert_or_assign( id.toStdString(), DB::select( "Type", "Materials", id ).toString().toStdString() );
	}

	for ( const auto& color : DB::select2( "Color", "Materials", "Type", "Dye" ) )
	{
		m_colors.push_back( string2SDLColor( color.toString().toStdString() ) );
	}

	for ( const auto& color : DB::selectRows( "HairColors" ) )
	{
		m_hairColors.push_back( string2SDLColor( color.value( "Color" ).toString().toStdString() ) );
	}

	bool loaded = true;
	auto rows   = DB::selectRows( "BaseSprites" );
	for ( const auto& row : rows )
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
				pm = IMG_Load( imgPath.string().c_str() );
			}
			m_pixmapSources.insert_or_assign( tilesheet, pm );
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

		auto rows = DB::selectRows( "Sprites_ByMaterials", spriteID );
		if ( !rows.empty() )
		{
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
		rows = DB::selectRows( "Sprites_ByMaterialTypes", spriteID );
		if ( !rows.empty() )
		{
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
		rows = DB::selectRows( "Sprites_Combine", spriteID );
		if ( !rows.empty() )
		{
			QVariantList combine;
			for ( auto entry : rows )
			{
				QVariantMap cm;
				const auto& baseSprite = entry.value( "BaseSprite" ).toString();
				if ( baseSprite.isEmpty() )
				{
					cm.insert( "Sprite", entry.value( "Sprite" ).toString() );
				}
				else
				{
					cm.insert( "BaseSprite", baseSprite );
				}
				const auto& tint = entry.value( "Tint" ).toString();
				if ( !tint.isEmpty() )
				{
					cm.insert( "Tint", tint );
				}
				const auto& offset = entry.value( "Offset" ).toString();
				if ( !offset.isEmpty() )
				{
					cm.insert( "Offset", offset );
				}

				combine.append( cm );
			}
			sprite.remove( "BaseSprite" );
			sprite.insert( "Combine", combine );
		}
		rows = DB::selectRows( "Sprites_Frames", spriteID );
		if ( !rows.empty() )
		{
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
		rows = DB::selectRows( "Sprites_Random", spriteID );
		if ( !rows.empty() )
		{
			QVariantList random;
			for ( auto entry : rows )
			{
				QVariantMap cm;
				const auto& baseSprite = entry.value( "BaseSprite" ).toString();
				if ( baseSprite.isEmpty() )
				{
					cm.insert( "Sprite", entry.value( "Sprite" ).toString() );
				}
				else
				{
					cm.insert( "BaseSprite", baseSprite );
				}
				cm.insert( "Weight", entry.value( "Weight" ).toInt() );

				random.append( cm );
			}
			sprite.remove( "BaseSprite" );
			sprite.insert( "Random", random );
		}
		rows = DB::selectRows( "Sprites_Rotations", spriteID );
		if ( !rows.empty() )
		{
			QVariantList rotations;
			for ( auto entry : rows )
			{
				QVariantMap cm;
				const auto& baseSprite = entry.value( "BaseSprite" ).toString();
				if ( baseSprite.isEmpty() )
				{
					cm.insert( "Sprite", entry.value( "Sprite" ).toString() );
				}
				else
				{
					cm.insert( "BaseSprite", baseSprite );
				}
				const auto& effect = entry.value( "Effect" ).toString();
				if ( !effect.isEmpty() )
				{
					cm.insert( "Effect", effect );
				}
				cm.insert( "Rotation", entry.value( "Rotation" ).toString() );

				rotations.append( cm );
			}
			sprite.remove( "BaseSprite" );
			sprite.insert( "Rotations", rotations );
		}
		rows = DB::selectRows( "Sprites_Seasons", spriteID );
		if ( !rows.empty() )
		{
			QVariantList seasons;
			for ( auto entry : rows )
			{
				QVariantMap cm;
				const auto baseSprite = entry.value( "BaseSprite" ).toString();
				if ( !baseSprite.isEmpty() )
				{
					cm.insert( "BaseSprite", baseSprite );
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

	for ( const auto& row : spriteList )
	{
		m_spriteDefVMs.insert_or_assign( row.value( "ID" ).toString().toStdString(), row );
	}

	for ( const auto& row : spriteList )
	{
		DefNode* root = new DefNode;
		root->type    = DefNodeType::Root;
		root->value   = row.value( "ID" ).toString().toStdString();
		m_numFrames   = 1;
		parseDef( root, row );
		// TODO: Not sure how this makes sense? `parseDef()` is within a recursive call chain, so this value will always be the very last one?
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

Sprite* SpriteFactory::createSprite( const std::string& itemSID, const std::vector<std::string>& materialSIDs, const absl::flat_hash_map<int, int>& random )
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
Sprite* SpriteFactory::createSprite2( const std::string& itemSID, const std::vector<std::string>& materialSIDs, const absl::flat_hash_map<int, int>& random )
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

Sprite* SpriteFactory::createAnimalSprite( const std::string& spriteSID, const absl::flat_hash_map<int, int>& random )
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
	QVariantMap::iterator it;
	if ( (it = def.find( "Sprite" )) != def.end() )
	{
		const auto spriteID = it->toString().toStdString();
		if ( !spriteID.empty() )
		{
			QVariant tint   = def.value( "Tint" );
			QVariant effect = def.value( "Effect" );

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
			child->type = DefNodeType::ByItem;
			child->value = item.toMap().value( "ItemID" ).toString();
			parent->childs.insert( child->value, child );
			parseDef( child, item.toMap() );
		}
	}
	*/
	if ( (it = def.find( "ByMaterials" )) != def.end() )
	{
		for ( const auto& item : it->toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = parent->depth + 1;
			child->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
			child->type     = DefNodeType::ByMaterial;
			child->value    = item.toMap().value( "MaterialID" ).toString().toStdString();
			parent->childs.insert_or_assign( child->value, child );
			parseDef( child, item.toMap() );
		}
	}
	if ( (it = def.find( "ByMaterialTypes" )) != def.end() )
	{
		for ( const auto& item : it->toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = parent->depth + 1;
			child->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
			child->type     = DefNodeType::ByMaterialType;
			child->value    = item.toMap().value( "MaterialType" ).toString().toStdString();
			parent->childs.insert_or_assign( child->value, child );
			parseDef( child, item.toMap() );
		}
	}
	if ( (it = def.find( "Seasons" )) != def.end() )
	{
		for ( const auto& item : it->toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = parent->depth + 1;
			child->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
			child->type     = DefNodeType::Season;
			child->value    = item.toMap().value( "Season" ).toString().toStdString();
			parent->childs.insert_or_assign( child->value, child );
			parseDef( child, item.toMap() );
		}
	}
	if ( (it = def.find( "Rotations" )) != def.end() )
	{
		for ( const auto& item : it->toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = parent->depth + 1;
			child->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
			child->type     = DefNodeType::Rotation;
			child->value    = item.toMap().value( "Rotation" ).toString().toStdString();
			parent->childs.insert_or_assign( child->value, child );
			parseDef( child, item.toMap() );
		}
	}
	if ( (it = def.find( "Frames" )) != def.end() )
	{
		int index   = 0;
		const auto frames = it->toList();
		m_numFrames = frames.size();
		for ( const auto& item : frames )
		{
			DefNode* child  = new DefNode;
			child->depth    = parent->depth + 1;
			child->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
			child->type     = DefNodeType::Frame;
			parent->childs.insert_or_assign( "Frame" + std::to_string( index++ ), child );
			parseDef( child, item.toMap() );
		}
	}
	if ( (it = def.find( "Random" )) != def.end() )
	{
		int index            = 0;
		DefNode* randomNode  = new DefNode;
		randomNode->type     = DefNodeType::RandomNode;
		randomNode->depth    = parent->depth + 1;
		randomNode->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
		parent->childs.insert_or_assign( "RandomNode", randomNode );

		for ( const auto& item : it->toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = randomNode->depth + 1;
			child->childPos = randomNode->childPos + QString::number( randomNode->childs.size() + 1 );
			child->type     = DefNodeType::Random;
			randomNode->childs.insert_or_assign( "Random" + std::to_string( index++ ), child );
			randomNode->randomWeights.push_back( item.toMap().value( "Weight" ).toInt() );
			parseDef( child, item.toMap() );
		}
	}

	if ( (it = def.find( "Combine" )) != def.end() )
	{
		int index             = 0;
		DefNode* combineNode  = new DefNode;
		combineNode->type     = DefNodeType::CombineNode;
		combineNode->depth    = parent->depth + 1;
		combineNode->childPos = parent->childPos + QString::number( parent->childs.size() + 1 );
		parent->childs.insert_or_assign( "CombineNode", combineNode );

		for ( const auto& item : it->toList() )
		{
			DefNode* child  = new DefNode;
			child->depth    = parent->depth + 1;
			child->childPos = combineNode->childPos + QString::number( combineNode->childs.size() + 1 );
			child->type     = DefNodeType::Combine;
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

	if ( node->childs.size() && node->childs.begin()->second->type == DefNodeType::Rotation )
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
	if ( node->childs.size() && node->childs.begin()->second->type == DefNodeType::Season )
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
	if ( node->childs.size() && node->childs.begin()->second->type == DefNodeType::Frame )
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

	if ( node->type == DefNodeType::CombineNode && node->childs.size() > 1 )
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
	if ( node->type == DefNodeType::RandomNode )
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
	const auto frameKey = fmt::format( "Frame{}", animFrame );
	if ( maps::try_at( childs, frameKey, temp ) )
	{
		getBaseSpriteDryRun( temp, itemSID, materialSIDs, season, rotation, animFrame );
		return;
	}

	if ( node->type == DefNodeType::CombineNode && childs.size() > 1 )
	{
		getBaseSpriteDryRun( childs.at("Combine0"), itemSID, materialSIDs, season, rotation, animFrame );

		for ( int i = 1; i < childs.size(); ++i )
		{
			getBaseSpriteDryRun( childs.at(fmt::format( "Combine{}", i )), itemSID, materialSIDs, season, rotation, animFrame );
		}
		return;
	}
	if ( node->type == DefNodeType::RandomNode )
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
	spdlog::debug( "Used {} array textures", m_texesUsed );
	m_spriteCreations = scl;
}

SDL_Surface* SpriteFactory::getOrCreatePixelDataAt( int tex ) {
	const auto it = m_pixelData.find( tex );
	if ( it == m_pixelData.end() )
	{
		auto* pixmap = createPixmap( _textureSize, _textureSize );
		m_pixelData.insert_or_assign( tex, pixmap );
		return pixmap;
	} else {
		return it->second;
	}
}

// TODO: Make static & visible outside if needed
void spriteTexCoordsFromId(const auto id, int& x, int& y) {
	x = (id % _texNumSpritesX) * SpriteWidth;
	y = (std::floor((double)id / _texNumSpritesX)) * SpriteHeight;

	spdlog::debug("Used coords for id {}: {}x{}", id, x, y);
}

// TODO: All this can be replaced with a single `cellId`, and calculate the rest of the parameters from it
void SpriteFactory::calculateNextTexCoords() {
	m_lastTexCellX++;

	if ( m_lastTexCellX >= _texNumSpritesX )
	{
		m_lastTexCellX = 0;
		m_lastTexCellY++;

		if ( m_lastTexCellY >= _texNumSpritesY )
		{
			m_lastTexCellY = 0;
			m_lastTexId++;

			if ( m_lastTexId >= TextureLayers )
			{
				throw std::runtime_error( "Tried to use more textures than available layers" );
			}
		}
	}
}

void SpriteFactory::addPixmapToPixelData( Sprite* sprite )
{
	auto season = GameState::seasonString.toStdString();
	if( season.empty() )
	{
		season = "Spring";
	}

	constexpr auto skipFrameData = SpriteWidth * SpriteHeight * SpriteBytesPerPixel;

	int dstX, dstY;

	sprite->texDataBaseId = m_lastTexId;
	sprite->texDataBaseCellX = m_lastTexCellX;
	sprite->texDataBaseCellY = m_lastTexCellY;

	const auto baseId = sprite->uID * SpriteNumFrames * SpriteNumRotations;
	if ( sprite->anim )
	{
		for ( int frameIdx = 0; frameIdx < SpriteNumFrames; ++frameIdx )
		{
			for ( int rotIdx = 0; rotIdx < SpriteNumRotations; ++rotIdx )
			{
				m_texesUsed = std::max( m_texesUsed, static_cast<int>(m_lastTexId + 1) );

				auto* texData = getOrCreatePixelDataAt( m_lastTexId );

				dstX = m_lastTexCellX * SpriteWidth;
				dstY = m_lastTexCellY * SpriteHeight;
				calculateNextTexCoords();

				auto pm = sprite->pixmap( season, rotIdx, frameIdx );

				if ( pm->h == SpriteHeight && pm->w == SpriteWidth )
				{
					copyPixmap(texData, pm, dstX, dstY);
				}
			}
		}
	}
	else
	{

		for ( int rotIdx = 0; rotIdx < SpriteNumRotations; ++rotIdx )
		{
			m_texesUsed = std::max( m_texesUsed, static_cast<int>(m_lastTexId + 1) );
			auto* texData = getOrCreatePixelDataAt( m_lastTexId );

			dstX = m_lastTexCellX * SpriteWidth;
			dstY = m_lastTexCellY * SpriteHeight;
			calculateNextTexCoords();

			auto pm    = sprite->pixmap( season, rotIdx, 0 );

			if ( pm->h == SpriteHeight && pm->w == SpriteWidth )
			{
				copyPixmap( texData, pm, dstX, dstY );
			}
		}
	}
}

void SpriteFactory::addPixmapToPixelData32( Sprite* sprite )
{
	constexpr auto PixmapHeight = 32;

	constexpr auto spriteFrameBytes = SpriteWidth * SpriteHeight * SpriteBytesPerPixel;

	const auto baseId = sprite->uID * SpriteNumFrames * SpriteNumRotations;

	const auto season = GameState::seasonString.toStdString();


	int dstX, dstY;

	// We expect `sprite` to have never been stored
	calculateNextTexCoords();

	for ( int rotIdx = 0; rotIdx < SpriteNumRotations; ++rotIdx )
	{
		m_texesUsed = std::max( m_texesUsed, static_cast<int>(m_lastTexId + 1) );
		auto* texData = getOrCreatePixelDataAt( m_lastTexId );

		dstX = m_lastTexCellX * SpriteWidth;
		dstY = m_lastTexCellY * SpriteHeight;
		calculateNextTexCoords();

		auto pm = sprite->pixmap( season, rotIdx, 0 );

		copyPixmap(texData, pm, dstX, dstY + (SpriteHeight - PixmapHeight) / 2);
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
	return getTintedBaseSprite( baseSprite, string2SDLColor( DBH::materialColor( material ) ) );
}

SDL_Surface* SpriteFactory::getTintedBaseSprite( const std::optional<std::string>& baseSprite, const SDL_Color& color )
{
	if ( !baseSprite || baseSprite->empty() )
	{
		return m_baseSprites["EmptyWall"];
	}

	auto* pm     = m_baseSprites.at(*baseSprite);
	auto* result = clonePixmap( pm );
	tintPixmap( result, color );
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
			// TODO: BaseSprites can be 32x36, so we copy only 32x32. Is this correct?
			copyPixmapFrom( pmfr, m_baseSprites.at(*baseSprite), 0, 0, 32, 32 );
		}
		else if ( const auto* str = std::get_if<std::string>( &tint ) )
		{
			if ( *str == "Material" )
			{
				if ( baseSprite )
				{
					auto pm = getTintedBaseSprite( baseSprite, cm.Material );
					// TODO: BaseSprites can be 32x36, so we copy only 32x32. Is this correct?
					copyPixmapFrom( pmfr, pm, 0, 0, 32, 32 );
					SDL_FreeSurface( pm );
				}
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
				const auto color = isHair ? m_hairColors[*colorInt] : m_colors[*colorInt];
				auto pm = getTintedBaseSprite( baseSprite, color );
				// TODO: BaseSprites can be 32x36, so we copy only 32x32. Is this correct?
				copyPixmapFrom( pmfr, pm, 0, 0, 32, 32 );
				SDL_FreeSurface( pm );
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
			// TODO: BaseSprites can be 32x36, so we copy only 32x32. Is this correct?
			copyPixmapFrom( pmbr, m_baseSprites.at(*baseSprite), 0, 0, 32, 32 );
		}
		else if ( const auto* str = std::get_if<std::string>( &tint ) )
		{
			if ( *str == "Material" )
			{
				if ( baseSprite )
				{
					auto pm = getTintedBaseSprite( baseSprite, cm.Material );
					// TODO: BaseSprites can be 32x36, so we copy only 32x32. Is this correct?
					copyPixmapFrom( pmbr, pm, 0, 0, 32, 32 );
					SDL_FreeSurface( pm );
				}
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
				const auto color = isHair ? m_hairColors[*colorInt] : m_colors[*colorInt];
				auto pm = getTintedBaseSprite( baseSprite, color );
				// TODO: BaseSprites can be 32x36, so we copy only 32x32. Is this correct?
				copyPixmapFrom( pmbr, pm, 0, 0, 32, 32 );
				SDL_FreeSurface( pm );
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

		SDL_FreeSurface( pmfr );
		SDL_FreeSurface( pmfl );
		SDL_FreeSurface( pmbr );
		SDL_FreeSurface( pmbl );
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

SDL_Surface* SpriteFactory::pixelData( int index )
{
	QMutexLocker ml( &m_mutex );
	return m_pixelData[index];
}