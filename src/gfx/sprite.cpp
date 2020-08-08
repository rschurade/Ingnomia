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
#include "../gfx/sprite.h"

#include "../base/db.h"

#include <QDebug>
#include <QPainter>

Sprite::Sprite()
{
}

Sprite::Sprite( const Sprite& other )
{
	uID           = other.uID;
	sID           = other.sID;
	xOffset       = other.xOffset;
	yOffset       = other.yOffset;
	opacity       = other.opacity;
	randomNumbers = other.randomNumbers;
}

Sprite::~Sprite()
{
}

SpritePixmap::SpritePixmap( QPixmap pixmap ) :
	Sprite()
{
	m_pixmap = pixmap;
	m_type   = "pixmap";
}

SpritePixmap::SpritePixmap( QPixmap pixmap, QString offset ) :
	Sprite()
{
	m_type          = "pixmap";
	int xOffset     = 0;
	int yOffset     = 0;
	QStringList osl = offset.split( " " );
	if ( osl.size() == 2 )
	{
		xOffset = osl[0].toInt();
		yOffset = osl[1].toInt();
	}
	QImage img = pixmap.toImage();
	QImage target( 32, 64, QImage::Format::Format_RGBA8888 );

	target.fill( QColor( 0, 0, 0, 0 ) );

	for ( int y = 0; y < img.height(); ++y )
	{
		for ( int x = 0; x < img.width(); ++x )
		{
			if ( y > 63 )
				qDebug() << "SpritePixmap::SpritePixmap" << sID;
			target.setPixelColor( qMin( 31, x + xOffset ), qMax( 0, qMin( 63, y + yOffset + 16 ) ), img.pixelColor( x, y ) );
		}
	}

	m_pixmap = QPixmap::fromImage( target );
}

SpritePixmap::SpritePixmap( const SpritePixmap& other ) :
	Sprite( other )
{
	m_type   = "pixmap";
	m_pixmap = other.m_pixmap;
}

SpritePixmap::~SpritePixmap()
{
}

QPixmap& SpritePixmap::pixmap( QString season, unsigned char rotation, unsigned char animationStep )
{
	return m_pixmap;
}

void SpritePixmap::setPixmap( QPixmap pm, QString season, unsigned char rotation )
{
	m_pixmap = pm;
}

void SpritePixmap::applyEffect( QString effect )
{
	if ( effect == "FlipHorizontal" )
	{
		QImage img = m_pixmap.toImage();
		m_pixmap   = QPixmap::fromImage( img.mirrored( true, false ) );
	}
	else if ( effect == "Rot90" )
	{
		QImage img = m_pixmap.toImage();
		QImage tmp( 32, 32, QImage::Format::Format_RGBA8888 );
		for ( int y = 0; y < 32; ++y )
		{
			for ( int x = 0; x < 32; ++x )
			{
				if ( y + 16 > 63 )
					qDebug() << "SpritePixmap::applyEffect 1" << sID;
				tmp.setPixelColor( x, y, img.pixelColor( x, y + 16 ) );
			}
		}
		QPixmap pm = QPixmap::fromImage( tmp );
		pm         = pm.transformed( QTransform().rotate( 90 ) );
		tmp        = pm.toImage();
		for ( int y = 0; y < 32; ++y )
		{
			for ( int x = 0; x < 32; ++x )
			{
				if ( y > 63 )
					qDebug() << "SpritePixmap::applyEffect 2" << sID;
				img.setPixelColor( x, y + 16, tmp.pixelColor( x, y ) );
			}
		}
		m_pixmap = QPixmap::fromImage( img );
	}
}

void SpritePixmap::applyTint( QString tint, QString materialSID )
{
	if ( tint.isEmpty() )
		return;

	QColor color;

	if ( tint == "Material" )
	{
		tint = DB::select( "Color", "Materials", materialSID ).toString();
	}
	QList<QString> csl = tint.split( ' ' );
	if ( csl.size() == 4 )
	{
		color = QColor( csl[0].toInt(), csl[1].toInt(), csl[2].toInt(), csl[3].toInt() );
	}
	else
	{
		//qDebug() << "no tint" << materialSID << tint;
		color = QColor( 255, 255, 255, 255 );
	}
	opacity = color.alphaF();

	QImage img = m_pixmap.toImage();

	for ( int x = 0; x < img.size().width(); ++x )
	{
		for ( int y = 0; y < img.size().height(); ++y )
		{
			if ( y > 63 )
				qDebug() << "SpritePixmap::applyTint" << sID;
			QColor col = img.pixelColor( x, y );
			col.setRedF( qMin( 1., col.redF() * color.redF() ) );
			col.setGreenF( qMin( 1., col.greenF() * color.greenF() ) );
			col.setBlueF( qMin( 1., col.blueF() * color.blueF() ) );
			col.setAlphaF( qMin( 1., col.alphaF() * color.alphaF() ) );
			img.setPixelColor( x, y, col );
		}
	}
	m_pixmap = QPixmap::fromImage( img );
}

