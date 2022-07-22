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

#include "../base/dbstructs.h"
#include "../gfx/sprite.h"

#include <QBitmap>
#include <QGraphicsPixmapItem>

#include <SDL.h>
#include <vector>

#include <range/v3/view.hpp>

struct SDL_Surface;

struct DefNode;

struct SpriteCreation
{
	std::string itemSID;
	std::vector<std::string> materialSIDs;
	absl::flat_hash_map<int, int> random;
	unsigned int uID        = 0;
	unsigned int creatureID = 0;
};

class SpriteFactory
{
private:
	void parseDef( DefNode* parent, QVariantMap def );

	Sprite* createSpriteMaterial( const std::string& itemSID, const std::vector<std::string>& materialSIDs, const std::string& key );
	Sprite* getBaseSprite( const DefNode* node, const std::string& itemSID, const std::vector<std::string>& materialSIDs, int materialID = 0 );

	std::string createSpriteMaterialDryRun( const std::string& itemSID, const std::vector<std::string>& materialSIDs );
	void getBaseSpriteDryRun( const DefNode* node, const std::string& itemSID, const std::vector<std::string>& materialSIDs, const std::string& season, const std::string& rotation, const int animFrame );

	int numFrames( const DefNode* node, const std::string& itemSID, const std::vector<std::string>& materialSIDs, const std::string& season, const std::string& rotation );
	bool containsRandom( const std::string& itemSID, const std::vector<std::string>& materialSIDs );

	std::string getMaterialType( const std::string& materialSID );

	SDL_Surface* extractPixmap( const std::string& sourcePNG, QVariantMap def );
	unsigned char rotationToChar( QString suffix );

	// base sprites and sources for creation
	absl::flat_hash_map<std::string, SDL_Surface*> m_pixmapSources;
	absl::btree_map<std::string, SDL_Surface*> m_baseSprites;
	absl::btree_map<std::string, DefNode*> m_spriteDefinitions;
	absl::btree_map<std::string, QVariantMap> m_spriteDefVMs;

	absl::btree_map<QString, unsigned int> m_thoughtBubbleIDs;

	// cached DB values for faster lookup
	std::vector<std::string> m_seasons;
	absl::flat_hash_map<std::string, std::string> m_materialTypes;

	//pixel data for array textures
	absl::flat_hash_map<uint32_t, std::vector<uint8_t>> m_pixelData;
	int m_texesUsed = 0;

	bool m_textureAdded         = false;
	bool m_creatureTextureAdded = false;

	// intermediate variables during sprite creation
	QString m_offset  = "";
	float m_opacity   = 1.0;
	float m_numFrames = 1;
	absl::flat_hash_map<int, int> m_randomNumbers;

	std::vector<SDL_Color> m_colors;
	std::vector<SDL_Color> m_hairColors;

	// created sprites for current game
	QList<Sprite*> m_sprites;

	absl::btree_map<std::string, int> m_spriteIDs;
	absl::btree_map<unsigned int, int> m_creatureSpriteIDs;

	QList<SpriteCreation> m_spriteCreations;

	void addPixmapToPixelData( Sprite* sprite );
	void addPixmapToPixelData32( Sprite* sprite );

	void addEmptyRows( int startIndex, int rows, std::vector<uint8_t>& pixelData );

	void createStandardSprites();

	SDL_Surface* getTintedBaseSprite( const std::optional<std::string>& baseSprite, const std::string& material );

	Sprite* createSprite2( const std::string& itemSID, const std::vector<std::string>& materialSID, const absl::flat_hash_map<int, int>& random = absl::flat_hash_map<int, int>() );
	
	void printDebug();

	SDL_Surface* pixmap( const std::string& name );

	SDL_Surface* baseSprite( const std::string& id );

	QMutex m_mutex;

	bool init();

	std::vector<uint8_t>& getOrCreatePixelDataAt( int tex );

public:
	SpriteFactory();
	~SpriteFactory();

	Sprite* createSprite( const std::string& itemSID, const std::vector<std::string>& materialSID, const absl::flat_hash_map<int, int>& random = absl::flat_hash_map<int, int>() );
	Sprite* createAnimalSprite( const std::string& spriteSID, const absl::flat_hash_map<int, int>& random = absl::flat_hash_map<int, int>() );
	Sprite* getSprite( const int id );

	Sprite* setCreatureSprite( const unsigned int gnomeUID, const std::vector<DBS::Creature_Parts>& components, const std::vector<DBS::Creature_Parts>& componentsBack, bool isDead = false );
	//unsigned int setAutomatonSprite( const unsigned int automatonID, const unsigned int id );
	Sprite* getCreatureSprite( const unsigned int id, unsigned int& spriteID );

	QList<SpriteCreation> spriteCreations()
	{
		return m_spriteCreations;
	}

	void createSprites( QList<SpriteCreation> scl );

	bool textureAdded();
	bool creatureTextureAdded();

	void forceUpdate();

	unsigned int thoughtBubbleID( QString sid );
	
	int texesUsed();
	
	unsigned int size();
	
	const std::vector<uint8_t>& pixelData( int index );
	
};