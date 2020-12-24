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

#include "object.h"

#include <QString>

class Game;

enum class GrowLight : unsigned char
{
	SUN,
	DARK,
	SUN_AND_DARK
};

enum class OnTickReturn : unsigned char
{
	NOOP,
	UPDATE,
	DESTROY
};

class Plant : public Object
{
public:
	Plant();
	Plant( Position& pos, QString ID, bool fullyGrown, Game* game );
	Plant( QVariant values, Game* game );
	~Plant();

	virtual QVariant serialize() const;

	OnTickReturn onTick( quint64 tickNumber, bool dayChanged, bool seasonChanged );

	OnTickReturn liveOneTick( bool dayChanged = false, bool seasonChanged = false );

	QString getDesignation();

	bool isTree()
	{
		return m_isTree;
	}
	bool isMushroom()
	{
		return m_isMushroom;
	}
	bool isPlant()
	{
		return m_isPlant;
	}
	bool isFruitTree();

	bool fullGrown()
	{
		return m_fullyGrown;
	}
	bool producesHarvest()
	{
		return m_producesHarvest;
	}
	bool harvestable();
	bool matureWood()
	{
		return m_isTree && m_matureWood;
	}

	bool hasAlpha()
	{
		return m_hasAlpha;
	}
	int lightIntensity()
	{
		return m_lightIntensity;
	}

	//return true when plant is destroyed
	bool fell();
	//return true when plant is destroyed
	// param pos Position of the harvest to be created
	bool harvest( Position& pos );
	//return true when plant is destroyed
	bool reduceOneGrowLevel();

	void setSprite( Sprite* s )
	{
		m_sprite = s;
	}
	Sprite* getSprite()
	{
		return m_sprite;
	}

	void speedUpGrowth( unsigned int ticks );

	bool growsThisSeason();
	void setGrowsThisSeason();

	static bool testLayoutMulti( QString layoutSID, Position rootPos, Game* game );

private:
	QPointer<Game> g;

	QString m_plantID;

	Sprite* m_sprite = 0;

	qint8 m_state                   = 0;
	unsigned int m_ticksToNextState = 0;
	bool m_fullyGrown               = false;
	bool m_matureWood               = false;
	bool m_harvestable              = false;
	bool m_producesHarvest          = false;

	bool m_isTree             = false;
	bool m_isFruitTree        = false;
	bool m_isMulti            = false;
	bool m_isPlant            = false;
	bool m_isMushroom         = false;
	bool m_isUndergroundPlant = false;
	bool m_hasAlpha           = false;
	int m_lightIntensity      = 0;

	bool m_growsThisSeason = true;

	GrowLight m_growLight = GrowLight::SUN;

	int m_numFruits = 0;

	void setGrowTime();
	void updateState();
	void layoutMulti( QString layoutSID, bool withFruit = false );
};
