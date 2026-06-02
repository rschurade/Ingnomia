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
/** @file sprite.h
 *  @brief Sprite class hierarchy: abstract Sprite base plus leaf and container variants
 *         (pixmap, per-season, per-rotation, animated frames).
 */
#pragma once

#include <QMap>
#include <QPixmap>
#include <QString>

/// @brief Four facing directions used to pick a SpriteRotations sub-sprite.
enum SpriteRotation : unsigned char
{
	NoRot,
	FR,        ///< Front-right.
	FL = 1,    ///< Front-left.
	BL = 2,    ///< Back-left.
	BR = 3     ///< Back-right.
};

/// @brief Abstract base for all sprite variants. Holds identity, draw offsets, opacity,
///        and animation/transparency flags; subclasses implement pixmap access and
///        in-place transformations.
class Sprite
{
public:
	Sprite();
	Sprite( const Sprite& other );
	virtual ~Sprite();

	virtual QPixmap& pixmap( QString season, unsigned char rotation, unsigned char animationStep ) = 0;
	virtual void setPixmap( QPixmap pm, QString season, unsigned char rotation )                   = 0;

	virtual void applyEffect( QString effect )                  = 0;
	virtual void applyTint( QString tint, QString materialSID ) = 0;

	virtual void combine( Sprite* other, QString season, unsigned char rotation, unsigned char animationStep ) = 0;

	unsigned int uID = 0;           ///< Numeric index assigned by SpriteFactory.
	QString sID      = "";          ///< Optional string ID for debugging.
	char xOffset     = 0;           ///< Draw X offset in pixels (signed).
	char yOffset     = 0;           ///< Draw Y offset in pixels (signed).
	float opacity    = 1.0;         ///< Global opacity (affected by tint alpha).
	bool anim        = false;       ///< True if this sprite has multiple animation frames.
	bool hasTransp   = false;       ///< True if the sprite contains transparent pixels.
	QMap<int, int> randomNumbers;   ///< Per-RandomNode pick history for deterministic rebuild.
	QString m_type = "";            ///< Discriminator string: "pixmap", "seasons", "rotations", "frames".
};

/// @brief Leaf sprite wrapping a single QPixmap. Ignores season/rotation/animationStep arguments.
class SpritePixmap : public Sprite
{
public:
	SpritePixmap( QPixmap pixmap );
	SpritePixmap( QPixmap pixmap, QString offset );
	SpritePixmap( const SpritePixmap& other );
	~SpritePixmap();

	QPixmap& pixmap( QString season, unsigned char rotation, unsigned char animationStep );
	void setPixmap( QPixmap pm, QString season, unsigned char rotation );

	void applyEffect( QString effect );
	void applyTint( QString tint, QString materialSID );

	void combine( Sprite* other, QString season, unsigned char rotation, unsigned char animationStep );

	QPixmap m_pixmap;  ///< The stored pixmap.
};

/// @brief Container sprite that holds one sub-sprite per season (Spring, Summer, …).
class SpriteSeasons : public Sprite
{
public:
	SpriteSeasons();
	SpriteSeasons( const SpriteSeasons& other );
	~SpriteSeasons();

	QPixmap& pixmap( QString season, unsigned char rotation, unsigned char animationStep );
	void setPixmap( QPixmap pm, QString season, unsigned char rotation );

	void applyEffect( QString effect );
	void applyTint( QString tint, QString materialSID );

	void combine( Sprite* other, QString season, unsigned char rotation, unsigned char animationStep );

	QHash<QString, Sprite*> m_sprites;  ///< Sub-sprites keyed by season string.
};

/// @brief Container sprite with one sub-sprite per facing direction (FR, FL, BL, BR).
class SpriteRotations : public Sprite
{
public:
	SpriteRotations();
	SpriteRotations( const SpriteRotations& other );
	~SpriteRotations();

	QPixmap& pixmap( QString season, unsigned char rotation, unsigned char animationStep );
	void setPixmap( QPixmap pm, QString season, unsigned char rotation );

	void applyEffect( QString effect );
	void applyTint( QString tint, QString materialSID );

	void combine( Sprite* other, QString season, unsigned char rotation, unsigned char animationStep );

	QList<Sprite*> m_sprites;  ///< Sub-sprites indexed by SpriteRotation enum.
};

/// @brief Container sprite that cycles through animation frames using animationStep modulo count.
class SpriteFrames : public Sprite
{
public:
	SpriteFrames();
	SpriteFrames( const SpriteFrames& other );
	~SpriteFrames();

	QPixmap& pixmap( QString season, unsigned char rotation, unsigned char animationStep );
	void setPixmap( QPixmap pm, QString season, unsigned char rotation );

	void applyEffect( QString effect );
	void applyTint( QString tint, QString materialSID );

	void combine( Sprite* other, QString season, unsigned char rotation, unsigned char animationStep );

	QList<Sprite*> m_sprites;  ///< One sub-sprite per animation frame.
};
