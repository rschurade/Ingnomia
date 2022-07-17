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

#include <absl/container/btree_map.h>
#include <absl/container/flat_hash_map.h>
#include <string>

struct SDL_Surface;

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

	virtual SDL_Surface* pixmap( std::string season, unsigned char rotation, unsigned char animationStep ) = 0;
	virtual void setPixmap( SDL_Surface* pm, std::string season, unsigned char rotation )                   = 0;

	virtual void applyEffect( std::string effect )                  = 0;
	virtual void applyTint( std::string tint, std::string materialSID ) = 0;

	virtual void combine( Sprite* other, std::string season, unsigned char rotation, unsigned char animationStep ) = 0;

	unsigned int uID = 0;
	std::string sID      = "";
	char xOffset     = 0;
	char yOffset     = 0;
	float opacity    = 1.0;
	bool anim        = false;
	bool hasTransp   = false;
	absl::btree_map<int, int> randomNumbers;
	std::string m_type = "";
};

class SpritePixmap : public Sprite
{
public:
	SpritePixmap( SDL_Surface* pixmap );
	SpritePixmap( SDL_Surface* pixmap, std::string offset );
	SpritePixmap( const SpritePixmap& other );
	~SpritePixmap();

	SDL_Surface* pixmap( std::string season, unsigned char rotation, unsigned char animationStep );
	void setPixmap( SDL_Surface* pm, std::string season, unsigned char rotation );

	void applyEffect( std::string effect );
	void applyTint( std::string tint, std::string materialSID );

	void combine( Sprite* other, std::string season, unsigned char rotation, unsigned char animationStep );

	SDL_Surface* m_pixmap;
	// FIXME: Remove this and do proper resource management, probably with std::shared_ptr in this case
	bool m_pixmapOwned;
};

class SpriteSeasons : public Sprite
{
public:
	SpriteSeasons();
	SpriteSeasons( const SpriteSeasons& other );
	~SpriteSeasons();

	SDL_Surface* pixmap( std::string season, unsigned char rotation, unsigned char animationStep );
	void setPixmap( SDL_Surface* pm, std::string season, unsigned char rotation );

	void applyEffect( std::string effect );
	void applyTint( std::string tint, std::string materialSID );

	void combine( Sprite* other, std::string season, unsigned char rotation, unsigned char animationStep );

	absl::flat_hash_map<std::string, Sprite*> m_sprites;
};

class SpriteRotations : public Sprite
{
public:
	SpriteRotations();
	SpriteRotations( const SpriteRotations& other );
	~SpriteRotations();

	SDL_Surface* pixmap( std::string season, unsigned char rotation, unsigned char animationStep );
	void setPixmap( SDL_Surface* pm, std::string season, unsigned char rotation );

	void applyEffect( std::string effect );
	void applyTint( std::string tint, std::string materialSID );

	void combine( Sprite* other, std::string season, unsigned char rotation, unsigned char animationStep );

	std::vector<Sprite*> m_sprites;
};

class SpriteFrames : public Sprite
{
public:
	SpriteFrames();
	SpriteFrames( const SpriteFrames& other );
	~SpriteFrames();

	SDL_Surface* pixmap( std::string season, unsigned char rotation, unsigned char animationStep );
	void setPixmap( SDL_Surface* pm, std::string season, unsigned char rotation );

	void applyEffect( std::string effect );
	void applyTint( std::string tint, std::string materialSID );

	void combine( Sprite* other, std::string season, unsigned char rotation, unsigned char animationStep );

	std::vector<Sprite*> m_sprites;
};
