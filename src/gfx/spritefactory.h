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
/** @file spritefactory.h
 *  @brief SpriteFactory and its supporting types: DefNode (parsed sprite definition tree)
 *         and SpriteCreation (replay record). The factory owns the tilesheet cache,
 *         sprite cache, GPU pixel-data buffers, and all sprite creation entry points.
 */
#pragma once

#include "../gfx/sprite.h"

#include <QBitmap>
#include <QGraphicsPixmapItem>
#include <QPixmap>
#include <QVector>

/// @brief Node in the parsed sprite definition tree. Each node holds either a leaf
///        base-sprite reference or a keyed set of child nodes (by material, material type,
///        season, rotation, frame, random pick, or combine layer).
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
		for( auto& child : childs )
		{
			delete child;
		}
	}

	QString type;                    ///< Discriminator: "root", "ByMaterial", "Season", "Rotation", "Frame", "Random", "RandomNode", "Combine", "CombineNode".
	QString offset;                  ///< Pixel offset "x y" propagated to SpritePixmap.
	QString effect;                  ///< Effect name (e.g. "FlipHorizontal").
	QString tint;                    ///< Tint spec ("Material" or "R G B A").
	QString value;                   ///< Key this node is keyed on in its parent (material id, season id, etc.).
	QString baseSprite;              ///< Leaf base-sprite key; empty for container nodes.
	QString defaultMaterial;         ///< Fallback child key when material lookup misses.
	int depth;                       ///< Tree depth (root = 0).
	int numFrames;                   ///< Total frame count under this subtree (root-level only).
	QString childPos;                ///< Positional path string used as the RandomNode seed key.
	QList<int> randomWeights;        ///< Weights for RandomNode children (parallel to childs).
	bool rot90 = false;              ///< Unused flag reserved for 90° rotation handling.
	bool hasTransp = false;          ///< True if this subtree produces pixels with alpha < 1.

	//DefNode* parent;
	QMap<QString, DefNode*> childs;  ///< Child nodes keyed by discriminator value.
};

/// @brief Replay record capturing enough state to rebuild a previously created sprite after
///        a save-game load. Stores the item/material inputs, any random picks, and the
///        assigned uID (plus creature UID for creature sprites).
struct SpriteCreation
{
	QString itemSID;              ///< Item/sprite string ID.
	QStringList materialSIDs;     ///< Material list used at creation time.
	QMap<int, int> random;        ///< Recorded random picks indexed by RandomNode childPos.
	unsigned int uID        = 0;  ///< Sprite UID assigned at creation.
	unsigned int creatureID = 0;  ///< Nonzero if this is a creature sprite; 1 for animal, otherwise creature UID.
};

/// @brief Sprite asset pipeline. Loads tilesheets, parses the Sprites DB table into DefNode
///        trees, builds composite Sprite instances on demand, composes creature sprites from
///        equipment layer stacks, and maintains the GPU-bound RGBA8 pixel-data buffers for
///        the sprite array texture.
class SpriteFactory
{
private:
	void parseDef( DefNode* parent, QVariantMap def );

	Sprite* createSpriteMaterial( const QString itemSID, const QStringList materialSIDs, const QString key );
	Sprite* getBaseSprite( const DefNode* node, const QString itemSID, const QStringList materialSIDs, int materialID = 0 );

	QString createSpriteMaterialDryRun( const QString itemSID, const QStringList materialSIDs );
	void getBaseSpriteDryRun( const DefNode* node, const QString itemSID, const QStringList materialSIDs, const QString season, const QString rotation, const int animFrame );

	int numFrames( const DefNode* node, const QString itemSID, const QStringList materialSIDs, const QString season, const QString rotation );
	bool containsRandom( const QString itemSID, const QStringList materialSIDs );

	QString getMaterialType( const QString materialSID );

	QPixmap extractPixmap( QString sourcePNG, QVariantMap def );
	unsigned char rotationToChar( QString suffix );

