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
#include "item.h"

#include "../base/db.h"
#include "../base/position.h"
#include "../base/util.h"
#include "../gfx/spritefactory.h"
#include "../gui/strings.h"

#include "jobmanager.h"

#include <QDebug>
#include <QVariantMap>

Item::Item() :
	Object( Position( 0, 0, 0 ) )
{
}

Item::Item( Position& pos, QString itemSID, QString materialSID ) :
	Object( pos )
{
	m_itemUID     = DBH::itemUID( itemSID );
	m_materialUID = DBH::materialUID( materialSID );
	int value     = DB::select( "Value", "Items", itemSID ).toFloat() * DB::select( "Value", "Materials", materialSID ).toFloat();
	setValue( value );
	{
		setNutritionalValue( DB::select( "EatValue", "Items", itemSID ).toInt() );
		setDrinkValue( DB::select( "DrinkValue", "Items", itemSID ).toInt() );
	}
}

Item::Item( QVariantMap in ) :
	Object( in )
{
	/*
	m_id = in.value( "ID" ).toUInt();
	m_position = Position( in.value( "Position" ).toString() );
	m_spriteID = in.value( "SpriteID" ).toUInt();
	*/
	QString itemSID = in.value( "ItemSID" ).toString();
	m_itemUID       = DBH::itemUID( itemSID );
	m_materialUID   = DBH::materialUID( in.value( "MaterialSID" ).toString() );
	m_stockpileID = in.value( "StockpileID" ).toUInt();
	m_containerID = in.value( "ContainerID" ).toUInt();

	m_location      = static_cast<ItemLocation>( in.value( "Location" ).toUInt() );
	m_locationOwner = in.value( "LocationOwner" ).toUInt();
	m_claim         = static_cast<ItemClaim>( in.value( "Claim" ).toUInt() );
	m_claimOwner    = in.value( "ClaimOwner" ).toUInt();

	m_value         = in.value( "Value" ).toUInt();
	m_madeBy        = in.value( "MadeBy" ).toUInt();
	m_quality       = in.value( "Quality" ).toUInt();

	if ( in.value( "Extra" ).toBool() )
	{
		m_extraData = new ItemExtraData;
	}

	if ( in.contains( "Components" ) )
	{
		QVariantList vlc = in.value( "Components" ).toList();
		for ( auto item : vlc )
		{
			QVariantMap cvm = item.toMap();
			ItemMaterial im = { (unsigned int)DBH::itemUID( cvm.value( "ItSID" ).toString() ),
								(unsigned int)DBH::materialUID( cvm.value( "MaSID" ).toString() ) };
			m_extraData->components.append( im );
		}
	}

	if ( in.contains( "EatValue" ) )
	{
		m_extraData->nutritionalValue = in.value( "EatValue" ).toUInt();
	}
	if ( in.contains( "DrinkValue" ) )
	{
		m_extraData->drinkValue = in.value( "DrinkValue" ).toUInt();
	}
}

Item::Item( const Item& other ) :
	m_itemUID( other.m_itemUID ),
	m_materialUID( other.m_materialUID ),
	m_location( other.m_location ),
	m_locationOwner( other.m_locationOwner ),
	m_claim( other.m_claim ),
	m_claimOwner( other.m_claimOwner ),
	m_stockpileID( other.m_stockpileID ),
	m_containerID( other.m_containerID ),
	m_value( other.m_value ),
	m_madeBy( other.m_madeBy ),
	m_quality( other.m_quality ),
	m_extraData( nullptr )
{
	m_id       = other.m_id;
	m_position = other.m_position;
	m_spriteID = other.m_spriteID;

	if ( other.m_extraData != nullptr )
	{
		m_extraData                   = new ItemExtraData;
		m_extraData->components       = other.m_extraData->components;
		m_extraData->nutritionalValue = other.m_extraData->nutritionalValue;
		m_extraData->drinkValue       = other.m_extraData->drinkValue;
	}
}

Item::~Item()
{
	if ( m_extraData != nullptr )
	{
		delete m_extraData;
	}
}

QVariant Item::serialize() const
{
	QVariantMap out;
	Object::serialize( out );
	/*
	out.insert( "ID", m_id );
	out.insert( "Position", m_position.toString() );
	out.insert( "SpriteID", m_spriteID );
	*/
	out.insert( "ItemSID", itemSID() );
	out.insert( "MaterialSID", materialSID() );
	out.insert( "StockpileID", m_stockpileID );
	out.insert( "ContainerID", m_containerID );
	out.insert( "Location", static_cast<unsigned int>( m_location ) );
	out.insert( "LocationOwner", m_locationOwner );
	out.insert( "Claim", static_cast<unsigned int>( m_claim ) );
	out.insert( "ClaimOwner", m_claimOwner );
	out.insert( "Value", m_value );
	out.insert( "MadeBy", m_madeBy );
	out.insert( "Quality", m_quality );

	if ( m_extraData != nullptr )
	{
		out.insert( "Extra", true );

		out.insert( "EatValue", m_extraData->nutritionalValue );
		out.insert( "DrinkValue", m_extraData->drinkValue );

		if ( !m_extraData->components.empty() )
		{
			QVariantList vli;
			for ( auto comp : m_extraData->components )
			{
				QVariantMap vm;
				vm.insert( "ItSID", DBH::itemSID( comp.itemUID ) );
				vm.insert( "MaSID", DBH::materialSID( comp.materialUID ) );
				vli.push_back( vm );
			}
			out.insert( "Components", vli );
		}
	}
	return out;
}

