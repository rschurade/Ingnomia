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

#include "../gfx/sprite.h"

#include <QBitmap>
#include <QGraphicsPixmapItem>

#include <SDL.h>
#include <vector>

#include <range/v3/view.hpp>

struct SDL_Surface;

struct DefNode
{
	DefNode()
	{
		//parent = 0;

		type       = "";
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

	QString type;
	QString offset;
	std::string effect;
	std::string tint;
	QString value;
	QString baseSprite;
	QString defaultMaterial;
	int depth;
	int numFrames;
	QString childPos;
	QList<int> randomWeights;
	bool rot90 = false;
	bool hasTransp = false;

	//DefNode* parent;
	absl::btree_map<QString, DefNode*> childs;
};

struct SpriteCreation
{
	QString itemSID;
	QStringList materialSIDs;
	absl::btree_map<int, int> random;
	unsigned int uID        = 0;
	unsigned int creatureID = 0;
};

class SpriteFactory
{
private:
	void parseDef( DefNode* parent, QVariantMap def );

	Sprite* createSpriteMaterial( const QString itemSID, const QStringList materialSIDs, const QString key );
	Sprite* getBaseSprite( const DefNode* node, const QString itemSID, const QStringList materialSIDs, int materialID = 0 );

	QString createSpriteMaterialDryRun( const QString itemSID, const QStringList materialSIDs );
	void getBaseSpriteDryRun( const DefNode* node, const QString itemSID, const QStringList materialSIDs, const std::string& season, const QString rotation, const int animFrame );

	int numFrames( const DefNode* node, const QString itemSID, const QStringList materialSIDs, const std::string& season, const QString rotation );
	bool containsRandom( const QString itemSID, const QStringList materialSIDs );

	QString getMaterialType( const std::string& materialSID );

	SDL_Surface* extractPixmap( QString sourcePNG, QVariantMap def );
	unsigned char rotationToChar( QString suffix );

	// base sprites and sources for creation
	absl::flat_hash_map<QString, SDL_Surface*> m_pixmapSources;
	absl::btree_map<QString, SDL_Surface*> m_baseSprites;
	absl::btree_map<QString, DefNode*> m_spriteDefinitions;
	absl::btree_map<QString, QVariantMap> m_spriteDefVMs;

	absl::btree_map<QString, unsigned int> m_thoughtBubbleIDs;

	// cached DB values for faster lookup
	std::vector<std::string> m_seasons;
	absl::btree_map<QString, QString> m_materialTypes;

	//pixel data for array textures
	std::vector<std::vector<uint8_t>> m_pixelData;
	int m_texesUsed = 0;

	bool m_textureAdded         = false;
	bool m_creatureTextureAdded = false;

	// intermediate variables during sprite creation
	QString m_offset  = "";
	float m_opacity   = 1.0;
	float m_numFrames = 1;
	absl::btree_map<int, int> m_randomNumbers;

	std::vector<SDL_Color> m_colors;
	std::vector<SDL_Color> m_hairColors;

	// created sprites for current game
	QList<Sprite*> m_sprites;

	absl::btree_map<QString, int> m_spriteIDs;
	absl::btree_map<unsigned int, int> m_creatureSpriteIDs;

	QList<SpriteCreation> m_spriteCreations;

	void addPixmapToPixelData( Sprite* sprite );
	void addPixmapToPixelData32( Sprite* sprite );

	void addEmptyRows( int startIndex, int rows, std::vector<uint8_t>& pixelData );

	void createStandardSprites();

	SDL_Surface* getTintedBaseSprite( QString baseSprite, QString material );

	Sprite* createSprite2( const QString itemSID, QStringList materialSID, const absl::btree_map<int, int>& random = absl::btree_map<int, int>() );
	
	void printDebug();

	SDL_Surface* pixmap( QString name );

	SDL_Surface* baseSprite( QString id );

	QMutex m_mutex;

	bool init();

public:
	SpriteFactory();
	~SpriteFactory();

	

	Sprite* createSprite( const QString itemSID, QStringList materialSID, const absl::btree_map<int, int>& random = absl::btree_map<int, int>() );
	Sprite* createAnimalSprite( const QString spriteSID, const absl::btree_map<int, int>& random = absl::btree_map<int, int>() );
	Sprite* getSprite( const int id );

	Sprite* setCreatureSprite( const unsigned int gnomeUID, QVariantList components, QVariantList componentsBack, bool isDead = false );
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
	
	std::vector<uint8_t> pixelData( int index );
	
};