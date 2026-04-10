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
/** @file sprite.cpp
 *  @brief Sprite class hierarchy: base Sprite, SpritePixmap (single image), SpriteSeasons
 *         (per-season variants), SpriteRotations (per-direction variants), and SpriteFrames
 *         (animated). Implements pixmap access, tinting, effects, and compositing.
 */
#include "../gfx/sprite.h"

#include "../base/db.h"

#include <QDebug>
#include <QPainter>

/// @brief Default constructor. Members left with default values.
Sprite::Sprite()
{
}

/// @brief Copy constructor. Copies identity, offsets, opacity, and random-number seed array.
/// @param other Sprite to copy from.
Sprite::Sprite( const Sprite& other )
{
	uID           = other.uID;
	sID           = other.sID;
	xOffset       = other.xOffset;
	yOffset       = other.yOffset;
	opacity       = other.opacity;
	randomNumbers = other.randomNumbers;
}

/// @brief Virtual destructor.
Sprite::~Sprite()
{
}

/// @brief Constructs a SpritePixmap wrapping the given pixmap without any offset adjustment.
/// @param pixmap The raw QPixmap to store.
SpritePixmap::SpritePixmap( QPixmap pixmap ) :
	Sprite()
{
	m_pixmap = pixmap;
	m_type   = "pixmap";
}

/// @brief Constructs a SpritePixmap by blitting @p pixmap into a 32×64 canvas with a
///        pixel offset parsed from @p offset (format: "x y"). The source is shifted down
///        by 16 px to sit on the lower tile half; coordinates are clamped to the canvas.
/// @param pixmap Source pixmap.
/// @param offset Space-separated "x y" offset string.
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

/// @brief Copy constructor.
/// @param other SpritePixmap to copy from.
SpritePixmap::SpritePixmap( const SpritePixmap& other ) :
	Sprite( other )
{
	m_type   = "pixmap";
	m_pixmap = other.m_pixmap;
}

/// @brief Destructor.
SpritePixmap::~SpritePixmap()
{
}

/// @brief Returns the stored pixmap. Season/rotation/animationStep are ignored for this leaf type.
/// @param season        Unused.
/// @param rotation      Unused.
/// @param animationStep Unused.
/// @return Reference to the stored QPixmap.
QPixmap& SpritePixmap::pixmap( QString season, unsigned char rotation, unsigned char animationStep )
{
	return m_pixmap;
}

/// @brief Replaces the stored pixmap. Season/rotation are ignored for this leaf type.
/// @param pm       New pixmap.
/// @param season   Unused.
/// @param rotation Unused.
void SpritePixmap::setPixmap( QPixmap pm, QString season, unsigned char rotation )
{
	m_pixmap = pm;
}

/// @brief Applies an in-place image transformation to the stored pixmap.
///        Supported: "FlipHorizontal" (mirror X), "Rot90" (rotate the lower 32×32 tile by 90°).
/// @param effect Effect name.
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

/// @brief Multiplies each pixel of the stored image by a colour tint. If @p tint is "Material",
///        looks up the colour from the Materials DB table using @p materialSID. Updates opacity
///        from the tint's alpha channel. Empty tint is a no-op.
/// @param tint        Tint colour as "R G B A" string, "Material", or empty.
/// @param materialSID Material key used when @p tint == "Material".
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

/// @brief Overlays @p other on top of this sprite's pixmap in place.
/// @param other         Sprite to draw on top.
/// @param season        Season variant to request from @p other.
/// @param rotation      Rotation variant to request from @p other.
/// @param animationStep Animation frame to request from @p other.
void SpritePixmap::combine( Sprite* other, QString season, unsigned char rotation, unsigned char animationStep )
{
	if ( m_pixmap.size().width() > 0 && m_pixmap.size().height() > 0 )
	{
		QPainter painter( &m_pixmap );
		painter.drawPixmap( 0, 0, other->pixmap( season, rotation, animationStep ) );
	}
}