unsigned short Item::materialUID() const
{
	return m_materialUID;
}

unsigned short Item::itemUID() const
{
	return m_itemUID;
}

QString Item::getPixmapSID() const
{
	return DBH::spriteID( DBH::itemSID( m_itemUID ) ) + "_" + DBH::materialSID( m_materialUID );
}

QString Item::getDesignation() const
{
	return S::s( "$MaterialName_" + DBH::materialSID( m_materialUID ) ) + " " + S::s( "$ItemName_" + DBH::itemSID( m_itemUID ) );
}

QString Item::itemSID() const
{
	return DBH::itemSID( m_itemUID );
}

QString Item::materialSID() const
{
	return DBH::materialSID( m_materialUID );
}

QString Item::combinedSID() const
{
	return DBH::itemSID( m_itemUID ) + "_" + DBH::materialSID( m_materialUID );
}

int Item::distanceSquare( const Position& pos, int zWeight ) const
{
	return ( m_position.x - pos.x ) * ( m_position.x - pos.x ) + ( m_position.y - pos.y ) * ( m_position.y - pos.y ) + ( m_position.z - pos.z ) * ( m_position.z - pos.z ) * zWeight;
}

unsigned int Item::isInJob() const
{
	return ( m_claim == ItemClaim::Job ) ? m_claimOwner : 0;
}

void Item::setInJob( unsigned int job )
{
	if ( job != 0 )
	{
		setClaim( ItemClaim::Job, job );
	}
	else if ( m_claim == ItemClaim::Job )
	{
		setClaim( ItemClaim::None, 0 );
	}
}

unsigned int Item::isHeldBy() const
{
	return ( m_location == ItemLocation::Carried ) ? m_locationOwner : 0;
}

void Item::setHeldBy( unsigned int creatureID )
{
	if ( creatureID != 0 )
	{
		setLocation( ItemLocation::Carried, creatureID );
	}
	else
	{
		setLocation( ItemLocation::Ground, 0 );
	}
}

unsigned int Item::isUsedBy() const
{
	return ( m_claim == ItemClaim::Equipped ) ? m_claimOwner : 0;
}

void Item::setUsedBy( unsigned int creatureID )
{
	if ( creatureID != 0 )
	{
		setClaim( ItemClaim::Equipped, creatureID );
	}
	else if ( m_claim == ItemClaim::Equipped )
	{
		setClaim( ItemClaim::None, 0 );
	}
}

unsigned char Item::stackSize() const
{
	return DB::select( "StackSize", "Items", DBH::itemSID( m_itemUID ) ).toUInt();
}


void Item::addComponent( ItemMaterial im )
{
	if ( m_extraData == nullptr )
	{
		m_extraData = new ItemExtraData;
	}
	m_extraData->components.push_back( im );
}

QList<ItemMaterial> Item::components() const
{
	if ( m_extraData )
	{
		return m_extraData->components;
	}
	return {};
}

unsigned short Item::value() const
{
	return m_value * DBH::qualityMod(m_quality);
}

unsigned int Item::madeBy() const
{
	return m_madeBy;
}

void Item::setValue( unsigned short value )
{
	m_value = value;
}

void Item::setMadeBy( unsigned int creatureID )
{
	m_madeBy = creatureID;
}

unsigned char Item::quality() const
{
	return m_quality;
}

void Item::setQuality( unsigned char quality )
{
	m_quality = quality;
}

unsigned char Item::nutritionalValue() const
{
	if ( m_extraData )
	{
		return m_extraData->nutritionalValue;
	}
	return 0;
}

void Item::setNutritionalValue( unsigned char value )
{
	if ( value != 0 )
	{
		if ( m_extraData == nullptr )
		{
			m_extraData = new ItemExtraData;
		}
		m_extraData->nutritionalValue = value;
	}
}

unsigned char Item::drinkValue() const
{
	if ( m_extraData )
	{
		return m_extraData->drinkValue;
	}
	return 0;
}

void Item::setDrinkValue( unsigned char value )
{
	if ( value != 0 )
	{
		if ( m_extraData == nullptr )
		{
			m_extraData = new ItemExtraData;
		}
		m_extraData->drinkValue = value;
	}
}

unsigned int Item::color() const
{
	if ( m_extraData )
	{
		return m_extraData->color;
	}
	return 0;
}

void Item::setColor( QString color )
{
	//TODO add alpha
	if ( !color.isEmpty() )
	{
		if ( m_extraData == nullptr )
		{
			m_extraData = new ItemExtraData;
		}
		m_extraData->color = Global::util->string2Color( color );
	}
}

int Item::attackValue() const
{
	return DB::select( "AttackValue", "Items", DBH::itemSID( m_itemUID ) ).toInt();
}

bool Item::isWeapon() const
{
	return DB::select( "AttackValue", "Items", DBH::itemSID( m_itemUID ) ).toInt() > 0;
}

bool Item::isTool() const
{
	return DB::select( "IsTool", "Items", DBH::itemSID( m_itemUID ) ).toBool();
}

bool Item::isFree() const
{
	return m_location == ItemLocation::Ground && m_claim == ItemClaim::None;
}

void Item::setLocation( ItemLocation loc, unsigned int owner )
{
	m_location = loc;
	m_locationOwner = owner;
}

void Item::setClaim( ItemClaim cl, unsigned int owner )
{
	m_claim = cl;
	m_claimOwner = owner;
}