	// base sprites and sources for creation
	QHash<QString, QPixmap> m_pixmapSources;       ///< Loaded tilesheet PNGs keyed by filename.
	QMap<QString, QPixmap> m_baseSprites;          ///< Base sprites extracted from tilesheets, keyed by BaseSprite ID.
	QMap<QString, DefNode*> m_spriteDefinitions;   ///< Parsed DefNode tree per sprite ID (owned).
	QMap<QString, QVariantMap> m_spriteDefVMs;     ///< Raw sprite definition QVariantMaps keyed by sprite ID.

	QMap<QString, unsigned int> m_thoughtBubbleIDs; ///< Sprite UIDs for each thought-bubble kind.

	// cached DB values for faster lookup
	QStringList m_seasons;                         ///< Seasons DB ids in order.
	QMap<QString, QString> m_materialTypes;        ///< Material ID → material type ("Metal", "Wood", …).

	//pixel data for array textures
	QVector<QVector<uint8_t>> m_pixelData;         ///< RGBA8 pixel buffers, one per 512-slot array-texture slice.
	int m_texesUsed = 0;                           ///< Number of texture slices currently populated.

	bool m_textureAdded         = false;           ///< Dirty flag for tile textures.
	bool m_creatureTextureAdded = false;           ///< Dirty flag for creature textures.

	// intermediate variables during sprite creation
	QString m_offset  = "";                        ///< Last offset seen while walking a DefNode tree.
	float m_opacity   = 1.0;                       ///< Last opacity seen while walking a DefNode tree.
	float m_numFrames = 1;                         ///< Frame count detected during the current parse.
	QMap<int, int> m_randomNumbers;                ///< Per-RandomNode picks for the current creation.

	QList<QColor> m_colors;                        ///< Dye colour palette.
	QList<QColor> m_hairColors;                    ///< Hair colour palette.

	// created sprites for current game
	QList<Sprite*> m_sprites;                      ///< All created sprites indexed by Sprite::uID (index 0 unused).

	QMap<QString, int> m_spriteIDs;                ///< Cache key → sprite UID for deduplication.
	QMap<unsigned int, int> m_creatureSpriteIDs;   ///< Creature UID → sprite UID.

	QList<SpriteCreation> m_spriteCreations;       ///< Replay log of every createSprite call in this session.

	void addPixmapToPixelData( Sprite* sprite );
	void addPixmapToPixelData32( Sprite* sprite );

	void addEmptyRows( int startIndex, int rows, QVector<uint8_t>& pixelData );

	void tintPixmap( QPixmap& pm, QColor color );

	void createStandardSprites();

	QPixmap getTintedBaseSprite( QString baseSprite, QString material );

	Sprite* createSprite2( const QString itemSID, QStringList materialSID, const QMap<int, int>& random = QMap<int, int>() );
	
	void printDebug();

	QStringList pixmaps();
	QPixmap pixmap( QString name );

	QPixmap baseSprite( QString id );

	QMutex m_mutex;                                ///< Protects sprite cache access across threads.

	bool init();

public:
	SpriteFactory();
	~SpriteFactory();

	

	Sprite* createSprite( const QString itemSID, QStringList materialSID, const QMap<int, int>& random = QMap<int, int>() );
	Sprite* createAnimalSprite( const QString spriteSID, const QMap<int, int>& random = QMap<int, int>() );
	Sprite* getSprite( const int id );

	Sprite* setCreatureSprite( const unsigned int gnomeUID, QVariantList components, QVariantList componentsBack, bool isDead = false );
	//unsigned int setAutomatonSprite( const unsigned int automatonID, const unsigned int id );
	Sprite* getCreatureSprite( const unsigned int id, unsigned int& spriteID );

	/// @brief Returns the replay log of all sprite creations so far (for save games).
	/// @return Copy of m_spriteCreations.
	QList<SpriteCreation> spriteCreations()
	{
		return m_spriteCreations;
	}

	void createSprites( QList<SpriteCreation> scl );

	

	bool textureAdded();
	bool creatureTextureAdded();

	void forceUpdate();

	void addPixmapSource( QString name, QString path );

	unsigned int thoughtBubbleID( QString sid );
	
	int texesUsed();
	
	unsigned int size();
	
	QVector<uint8_t> pixelData( int index );
	
};