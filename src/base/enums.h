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

#include <QObject>

enum Connection : unsigned int
{
	noCon     = 0,
	north     = 0x01,
	northeast = 0x02,
	east      = 0x04,
	southeast = 0x08,
	south     = 0x10,
	southwest = 0x20,
	west      = 0x40,
	northwest = 0x80,
	up        = 0x100,
	down      = 0x200,
	upnorth   = 0x400,
	upeast    = 0x800,
	upsouth   = 0x1000,
	upwest    = 0x2000,
	downnorth = 0x4000,
	downeast  = 0x8000,
	downsouth = 0x10000,
	downwest  = 0x20000
};


enum class ScheduleActivity : unsigned char {
	None,
	Eat,
	Sleep,
	Training
};
Q_DECLARE_METATYPE( ScheduleActivity )

enum DBSkillIds : int
{
	SK_Mining,
	SK_Masonry,
	SK_Stonecarving,
	SK_Woodcutting,
	SK_Carpentry,
	SK_Woodcarving,
	SK_Smelting,
	SK_Blacksmithing,
	SK_Metalworking,
	SK_WeaponCrafting,
	SK_ArmorCrafting,
	SK_Gemcutting,
	SK_JewelryMaking,
	SK_Weaving,
	SK_Tailoring,
	SK_Dyeing,
	SK_Pottery,
	SK_Leatherworking,
	SK_Bonecarving,
	SK_Prospecting,
	SK_Tinkering,
	SK_Machining,
	SK_Engineering,
	SK_Mechanic,
	SK_AnimalHusbandry,
	SK_Butchery,
	SK_Fishing,
	SK_Horticulture,
	SK_Farming,
	SK_Cooking,
	SK_Brewing,
	SK_Construction,
	SK_Hauling,
	SK_Unarmed,
	SK_Melee,
	SK_Ranged,
	SK_Thrown,
	SK_Dodge,
	SK_Block,
	SK_Armor,
	SK_Crossbow,
	SK_Gun,
	SK_Medic,
	SK_Caretaking,
	SK_MagicNature,
	SK_MagicGeomancy
};

enum CreaturePart : unsigned char
{
	KCP_NONE,
	CP_HEAD,
	CP_TORSO,
	CP_LEFT_ARM,
	CP_RIGHT_ARM,
	CP_LEFT_HAND,
	CP_RIGHT_HAND,

	CP_LEFT_FRONT_LEG,
	CP_RIGHT_FRONT_LEG,
	CP_LEFT_FRONT_FOOT,
	CP_RIGHT_FRONT_FOOT,

	CP_LEFT_WING,
	CP_RIGHT_WING,

	CP_LEFT_LEG,
	CP_RIGHT_LEG,
	CP_LEFT_FOOT,
	CP_RIGHT_FOOT,

	CP_BRAIN,
	CP_LEFT_EYE,
	CP_RIGHT_EYE,
	CP_HEART,
	CP_LEFT_LUNG,
	CP_RIGHT_LUNG,

	CP_HAIR,
	CP_FACIAL_HAIR,
	CP_CLOTHING,
	CP_BOOTS,
	CP_HAT,
	CP_BACK,

	CP_ARMOR_HEAD,
	CP_ARMOR_TORSO,
	CP_ARMOR_ARM,
	CP_ARMOR_HAND,
	CP_ARMOR_LEG,
	CP_ARMOR_FOOT,

	CP_ARMOR_LEFT_ARM,
	CP_ARMOR_RIGHT_ARM,
	CP_ARMOR_LEFT_HAND,
	CP_ARMOR_RIGHT_HAND,
	CP_ARMOR_LEFT_FOOT,
	CP_ARMOR_RIGHT_FOOT,
	CP_LEFT_HAND_HELD,
	CP_RIGHT_HAND_HELD
};

enum DamageType : unsigned char
{
	DT_BLUNT,
	DT_SLASH,
	DT_PIERCING
};

enum AnatomyHeight : unsigned char
{
	AH_LOW,
	AH_MIDDLE,
	AH_HIGH
};

enum AnatomySide : unsigned char
{
	AS_CENTER = 0x00,
	AS_FRONT  = 0x01,
	AS_BACK   = 0x02,
	AS_LEFT   = 0x04,
	AS_RIGHT  = 0x08
};

enum AnatomyStatus : unsigned int
{
	AS_HEALTHY     = 0,
	AS_WOUNDED     = 0x01,
	AS_UNCONSCIOUS = 0x02,
	AS_DEAD        = 0x04,
	AS_HALFBLIND   = 0x08,
	AS_BLIND       = 0x10,

};

enum class GameSpeed
{
	Normal,
	Fast
};
Q_DECLARE_METATYPE( GameSpeed )

enum class ButtonSelection
{
	None,
	Build,
	Mine,
	Agriculture,
	Designation,
	Job,
	Magic
};

enum class BuildSelection
{
	None,
	Workshop,
	Wall,
	Floor,
	Stairs,
	Ramps,
	Containers,
	Fence,
	Furniture,
	Utility
};
Q_DECLARE_METATYPE( BuildSelection )

enum class ShownInfo
{
	None,
	TileInfo,
	Stockpile,
	Workshop,
	Agriculture,
	Population,
	CreatureInfo,
	Debug,
	Neighbors,
	Military,
	Inventory
};

enum class BuildItemType
{
	Workshop,
	Item,
	Terrain
};
Q_DECLARE_METATYPE( BuildItemType )