/// @brief Default constructor. Type is set to "seasons".
SpriteSeasons::SpriteSeasons() :
	Sprite()
{
	m_type = "seasons";
}

/// @brief Copy constructor. Shallow-copies the per-season sprite map (sub-sprites are shared).
/// @param other SpriteSeasons to copy from.
SpriteSeasons::SpriteSeasons( const SpriteSeasons& other ) :
	Sprite( other )
{
	m_type    = "seasons";
	m_sprites = other.m_sprites;
}

/// @brief Destructor. Deletes all sub-sprites.
SpriteSeasons::~SpriteSeasons()
{
	for ( auto s : m_sprites )
	{
		delete s;
	}
}

/// @brief Returns the pixmap for the requested season, delegating to the matching sub-sprite.
/// @param season        Season key (e.g. "Spring").
/// @param rotation      Rotation index forwarded to the sub-sprite.
/// @param animationStep Animation frame forwarded to the sub-sprite.
/// @return Reference to the season-specific QPixmap.
QPixmap& SpriteSeasons::pixmap( QString season, unsigned char rotation, unsigned char animationStep )
{
	return m_sprites[season]->pixmap( season, rotation, animationStep );
}

/// @brief Sets the pixmap for the requested season's sub-sprite.
/// @param pm       New pixmap.
/// @param season   Season key to update.
/// @param rotation Rotation index forwarded to the sub-sprite.
void SpriteSeasons::setPixmap( QPixmap pm, QString season, unsigned char rotation )
{
	m_sprites[season]->setPixmap( pm, season, rotation );
}

/// @brief Applies the given effect to every per-season sub-sprite.
/// @param effect Effect name (e.g. "FlipHorizontal", "Rot90").
void SpriteSeasons::applyEffect( QString effect )
{
	for ( auto s : m_sprites )
	{
		s->applyEffect( effect );
	}
}

/// @brief Applies a material tint to every per-season sub-sprite.
/// @param tint        Tint specification (RGBA string, "Material", or empty).
/// @param materialSID Material key used when @p tint == "Material".
void SpriteSeasons::applyTint( QString tint, QString materialSID )
{
	for ( auto s : m_sprites )
	{
		s->applyTint( tint, materialSID );
	}
}

/// @brief Composites @p other onto the season-specific sub-sprite.
/// @param other         Sprite to overlay.
/// @param season        Season key selecting the target sub-sprite.
/// @param rotation      Rotation index forwarded to @p other.
/// @param animationStep Animation frame forwarded to @p other.
void SpriteSeasons::combine( Sprite* other, QString season, unsigned char rotation, unsigned char animationStep )
{
	m_sprites[season]->combine( other, season, rotation, animationStep );
}

/// @brief Default constructor. Type is set to "rotations".
SpriteRotations::SpriteRotations() :
	Sprite()
{
	m_type = "rotations";
}

/// @brief Destructor. Deletes all sub-sprites.
SpriteRotations::~SpriteRotations()
{
	for ( auto s : m_sprites )
	{
		delete s;
	}
}

/// @brief Copy constructor. Shallow-copies the per-rotation sprite vector.
/// @param other SpriteRotations to copy from.
SpriteRotations::SpriteRotations( const SpriteRotations& other ) :
	Sprite( other )
{
	m_type    = "rotations";
	m_sprites = other.m_sprites;
}

/// @brief Returns the pixmap for the requested rotation index.
/// @param season        Season forwarded to the sub-sprite.
/// @param rotation      Rotation index (0–3) selecting the sub-sprite.
/// @param animationStep Animation frame forwarded to the sub-sprite.
/// @return Reference to the rotation-specific QPixmap.
QPixmap& SpriteRotations::pixmap( QString season, unsigned char rotation, unsigned char animationStep )
{
	return m_sprites[rotation]->pixmap( season, rotation, animationStep );
}

