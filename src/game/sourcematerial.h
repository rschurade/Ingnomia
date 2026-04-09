/** @file sourcematerial.h
 *  @brief POD struct storing item specifications (itemSID, materialSID, quality) for
 *         items that are consumed/destroyed during construction.
 */
#pragma once

#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <QList>

/** @brief Stores the type, material, and quality of a source item used in construction.
 *
 *  Used to remember what materials went into a constructed object so that they
 *  can be recovered on deconstruction.
 */
struct SourceMaterial
{
	QString itemSID;
	QString materialSID;
	uint8_t quality = 0;

	SourceMaterial() = default;

	SourceMaterial( QString item, QString mat, uint8_t q ) :
		itemSID( std::move( item ) ),
		materialSID( std::move( mat ) ),
		quality( q )
	{
	}

	QVariantMap serialize() const
	{
		QVariantMap m;
		m.insert( "ItemSID", itemSID );
		m.insert( "MaterialSID", materialSID );
		m.insert( "Quality", quality );
		return m;
	}

	static SourceMaterial deserialize( const QVariantMap& m )
	{
		return SourceMaterial(
			m.value( "ItemSID" ).toString(),
			m.value( "MaterialSID" ).toString(),
			static_cast<uint8_t>( m.value( "Quality" ).toUInt() )
		);
	}

	static QVariantList serializeList( const QList<SourceMaterial>& list )
	{
		QVariantList out;
		for ( const auto& sm : list )
		{
			out.append( sm.serialize() );
		}
		return out;
	}

	static QList<SourceMaterial> deserializeList( const QVariantList& list )
	{
		QList<SourceMaterial> out;
		for ( const auto& v : list )
		{
			out.append( deserialize( v.toMap() ) );
		}
		return out;
	}
};
