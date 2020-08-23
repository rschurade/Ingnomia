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

#include <QMap>
#include <QPixmap>
#include <QString>

enum SpriteRotation : unsigned char
{
	NoRot,
	FR,
	FL = 1,
	BL = 2,
	BR = 3
};

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

	unsigned int uID = 0;
	QString sID      = "";
	char xOffset     = 0;
	char yOffset     = 0;
	float opacity    = 1.0;
	bool anim        = false;
	bool hasTransp   = false;
	QMap<int, int> randomNumbers;
	QString m_type = "";
};

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

	QPixmap m_pixmap;
};

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

	QHash<QString, Sprite*> m_sprites;
};

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

	QList<Sprite*> m_sprites;
};

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

	QList<Sprite*> m_sprites;
};
