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
#include <QPixmap>
#include <QVector>

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

	QString type;
	QString offset;
	QString effect;
	QString tint;
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
	QMap<QString, DefNode*> childs;
};

struct SpriteCreation
{
	QString itemSID;
	QStringList materialSIDs;
	QMap<int, int> random;
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
	void getBaseSpriteDryRun( const DefNode* node, const QString itemSID, const QStringList materialSIDs, const QString season, const QString rotation, const int animFrame );

	int numFrames( const DefNode* node, const QString itemSID, const QStringList materialSIDs, const QString season, const QString rotation );
	bool containsRandom( const QString itemSID, const QStringList materialSIDs );

	QString getMaterialType( const QString materialSID );

	QPixmap extractPixmap( QString sourcePNG, QVariantMap def );
	unsigned char rotationToChar( QString suffix );

	// base sprites and sources for creation
	QHash<QString, QPixmap> m_pixmapSources;
	QMap<QString, QPixmap> m_baseSprites;
	QMap<QString, DefNode*> m_spriteDefinitions;
	QMap<QString, QVariantMap> m_spriteDefVMs;

	QMap<QString, unsigned int> m_thoughtBubbleIDs;

	// cached DB values for faster lookup
	QStringList m_seasons;
	QMap<QString, QString> m_materialTypes;

	//pixel data for array textures
	QVector<QVector<uint8_t>> m_pixelData;
	int m_texesUsed = 0;

	bool m_textureAdded         = false;
	bool m_creatureTextureAdded = false;

	// intermediate variables during sprite creation
	QString m_offset  = "";
	float m_opacity   = 1.0;
	float m_numFrames = 1;
	QMap<int, int> m_randomNumbers;

	QList<QColor> m_colors;
	QList<QColor> m_hairColors;

	// created sprites for current game
	QList<Sprite*> m_sprites;

	QMap<QString, int> m_spriteIDs;
	QMap<unsigned int, int> m_creatureSpriteIDs;

	QList<SpriteCreation> m_spriteCreations;

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

	QMutex m_mutex;

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