/// @brief Sets the pixmap for the rotation-specific sub-sprite.
/// @param pm       New pixmap.
/// @param season   Season forwarded to the sub-sprite.
/// @param rotation Rotation index to update.
void SpriteRotations::setPixmap( QPixmap pm, QString season, unsigned char rotation )
{
	m_sprites[rotation]->setPixmap( pm, season, rotation );
}

/// @brief Applies the given effect to every per-rotation sub-sprite.
/// @param effect Effect name.
void SpriteRotations::applyEffect( QString effect )
{
	for ( auto s : m_sprites )
	{
		s->applyEffect( effect );
	}
}

/// @brief Applies a material tint to every per-rotation sub-sprite.
/// @param tint        Tint specification.
/// @param materialSID Material key used when @p tint == "Material".
void SpriteRotations::applyTint( QString tint, QString materialSID )
{
	for ( auto s : m_sprites )
	{
		s->applyTint( tint, materialSID );
	}
}

/// @brief Composites @p other onto the rotation-specific sub-sprite.
/// @param other         Sprite to overlay.
/// @param season        Season forwarded to @p other.
/// @param rotation      Rotation index selecting the target sub-sprite.
/// @param animationStep Animation frame forwarded to @p other.
void SpriteRotations::combine( Sprite* other, QString season, unsigned char rotation, unsigned char animationStep )
{
	m_sprites[rotation]->combine( other, season, rotation, animationStep );
}

/// @brief Default constructor. Type is set to "frames".
SpriteFrames::SpriteFrames() :
	Sprite()
{
	m_type = "frames";
}

/// @brief Copy constructor. Shallow-copies the per-frame sprite vector.
/// @param other SpriteFrames to copy from.
SpriteFrames::SpriteFrames( const SpriteFrames& other ) :
	Sprite( other )
{
	m_type    = "frames";
	m_sprites = other.m_sprites;
}

/// @brief Destructor. Deletes all per-frame sub-sprites.
SpriteFrames::~SpriteFrames()
{
	for ( auto s : m_sprites )
	{
		delete s;
	}
}

/// @brief Returns the pixmap for the current animation frame (wraps modulo frame count).
/// @param season        Season forwarded to the sub-sprite.
/// @param rotation      Rotation index forwarded to the sub-sprite.
/// @param animationStep Animation step; modulo the number of frames selects the active frame.
/// @return Reference to the frame-specific QPixmap.
QPixmap& SpriteFrames::pixmap( QString season, unsigned char rotation, unsigned char animationStep )
{
	return m_sprites[animationStep % m_sprites.size()]->pixmap( season, rotation, animationStep );
}

/// @brief Sets the pixmap for the first animation frame only.
/// @param pm       New pixmap.
/// @param season   Season forwarded to the sub-sprite.
/// @param rotation Rotation index forwarded to the sub-sprite.
void SpriteFrames::setPixmap( QPixmap pm, QString season, unsigned char rotation )
{
	m_sprites[0]->setPixmap( pm, season, rotation );
}

/// @brief Applies the given effect to every animation frame.
/// @param effect Effect name.
void SpriteFrames::applyEffect( QString effect )
{
	for ( auto s : m_sprites )
	{
		s->applyEffect( effect );
	}
}

/// @brief Applies a material tint to every animation frame.
/// @param tint        Tint specification.
/// @param materialSID Material key used when @p tint == "Material".
void SpriteFrames::applyTint( QString tint, QString materialSID )
{
	for ( auto s : m_sprites )
	{
		s->applyTint( tint, materialSID );
	}
}

/// @brief Composites @p other onto a single animation frame (if @p animationStep is in range).
/// @param other         Sprite to overlay.
/// @param season        Season forwarded to @p other.
/// @param rotation      Rotation index forwarded to @p other.
/// @param animationStep Frame index to composite onto.
void SpriteFrames::combine( Sprite* other, QString season, unsigned char rotation, unsigned char animationStep )
{
	if ( m_sprites.size() > animationStep )
	{
		m_sprites[animationStep]->combine( other, season, rotation, animationStep );
	}
}