void SpritePixmap::combine( Sprite* other, QString season, unsigned char rotation, unsigned char animationStep )
{
	if ( m_pixmap.size().width() > 0 && m_pixmap.size().height() > 0 )
	{
		QPainter painter( &m_pixmap );
		painter.drawPixmap( 0, 0, other->pixmap( season, rotation, animationStep ) );
	}
}

SpriteSeasons::SpriteSeasons() :
	Sprite()
{
	m_type = "seasons";
}

SpriteSeasons::SpriteSeasons( const SpriteSeasons& other ) :
	Sprite( other )
{
	m_type    = "seasons";
	m_sprites = other.m_sprites;
}

SpriteSeasons::~SpriteSeasons()
{
	for ( auto s : m_sprites )
	{
		delete s;
	}
}

QPixmap& SpriteSeasons::pixmap( QString season, unsigned char rotation, unsigned char animationStep )
{
	return m_sprites[season]->pixmap( season, rotation, animationStep );
}

void SpriteSeasons::setPixmap( QPixmap pm, QString season, unsigned char rotation )
{
	m_sprites[season]->setPixmap( pm, season, rotation );
}

void SpriteSeasons::applyEffect( QString effect )
{
	for ( auto s : m_sprites )
	{
		s->applyEffect( effect );
	}
}

void SpriteSeasons::applyTint( QString tint, QString materialSID )
{
	for ( auto s : m_sprites )
	{
		s->applyTint( tint, materialSID );
	}
}

void SpriteSeasons::combine( Sprite* other, QString season, unsigned char rotation, unsigned char animationStep )
{
	m_sprites[season]->combine( other, season, rotation, animationStep );
}

SpriteRotations::SpriteRotations() :
	Sprite()
{
	m_type = "rotations";
}

SpriteRotations::~SpriteRotations()
{
	for ( auto s : m_sprites )
	{
		delete s;
	}
}

SpriteRotations::SpriteRotations( const SpriteRotations& other ) :
	Sprite( other )
{
	m_type    = "rotations";
	m_sprites = other.m_sprites;
}

QPixmap& SpriteRotations::pixmap( QString season, unsigned char rotation, unsigned char animationStep )
{
	return m_sprites[rotation]->pixmap( season, rotation, animationStep );
}

void SpriteRotations::setPixmap( QPixmap pm, QString season, unsigned char rotation )
{
	m_sprites[rotation]->setPixmap( pm, season, rotation );
}

void SpriteRotations::applyEffect( QString effect )
{
	for ( auto s : m_sprites )
	{
		s->applyEffect( effect );
	}
}

void SpriteRotations::applyTint( QString tint, QString materialSID )
{
	for ( auto s : m_sprites )
	{
		s->applyTint( tint, materialSID );
	}
}

void SpriteRotations::combine( Sprite* other, QString season, unsigned char rotation, unsigned char animationStep )
{
	m_sprites[rotation]->combine( other, season, rotation, animationStep );
}

SpriteFrames::SpriteFrames() :
	Sprite()
{
	m_type = "frames";
}

SpriteFrames::SpriteFrames( const SpriteFrames& other ) :
	Sprite( other )
{
	m_type    = "frames";
	m_sprites = other.m_sprites;
}

SpriteFrames::~SpriteFrames()
{
	for ( auto s : m_sprites )
	{
		delete s;
	}
}

QPixmap& SpriteFrames::pixmap( QString season, unsigned char rotation, unsigned char animationStep )
{
	return m_sprites[animationStep % m_sprites.size()]->pixmap( season, rotation, animationStep );
}

void SpriteFrames::setPixmap( QPixmap pm, QString season, unsigned char rotation )
{
	m_sprites[0]->setPixmap( pm, season, rotation );
}

void SpriteFrames::applyEffect( QString effect )
{
	for ( auto s : m_sprites )
	{
		s->applyEffect( effect );
	}
}

void SpriteFrames::applyTint( QString tint, QString materialSID )
{
	for ( auto s : m_sprites )
	{
		s->applyTint( tint, materialSID );
	}
}

void SpriteFrames::combine( Sprite* other, QString season, unsigned char rotation, unsigned char animationStep )
{
	if ( m_sprites.size() > animationStep )
	{
		m_sprites[animationStep]->combine( other, season, rotation, animationStep );
	}
}
