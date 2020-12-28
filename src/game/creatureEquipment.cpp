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
#include "creature.h"

QVariantMap EquipmentItem::serialize()
{
	QVariantMap out;
	out.insert( "Item", item );
	out.insert( "Material", material );
	out.insert( "ItemID", itemID );
	out.insert( "MaterialID", materialID );
	out.insert( "AllMats", allMats );
	return out;
}

EquipmentItem::EquipmentItem( const QVariantMap& in )
{
	item       = in.value( "Item" ).toString();
	material   = in.value( "Material" ).toString();
	itemID     = in.value( "ItemID" ).toUInt();
	materialID = in.value( "MaterialID" ).toUInt();
	allMats	   = in.value( "AllMats" ).toStringList();
}

QList<unsigned int> Equipment::wornItems() const
{
	QList<unsigned int> items;
	EquipmentItem const *const equipmentSlots[] = {
		&head,
		&chest,
		&arm,
		&hand,
		&leg,
		&foot,
		&leftHandHeld,
		&rightHandHeld,
		&back,
	};
	for ( const auto& slot : equipmentSlots )
	{
		if (slot->itemID)
		{
			items.append( slot->itemID );
		}
	}
	return items;
}

QVariantMap Equipment::serialize()
{
	QVariantMap vEqui;

	vEqui.insert( "Hair", hair );
	vEqui.insert( "FacialHair", facialHair );
	vEqui.insert( "HairColor", hairColor );
	vEqui.insert( "Shirt", shirt );
	vEqui.insert( "ShirtColor", shirtColor );

	vEqui.insert( "UniformID", uniformID );
	vEqui.insert( "RoomID", roomID );

	QVariantMap items;
	items.insert( "HeadArmor", head.serialize() );
	items.insert( "ChestArmor", chest.serialize() );
	items.insert( "ArmArmor", arm.serialize() );
	items.insert( "HandArmor", hand.serialize() );
	items.insert( "LegArmor", leg.serialize() );
	items.insert( "FootArmor", foot.serialize() );
	items.insert( "LeftHandHeld", leftHandHeld.serialize() );
	items.insert( "RightHandHeld", rightHandHeld.serialize() );
	items.insert( "Back", back.serialize() );
	vEqui.insert( "Items", items );

	return vEqui;
}

Equipment::Equipment( const QVariantMap& in )
{
	hair       = in.value( "Hair" ).toString();
	facialHair = in.value( "FacialHair" ).toString();
	hairColor  = in.value( "HairColor" ).toInt();
	shirt      = in.value( "Shirt" ).toString();
	shirtColor = in.value( "ShirtColor" ).toInt();

	uniformID = in.value( "UniformID" ).toUInt();
	roomID    = in.value( "RoomID" ).toUInt();

	auto vmItems  = in.value( "Items" ).toMap();
	head          = EquipmentItem( vmItems.value( "HeadArmor" ).toMap() );
	chest         = EquipmentItem( vmItems.value( "ChestArmor" ).toMap() );
	arm           = EquipmentItem( vmItems.value( "ArmArmor" ).toMap() );
	hand          = EquipmentItem( vmItems.value( "HandArmor" ).toMap() );
	leg           = EquipmentItem( vmItems.value( "LegArmor" ).toMap() );
	foot          = EquipmentItem( vmItems.value( "FootArmor" ).toMap() );
	leftHandHeld  = EquipmentItem( vmItems.value( "LeftHandHeld" ).toMap() );
	rightHandHeld = EquipmentItem( vmItems.value( "RightHandHeld" ).toMap() );
	back          = EquipmentItem( vmItems.value( "Back" ).toMap() );
}

void Equipment::clearAllItems()
{
	head          = EquipmentItem();
	chest         = EquipmentItem();
	arm           = EquipmentItem();
	hand          = EquipmentItem();
	leg           = EquipmentItem();
	foot          = EquipmentItem();
	leftHandHeld  = EquipmentItem();
	rightHandHeld = EquipmentItem();
	back          = EquipmentItem();
}

float Equipment::getDamageReduction( CreaturePart part )
{
	unsigned int itemID = 0;
	QString itemSID;
	float reduction = 0.0;
	switch ( part )
	{
		case CP_HEAD:
			itemID  = head.itemID;
			itemSID = head.item;
			break;
		case CP_TORSO:
			itemID  = chest.itemID;
			itemSID = chest.item;
			break;
		case CP_LEFT_ARM:
		case CP_RIGHT_ARM:
			itemID  = arm.itemID;
			itemSID = arm.item;
			break;
		case CP_LEFT_HAND:
		case CP_RIGHT_HAND:
			itemID  = hand.itemID;
			itemSID = hand.item;
			break;
		case CP_LEFT_LEG:
		case CP_RIGHT_LEG:
			itemID  = leg.itemID;
			itemSID = leg.item;
			break;
		case CP_LEFT_FOOT:
		case CP_RIGHT_FOOT:
			itemID  = foot.itemID;
			itemSID = foot.item;
			break;
		default:
			break;
	}
	if ( itemID )
	{ // TODO put these values on a dtabase table
		if ( itemSID.startsWith( "Leather" ) )
		{
			reduction = 2.0;
		}
		else if ( itemSID.startsWith( "Bone" ) )
		{
			reduction = 3.0;
		}
		else if ( itemSID.startsWith( "Chain" ) )
		{
			reduction = 4.0;
		}
		else if ( itemSID.startsWith( "Plate" ) )
		{
			reduction = 6.0;
		}
		else if ( itemSID.startsWith( "Heavy" ) )
		{
			reduction = 8.0;
		}
	}

	return reduction;
}

EquipmentItem& Equipment::getSlot( CreaturePart part )
{
	switch ( part )
	{
		case CP_ARMOR_HEAD:
			return head;
		case CP_ARMOR_TORSO:
			return chest;
		case CP_ARMOR_ARM:
			return arm;
		case CP_ARMOR_HAND:
			return hand;
		case CP_ARMOR_LEG:
			return leg;
		case CP_ARMOR_FOOT:
			return foot;
		case CP_LEFT_HAND_HELD:
			return leftHandHeld;
		case CP_RIGHT_HAND_HELD:
			return rightHandHeld;
		case CP_BACK:
			return back;
		default:
			qWarning() << "Invalid equipment slot!";
			abort();
	}
}
