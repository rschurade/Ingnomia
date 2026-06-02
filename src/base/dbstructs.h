/** @file dbstructs.h
 *  @brief Plain data structs mirroring database table schemas.
 *
 *  The DBS namespace contains C++ structs that correspond to rows in the
 *  game's SQLite database tables. Each struct's members map directly to
 *  database columns. These are used by the DB class to cache parsed
 *  table data in memory for fast access during gameplay.
 */

#include "../base/position.h"

#include <QString>



namespace DBS
{

/** @brief Row from the Actions table defining buildable/placeable actions. */
struct Actions {
	QString ID;                ///< Unique action identifier
	QString Job;               ///< Job ID triggered by this action
	QString ConstructionType;  ///< Type of construction this action creates
	bool ConstructionSelect;   ///< Whether the player must select a construction variant
	bool IsFloor;              ///< Whether this action places a floor tile
	bool Multi;                ///< Whether multiple tiles can be placed at once
	bool MultiZ;               ///< Whether placement spans multiple Z-levels
	bool Rotate;               ///< Whether the placed object can be rotated
};

/** @brief Row from the Actions_Tiles table defining tile offsets and sprite data for actions. */
struct Actions_Tiles {
	QString ID;        ///< Parent action ID
	QString Offset;    ///< Tile offset from the action origin (e.g. "0 0 0")
	QString SpriteID;  ///< Sprite to display at this tile
	bool IsFloor;      ///< Whether this tile entry is a floor
	QString Required;  ///< Required adjacent tile conditions
	QString Forbidden; ///< Forbidden adjacent tile conditions
};

/** @brief Row from the AI table mapping creature types to behavior trees. */
struct AI {
	QString ID;           ///< Creature type ID
	QString BehaviorTree; ///< Behavior tree ID used by this creature type
};

/** @brief Row from the Anatomy table defining a creature's body structure. */
struct Anatomy {
	QString ID;         ///< Anatomy type ID
	int Blood;          ///< Total blood volume
	QString Root;       ///< Root body part ID
	QString LayerOrder; ///< Order of body layers for rendering/hit detection
};

/** @brief Row from the Anatomy_Parts table defining individual body parts. */
struct Anatomy_Parts {
	QString ID;      ///< Anatomy type ID (parent)
	QString ID2;     ///< Body part ID
	QString Parent;  ///< Parent body part ID in the hierarchy
	bool IsInside;   ///< Whether this part is internal
	QString Height;  ///< Height zone of the body part (e.g. "Upper", "Lower")
	QString Side;    ///< Side of the body (e.g. "Left", "Right", "Center")
	QString Facing;  ///< Facing direction of the part
	int HP;          ///< Hit points of the body part
	bool IsVital;    ///< Whether destruction of this part is fatal
};

/** @brief Row from the Animals table defining animal species properties. */
struct Animals {
	QString ID;            ///< Animal species ID
	bool AllowInWild;      ///< Whether this animal spawns in the wild
	bool Aquatic;          ///< Whether this animal is aquatic
	QString BehaviorTree;  ///< Behavior tree ID for this species
	QString Biome;         ///< Biome(s) where this animal can appear
	bool Embark;           ///< Whether this animal is available at embark
	QString Food;          ///< Food type this animal eats
	int GestationDays;     ///< Number of days for pregnancy
	bool Pasture;          ///< Whether this animal can be pastured
	int PastureSize;       ///< Required pasture tiles per animal
	QString Prey;          ///< Prey species this animal hunts
	bool IsMulti;          ///< Whether this animal occupies multiple tiles
};

/** @brief Row from the Animals_OnButcher table defining butchery products. */
struct Animals_OnButcher {
	QString ID;      ///< Animal species ID (parent)
	int Amount;      ///< Quantity of the product
	QString ItemID;  ///< Item ID of the butchery product
	QString Type;    ///< Product type category
};

/** @brief Row from the Animals_States table defining animal life stages. */
struct Animals_States {
	QString ID;            ///< Animal species ID (parent)
	QString ID2;           ///< State/life stage ID
	QString SpriteID;      ///< Sprite for this state
	int DaysToNextState;   ///< Days until transitioning to the next state
	bool Immobile;         ///< Whether the animal is immobile in this state
	QString BehaviorTree;  ///< Behavior tree override for this state
	bool IsAggro;          ///< Whether the animal is aggressive in this state
	int Attack;            ///< Attack skill value
	int Damage;            ///< Damage dealt per attack
	QString Anatomy;       ///< Anatomy type used in this state
};

/** @brief Row from the Animals_States_Behavior table defining state-specific behaviors. */
struct Animals_States_Behavior {
	QString ID;              ///< Animal species ID (parent)
	QString ID2;             ///< State ID (parent)
	QString RequiredGender;  ///< Gender requirement for this behavior (e.g. "Female")
	int Amount;              ///< Quantity associated with the behavior
	QString CreatureID;      ///< Creature produced (e.g. offspring species)
	int DaysBetween;         ///< Minimum days between behavior occurrences
	int EatTime;             ///< Time in ticks to eat
	QString EggID;           ///< Item ID of eggs laid
	int FoodValue;           ///< Nutritional value gained from eating
	float HungerPerTick;     ///< Hunger increase per game tick
	QString ItemID;          ///< Item produced by the behavior
	float Speed;             ///< Movement speed in this state
};

/** @brief Row from the Attributes table listing character attributes. */
struct Attributes {
	QString ID; ///< Attribute name ID (e.g. "Strength", "Dexterity")
};

/** @brief Row from the Automaton_Cores table defining automaton core types. */
struct Automaton_Cores {
	QString ID;           ///< Core type ID
	QString BehaviorTree; ///< Behavior tree used by automatons with this core
};

/** @brief Row from the Automaton_Cores_Skills table defining skills granted by cores. */
struct Automaton_Cores_Skills {
	QString ID;       ///< Core type ID (parent)
	QString SkillID;  ///< Skill granted by this core
	int SkillValue;   ///< Skill level value
};

/** @brief Row from the BaseItems table listing base item types. */
struct BaseItems {
	QString ID; ///< Base item type ID
};

/** @brief Row from the BaseSprites table defining individual sprite source rectangles. */
struct BaseSprites {
	QString ID;              ///< Base sprite ID
	QString SourceRectangle; ///< Rectangle in the tilesheet (e.g. "x y w h")
	QString Tilesheet;       ///< Tilesheet filename containing this sprite
};

/** @brief Row from the Constructions table defining buildable constructions. */
struct Constructions {
	QString ID;            ///< Construction ID
	bool oConstruction;    ///< Whether this is an "o"-type construction
	bool Rotation;         ///< Whether the construction supports rotation
	QString Type;          ///< Construction type (e.g. "Wall", "Floor")
	QString Category;      ///< UI category for grouping
};

/** @brief Row from the Constructions_Components table defining required materials. */
struct Constructions_Components {
	QString ID;            ///< Construction ID (parent)
	int Amount;            ///< Quantity of the component required
	QString ItemID;        ///< Item ID of the required component
	QString MaterialTypes; ///< Allowed material types (pipe-separated)
	QString Materials;     ///< Specific allowed materials (pipe-separated)
};

/** @brief Row from the Constructions_IntermediateSprites table for in-progress build sprites. */
struct Constructions_IntermediateSprites {
	QString ID;       ///< Construction ID (parent)
	QString Offset;   ///< Tile offset from the construction origin
	float Percent;    ///< Completion percentage at which this sprite appears
	QString SpriteID; ///< Sprite to display
	QString Type;     ///< Sprite type category
};

/** @brief Row from the Constructions_Sprites table defining completed construction sprites. */
struct Constructions_Sprites {
	QString ID;               ///< Construction ID (parent)
	QString Offset;           ///< Tile offset from the construction origin
	QString SpriteID;         ///< Sprite to display
	QString SpriteIDOverride; ///< Override sprite for special conditions
	QString Type;             ///< Sprite type category (e.g. "Wall", "Floor")
};

/** @brief Row from the ConstructionTypes table listing construction type categories. */
struct ConstructionTypes {
	QString ID; ///< Construction type ID
};

/** @brief Row from the Containers table defining container properties. */
struct Containers {
	QString ID;       ///< Container ID
	bool Buildable;   ///< Whether the container can be built by gnomes
	int Capacity;     ///< Maximum number of items the container holds
	bool RequireSame; ///< Whether all items must be the same type
	QString Type;     ///< Container type category
	QString Item;     ///< Item ID used to build this container
};

/** @brief Row from the Containers_Components table defining container build materials. */
struct Containers_Components {
	QString ID;     ///< Container ID (parent)
	QString ItemID; ///< Required component item ID
};

/** @brief Row from the Containers_Tiles table defining container tile layout. */
struct Containers_Tiles {
	QString ID;       ///< Container ID (parent)
	bool Construction;///< Whether this tile is a construction tile
	bool Job;         ///< Whether this tile is a job interaction tile
	QString Offset;   ///< Tile offset from the container origin
	QString SpriteID; ///< Sprite to display at this tile
	bool Stockpile;   ///< Whether this tile acts as a stockpile slot
};

/** @brief Row from the Crafts table defining craftable items. */
struct Crafts {
	QString ID;                 ///< Craft recipe ID
	int Amount;                 ///< Number of items produced per craft
	QString ConversionMaterial; ///< Material conversion rule (if any)
	QString ItemID;             ///< Item ID of the crafted product
	int ProductionTime;         ///< Time in ticks to complete the craft
	QString ResultMaterial;     ///< Specific result material (if fixed)
	QString ResultMaterialTypes;///< Allowed result material types (pipe-separated)
	QString SkillID;            ///< Skill used for crafting
};

/** @brief Row from the Crafts_Components table defining craft input materials. */
struct Crafts_Components {
	QString ID;                  ///< Craft recipe ID (parent)
	QString AllowedMaterial;     ///< Specific allowed materials
	QString AllowedMaterialType; ///< Allowed material type category
	int Amount;                  ///< Quantity of this component required
	QString ItemID;              ///< Item ID of the required component
	bool RequireSame;            ///< Whether all instances must be the same material
};

/** @brief Row from the Crafts_Prereqs table defining craft prerequisites. */
struct Crafts_Prereqs {
	QString ID;       ///< Craft recipe ID (parent)
	QString Category; ///< Prerequisite category
	QString TechGroup;///< Required technology group
	float Value;      ///< Required tech level value
};

/** @brief Row from the Crafts_SkillGain table defining skill experience gained from crafting. */
struct Crafts_SkillGain {
	QString ID; ///< Craft recipe ID (parent)
	float Value;///< Skill experience points gained
};

/** @brief Row from the Crafts_TechGain table defining technology progress from crafting. */
struct Crafts_TechGain {
	QString ID;     ///< Craft recipe ID (parent)
	QString TechID; ///< Technology ID that gains progress
	float Value;    ///< Technology progress points gained
};

/** @brief Row from the Creature_Layouts table defining multi-tile creature sprite layouts. */
struct Creature_Layouts {
	QString ID;     ///< Layout ID
	QString Offset; ///< Tile offset for this sprite piece
	QString Sprite; ///< Sprite ID for this piece
};

/** @brief Row from the Creature_Parts table defining creature body part sprites. */
struct Creature_Parts {
	QString ID;         ///< Creature type ID (parent)
	QString Part;       ///< Body part name
	QString BaseSprite; ///< Base sprite for this part
	int Order;          ///< Rendering order (lower = drawn first)
	QString Tint;       ///< Color tint applied to this part
	QString Conceales;  ///< Body parts this part visually conceals
};

/** @brief Row from the EmbeddedMaterials table defining ore veins and embedded resources. */
struct EmbeddedMaterials {
	QString ID;         ///< Embedded material ID
	int Highest;        ///< Highest Z-level where this material appears
	int Lowest;         ///< Lowest Z-level where this material appears
	QString Type;       ///< Material type category
	QString WallSprite; ///< Sprite used when embedded in a wall
};

/** @brief Row from the Events table defining game events (raids, traders, etc.). */
struct Events {
	QString ID;          ///< Event ID
	QString Trigger_;    ///< Trigger condition type
	int TriggerCount;    ///< Number of trigger units before event fires
	QString TriggerUnit; ///< Unit of measurement for the trigger (e.g. "Day")
	int NotBeforeDay;    ///< Earliest game day this event can occur
	bool NotInPeaceful;  ///< Whether this event is disabled in peaceful mode
};

/** @brief Row from the Events_Expires table defining event expiration messages. */
struct Events_Expires {
	QString ID;      ///< Event ID (parent)
	int After;       ///< Expiration time amount
	QString Unit;    ///< Expiration time unit (e.g. "Day", "Minute")
	QString Title;   ///< Notification title shown on expiration
	QString Message; ///< Notification message shown on expiration
};

/** @brief Row from the Events_Init table defining event initialization parameters. */
struct Events_Init {
	QString ID;       ///< Event ID (parent)
	QString Title;    ///< Notification title on event start
	QString Message;  ///< Notification message on event start
	float Min;        ///< Minimum value for scaled events
	float Max;        ///< Maximum value for scaled events
	bool Pause;       ///< Whether the game pauses when this event fires
	QString Require;  ///< Requirements that must be met for the event
	QString Location; ///< Location constraint for the event
};

/** @brief Row from the Events_OnFailure table defining failure outcome messages. */
struct Events_OnFailure {
	QString ID;      ///< Event ID (parent)
	QString Message; ///< Notification message on failure
	QString Title;   ///< Notification title on failure
};

/** @brief Row from the Events_OnSuccess table defining success outcome actions. */
struct Events_OnSuccess {
	QString ID;          ///< Event ID (parent)
	QString Action;      ///< Action performed on success
	bool CenterCamera;   ///< Whether the camera centers on the event location
	QString Count;       ///< Count expression for the action
	QString Message;     ///< Notification message on success
	bool Pause;          ///< Whether the game pauses on success
	QString Title;       ///< Notification title on success
};

/** @brief Row from the FarmUtils table defining farm utility items (troughs, etc.). */
struct FarmUtils {
	QString ID;      ///< Farm utility ID
	bool Buildable;  ///< Whether this utility can be built
	QString Item;    ///< Item ID used to build this utility
};

/** @brief Row from the Food_Values table defining nutritional values of food items. */
struct Food_Values {
	QString ID;         ///< Food item ID
	QString MaterialID; ///< Material ID of the food
	float Fruit;        ///< Fruit nutrition category value
	float Vegetable;    ///< Vegetable nutrition category value
	float Meat;         ///< Meat nutrition category value
	float Dairy;        ///< Dairy nutrition category value
	float Grain;        ///< Grain nutrition category value
};

/** @brief Row from the FormationPerks table listing military formation perks. */
struct FormationPerks {
	QString ID; ///< Formation perk ID
};

/** @brief Row from the Furniture table defining placeable furniture. */
struct Furniture {
	QString ID;     ///< Furniture ID
	bool Buildable; ///< Whether this furniture can be built
	QString Item;   ///< Item ID used to build this furniture
};

/** @brief Row from the Gamestart table defining items/creatures placed at embark. */
struct Gamestart {
	int Amount;         ///< Quantity to place
	QString Color;      ///< Color tint for the item
	QString ItemID;     ///< Item ID to place
	QString MaterialID; ///< Material ID for the item
	QString Offset;     ///< Tile offset from embark center
	QString Type;       ///< Entry type (e.g. "Item", "Creature")
};

/** @brief Row from the HairColors table defining available hair colors. */
struct HairColors {
	QString ID;    ///< Hair color name ID
	QString Color; ///< Color value (hex or named)
};

/** @brief Row from the Hydraulics table defining hydraulic components. */
struct Hydraulics {
	QString ID;     ///< Hydraulic component ID
	bool Buildable; ///< Whether this component can be built
	QString Item;   ///< Item ID used to build this component
	QString Sprite; ///< Sprite ID for the component
};

/** @brief Row from the ItemGrouping table defining item group display properties. */
struct ItemGrouping {
	QString ID;       ///< Item grouping ID
	QString Color;    ///< Display color for the group
	QString SpriteID; ///< Icon sprite for the group
};

/** @brief Row from the ItemGrouping_Groups table defining sub-groups. */
struct ItemGrouping_Groups {
	QString ID;       ///< Parent grouping ID
	QString GroupID;  ///< Sub-group ID
	QString SpriteID; ///< Icon sprite for the sub-group
};

/** @brief Row from the Items table defining all game items and their properties. */
struct Items {
	QString ID;                  ///< Item ID
	QString SpriteID;            ///< Display sprite
	QString Category;            ///< Item category for sorting
	QString ItemGroup;           ///< Group this item belongs to
	int StackSize;               ///< Maximum items per stack
	bool HasQuality;             ///< Whether the item can have quality levels
	int Value;                   ///< Base trade/value
	int EatValue;                ///< Nutritional value when eaten
	int DrinkValue;              ///< Hydration value when drunk
	bool IsContainer;            ///< Whether this item is a container
	bool IsTool;                 ///< Whether this item is a tool
	int LightIntensity;          ///< Light emitted by this item (0 = none)
	bool HasComponents;          ///< Whether this item has sub-components
	QString AllowedMaterialTypes;///< Allowed material types (pipe-separated)
	QString AllowedMaterials;    ///< Specific allowed materials (pipe-separated)
	QString AllowedContainers;   ///< Container types that can hold this item
	QString CarryContainer;      ///< Container used when carried
	int AttackValue;             ///< Damage value when used as a weapon
	int BurnValue;               ///< Fuel value when burned
};

/** @brief Row from the Items_Components table defining item sub-components. */
struct Items_Components {
	QString ID;     ///< Item ID (parent)
	QString ItemID; ///< Component item ID
	bool NoMaterial;///< Whether this component ignores material
};

/** @brief Row from the Items_Tiles table defining item placement constraints. */
struct Items_Tiles {
	QString ID;        ///< Item ID (parent)
	QString Forbidden; ///< Forbidden tile conditions
	QString Required;  ///< Required tile conditions
	QString Location;  ///< Placement location constraint
	QString Offset;    ///< Tile offset
};

/** @brief Sprite ID entry for a job, defining what to render at each offset during the job. */
struct Job_SpriteID {
	Position Offset; ///< Tile offset from the job origin
	bool Rotate;     ///< Whether the sprite should rotate with job orientation
	QString SpriteID;///< Sprite to display
	QString Type;    ///< Sprite type (e.g. "Construction", "Floor")
};

/** @brief Individual task step within a job definition. */
struct Job_Task {
	QString ConstructionID; ///< Construction ID being built (if applicable)
	int Duration;           ///< Duration in ticks for this task step
	QString Material;       ///< Material requirement for the task
	Position Offset;        ///< Tile offset where the task is performed
	QString Task;           ///< Task type identifier (e.g. "PickUp", "Build")
};

/** @brief Complete job definition including tasks, sprites, and work positions. */
struct Job {
	QString ID;                   ///< Job ID
	QString ConstructionType;     ///< Type of construction this job creates
	bool MayTrapGnome;            ///< Whether performing this job may trap the gnome
	QString RequiredToolItemID;   ///< Item ID of the required tool
	QString RequiredToolLevel;    ///< Minimum tool level required
	QString SkillGain;            ///< Skill experience gained on completion
	QString SkillID;              ///< Skill used to perform the job
	QString TechGain;             ///< Technology progress gained on completion
	QList<Position> WorkPositions;///< Valid positions from which a gnome can work
	QList<Job_Task> tasks;        ///< Ordered list of task steps
	QList<Job_SpriteID>sprites;   ///< Sprites shown during/after the job
};

/** @brief Row from the Lairs table defining monster lair properties. */
struct Lairs {
	QString ID;     ///< Lair ID
	QString Type;   ///< Lair type category
	QString Size;   ///< Lair size descriptor
	QString Layout; ///< Layout template ID
};

/** @brief Row from the Lairs_Spawns table defining what spawns in a lair. */
struct Lairs_Spawns {
	QString ID;     ///< Lair ID (parent)
	QString Type;   ///< Creature type to spawn
	QString Offset; ///< Tile offset for spawn location
	int Level;      ///< Creature level
	int Rotation;   ///< Facing rotation of the spawned creature
	int Gender;     ///< Gender of the spawned creature (0/1)
};

/** @brief Row from the Lairs_Tiles table defining lair tile composition. */
struct Lairs_Tiles {
	QString ID;     ///< Lair ID (parent)
	QString Offset; ///< Tile offset from lair origin
	QString Type;   ///< Tile type (e.g. "Wall", "Floor")
};

/** @brief Row from the Magic table defining magic types. */
struct Magic {
	QString ID;    ///< Magic type ID
	QString Color; ///< Display color for this magic type
	QString Spells;///< Pipe-separated list of available spell IDs
};

/** @brief Row from the Materials table defining material properties. */
struct Materials {
	QString ID;      ///< Material ID
	QString Color;   ///< Display color
	float Strength;  ///< Material strength value
	QString Type;    ///< Material type category (e.g. "Metal", "Stone")
	float Value;     ///< Base trade value
};

/** @brief Row from the MaterialToToolLevel table mapping materials to tool levels. */
struct MaterialToToolLevel {
	QString ID;           ///< Material or material type ID
	int RequiredToolLevel;///< Tool level required to work with this material
	int ToolLevel;        ///< Tool level this material provides when used as a tool
};

/** @brief Row from the Mechanism table defining mechanical components. */
struct Mechanism {
	QString ID;            ///< Mechanism ID
	QString GUI;           ///< GUI panel ID for this mechanism
	bool Buildable;        ///< Whether this mechanism can be built
	QString Item;          ///< Item ID used to build this mechanism
	QString Sprite;        ///< Default sprite
	int MaxFuel;           ///< Maximum fuel capacity
	int ProducePower;      ///< Power output when active
	int ConsumePower;      ///< Power consumed when active
	bool Anim;             ///< Whether this mechanism has animation
	QString WallSpriteOn;  ///< Wall sprite when powered on
	QString WallSpriteOff; ///< Wall sprite when powered off
	QString FloorSpriteOn; ///< Floor sprite when powered on
	QString FloorSpriteOff;///< Floor sprite when powered off
	QString EffectOn;      ///< Visual effect when powered on
	QString EffectOff;     ///< Visual effect when powered off
};

/** @brief Row from the Missions table defining mission templates. */
struct Missions {
	QString ID;      ///< Mission ID
	int TypeInt;     ///< Mission type as integer enum
	int MinGnomes;   ///< Minimum gnomes required
	int MaxGnomes;   ///< Maximum gnomes allowed
	QString Target;  ///< Mission target descriptor
	QString Actions; ///< Available actions during the mission
};

/** @brief Row from the Monsters table defining monster species. */
struct Monsters {
	QString ID;           ///< Monster species ID
	QString BehaviorTree; ///< Behavior tree for this monster
	QString Food;         ///< Food type this monster eats
};

/** @brief Row from the Monsters_Levels table defining monster level scaling. */
struct Monsters_Levels {
	QString ID;    ///< Monster species ID (parent)
	int Level;     ///< Monster level
	QString Sprite;///< Sprite for this level
};

/** @brief Row from the MoveSpeed table mapping creature skill to movement speed. */
struct MoveSpeed {
	QString Creature; ///< Creature type ID
	int Skill;        ///< Movement skill level
	int Speed;        ///< Movement speed value (lower = faster)
};

/** @brief Row from the Namerules table defining name generation rule sets. */
struct Namerules {
	QString ID; ///< Name rule set ID
};

/** @brief Row from the Namerules_Rule table defining individual name generation parts. */
struct Namerules_Rule {
	QString ID;   ///< Name rule set ID (parent)
	QString Part; ///< Name part pattern
};

/** @brief Row from the Names table listing individual names by gender. */
struct Names {
	QString ID;     ///< The name string
	QString Gender; ///< Gender this name is used for
};

/** @brief Row from the Needs table defining creature needs (hunger, sleep, etc.). */
struct Needs {
	QString ID;           ///< Need ID (e.g. "Hunger", "Sleep")
	QString BarColor;     ///< UI bar color for this need
	float DecayPerMinute; ///< Rate at which this need depletes per game minute
	float GainFromSleep;  ///< Amount restored per sleep tick
	float Max_;           ///< Maximum value for this need
	QString Creature;     ///< Creature type this need applies to
};

/** @brief Row from the Needs_States table defining need threshold states. */
struct Needs_States {
	QString ID;             ///< Need ID (parent)
	QString Action;         ///< Action triggered at this threshold
	QString ID2;            ///< State sub-ID
	int Priority;           ///< Priority of this state relative to others
	QString ThoughtBubble;  ///< Thought bubble sprite shown at this state
	float Threshold;        ///< Need value threshold that triggers this state
};

/** @brief Row from the Needs_States_Modifiers table defining stat modifiers from need states. */
struct Needs_States_Modifiers {
	QString ID;       ///< Need ID (parent)
	QString Attribute;///< Attribute affected by the modifier
	QString Type;     ///< Modifier type (e.g. "Add", "Multiply")
	float Value_;     ///< Modifier value
};

/** @brief Row from the Plants table defining plant species properties. */
struct Plants {
	QString ID;                 ///< Plant species ID
	bool AllowInWild;           ///< Whether this plant spawns in the wild
	QString FruitItemID;        ///< Item ID of the fruit produced
	QString GrowsIn;            ///< Terrain types where this plant can grow
	QString GrowsInSeason;      ///< Seasons when the plant grows
	QString IsKilledInSeason;   ///< Season that kills this plant
	bool IsLarge;               ///< Whether this is a large (multi-tile) plant/tree
	QString LosesFruitInSeason; ///< Season when fruit drops
	QString Material;           ///< Material ID of the plant
	int NumFruitsPerSeason;     ///< Number of fruits produced per season
	QString SeedItemID;         ///< Item ID of the seed for farming
	QString ToolButtonSprite;   ///< Sprite used in the farming toolbar
	QString Type;               ///< Plant type (e.g. "Tree", "Crop", "Bush")
};

/** @brief Row from the Plants_OnFell table defining items dropped when a tree is felled. */
struct Plants_OnFell {
	QString ID;         ///< Plant species ID (parent)
	QString ItemID;     ///< Item ID of the drop
	QString MaterialID; ///< Material of the drop
	int Random;         ///< Random chance weight for this drop
};

/** @brief Row from the Plants_OnHarvest table defining harvest actions. */
struct Plants_OnHarvest {
	QString ID;     ///< Plant species ID (parent)
	QString Action; ///< Harvest action type
};

/** @brief Row from the Plants_OnHarvest_HarvestedItem table defining harvest drops. */
struct Plants_OnHarvest_HarvestedItem {
	QString ID;         ///< Plant species ID (parent)
	float Chance;       ///< Drop chance (0.0 to 1.0)
	QString ItemID;     ///< Item ID of the harvested item
	QString MaterialID; ///< Material of the harvested item
};

/** @brief Row from the Plants_States table defining plant growth stages. */
struct Plants_States {
	QString ID;              ///< Plant species ID (parent)
	bool Fell;               ///< Whether the plant can be felled in this state
	float GrowTime;          ///< Base time to grow through this state
	float GrowTimeDeviation; ///< Random deviation in grow time
	bool arvest;             ///< Whether the plant can be harvested in this state (note: typo in DB schema)
	QString ID2;             ///< Growth state sub-ID
	QString Layout;          ///< Layout template for multi-tile plants
	QString SpriteID;        ///< Sprite for this growth state
};

/** @brief Row from the PositionPerks table listing military position perks. */
struct PositionPerks {
	QString ID; ///< Position perk ID
};

/** @brief Row from the Quality table defining item quality levels. */
struct Quality {
	QString ID;    ///< Quality level name (e.g. "Normal", "Fine")
	int Rank;      ///< Numeric rank (higher = better)
	float Modifier;///< Stat multiplier applied to items of this quality
};

/** @brief Row from the RandomMetals table defining ore distribution weights per map. */
struct RandomMetals {
	QString ID;   ///< Distribution profile ID
	int Copper;   ///< Weight for copper ore
	int Tin;      ///< Weight for tin ore
	int Malachite; ///< Weight for malachite ore
	int Iron;     ///< Weight for iron ore
	int Lead;     ///< Weight for lead ore
	int Silver;   ///< Weight for silver ore
	int Gold;     ///< Weight for gold ore
	int Platinum; ///< Weight for platinum ore
};

/** @brief Row from the Seasons table defining season properties. */
struct Seasons {
	QString ID;           ///< Season name ID (e.g. "Spring", "Summer")
	QString NextSeason;   ///< ID of the following season
	int NumDays;          ///< Number of game days in this season
	QString SunRiseFirst; ///< Sunrise time on the first day
	QString SunsetFirst;  ///< Sunset time on the first day
};

/** @brief Row from the SkillGroups table defining skill group display properties. */
struct SkillGroups {
	QString ID;      ///< Skill group ID
	QString Color;   ///< Display color in the UI
	int Position;    ///< Sort position in the skill list
	QString SkillID; ///< Associated skill ID
	QString Text;    ///< Display text label
};

/** @brief Row from the Skills table defining individual skills. */
struct Skills {
	QString ID;                ///< Skill ID (e.g. "Mining", "Masonry")
	QString RequiredToolItemID;///< Item ID of the tool required for this skill
	QString SkillGroup;        ///< Skill group this skill belongs to
};

/** @brief Row from the Sounds table mapping sound IDs to audio files. */
struct Sounds {
	QString ID;       ///< Sound ID
	QString SoundFile;///< Path to the audio file
};

/** @brief Row from the Spells table defining spell properties. */
struct Spells {
	QString ID;                 ///< Spell ID
	QString EffectRequirements; ///< Conditions required for the spell to take effect
	QString Effects;            ///< Pipe-separated list of effects applied
	QString Radius;             ///< Area-of-effect radius
	QString SkillID;            ///< Skill used to cast the spell
};

/** @brief Row from the Sprites table defining composite sprite configurations. */
struct Sprites {
	QString ID;              ///< Sprite ID
	bool Anim;               ///< Whether this sprite has animation frames
	QString BaseSprite;      ///< Base sprite ID for the default appearance
	bool HasRandom;          ///< Whether random variants exist
	QString Offset;          ///< Rendering offset
	bool Rot90;              ///< Whether 90-degree rotation variants exist
	QString Tint;            ///< Default tint applied to the sprite
	QString DefaultMaterial; ///< Default material for tinting
	bool HasTransp;          ///< Whether the sprite has transparency
};

/** @brief Row from the Sprites_ByMaterials table for material-specific sprite overrides. */
struct Sprites_ByMaterials {
	QString ID;         ///< Sprite ID (parent)
	QString BaseSprite; ///< Base sprite ID
	QString Effect;     ///< Visual effect applied
	QString MaterialID; ///< Material that triggers this override
	QString Sprite;     ///< Override sprite ID
};

/** @brief Row from the Sprites_ByMaterialTypes table for material-type-specific sprite overrides. */
struct Sprites_ByMaterialTypes {
	QString ID;           ///< Sprite ID (parent)
	QString BaseSprite;   ///< Base sprite ID
	QString MaterialType; ///< Material type that triggers this override
	QString Sprite;       ///< Override sprite ID
};

/** @brief Row from the Sprites_Combine table defining sprite composition layers. */
struct Sprites_Combine {
	QString ID;         ///< Sprite ID (parent)
	QString BaseSprite; ///< Base sprite ID for this layer
	QString Offset;     ///< Rendering offset for this layer
	QString Sprite;     ///< Sprite used for this layer
	QString Tint;       ///< Tint applied to this layer
};

/** @brief Row from the Sprites_Frames table defining animation frame base sprites. */
struct Sprites_Frames {
	QString ID;         ///< Sprite ID (parent)
	QString BaseSprite; ///< Base sprite for this frame
};

/** @brief Row from the Sprites_Random table defining random sprite variant weights. */
struct Sprites_Random {
	QString ID;         ///< Sprite ID (parent)
	QString BaseSprite; ///< Base sprite ID
	QString Sprite;     ///< Variant sprite ID
	float Weight;       ///< Selection weight for this variant
};

/** @brief Row from the Sprites_Rotations table defining rotation-specific sprite variants. */
struct Sprites_Rotations {
	QString ID;         ///< Sprite ID (parent)
	QString BaseSprite; ///< Base sprite ID
	QString Effect;     ///< Visual effect for this rotation
	QString Rotation;   ///< Rotation value (e.g. "FR", "FL", "BR", "BL")
	QString Sprite;     ///< Sprite used for this rotation
};

/** @brief Row from the Sprites_Seasons table defining season-specific sprite variants. */
struct Sprites_Seasons {
	QString ID;         ///< Sprite ID (parent)
	QString BaseSprite; ///< Base sprite for this season
	QString Season;     ///< Season this variant applies to
};

/** @brief Row from the Sprites_Seasons_Rotations table for season+rotation sprite variants. */
struct Sprites_Seasons_Rotations {
	QString ID;         ///< Sprite ID (parent)
	QString BaseSprite; ///< Base sprite for this variant
	QString Rotation;   ///< Rotation value
};

/** @brief Row from the Tech table listing technology IDs. */
struct Tech {
	QString ID; ///< Technology ID
};

/** @brief Row from the TerrainMaterials table defining terrain layer appearance. */
struct TerrainMaterials {
	QString ID;              ///< Terrain material ID
	QString FloorSprite;     ///< Sprite used for floor tiles
	int Highest;             ///< Highest Z-level this material appears at
	int Lowest;              ///< Lowest Z-level this material appears at
	QString ShortWallSprite; ///< Sprite for short wall tiles
	QString Type;            ///< Material type category
	QString WallSprite;      ///< Sprite for full wall tiles
};

/** @brief Row from the Time table defining named time constants. */
struct Time {
	QString ID; ///< Time constant name
	int Value_; ///< Value in game ticks
};

/** @brief Row from the Traders table listing trader types. */
struct Traders {
	QString ID; ///< Trader type ID
};

/** @brief Row from the Traders_Items table defining items traders can buy/sell. */
struct Traders_Items {
	QString ID;       ///< Trader type ID (parent)
	QString Gender;   ///< Gender filter for the item
	QString Item;     ///< Item ID
	QString Material; ///< Material filter
	int Max_;         ///< Maximum quantity offered
	int Min_;         ///< Minimum quantity offered
	QString Type;     ///< Trade type (e.g. "Buy", "Sell")
	int Value_;       ///< Trade value
};

/** @brief Row from the Translation table mapping IDs to localized text. */
struct Translation {
	QString ID;   ///< Translation key
	QString Text; ///< Localized text string
};

/** @brief Row from the TreeLayouts table listing tree layout template IDs. */
struct TreeLayouts {
	QString ID; ///< Tree layout ID
};

/** @brief Row from the TreeLayouts_Layout table defining individual tree tile positions. */
struct TreeLayouts_Layout {
	QString ID;       ///< Tree layout ID (parent)
	bool FruitPos;    ///< Whether this tile can have fruit
	QString Offset;   ///< Tile offset from tree origin
	QString Rotation; ///< Rotation of the sprite at this tile
	QString SpriteID; ///< Sprite displayed at this tile
};

/** @brief Row from the Uniform table defining uniform slot configurations. */
struct Uniform {
	QString ID; ///< Uniform slot type ID
	int Sides;  ///< Number of sides/slots (e.g. 2 for "Left"/"Right")
};

/** @brief Row from the Uniform_Slots table defining individual uniform slot entries. */
struct Uniform_Slots {
	QString ID;           ///< Uniform ID (parent)
	QString Type;         ///< Slot type (e.g. "Head", "Chest")
	QString ItemID;       ///< Required item ID for this slot
	QString MaterialType; ///< Required material type
};

/** @brief Row from the Uniforms table defining saved uniform presets. */
struct Uniforms {
	int ID;           ///< Numeric uniform preset ID
	QString Name;     ///< Display name
	bool UserDefined; ///< Whether this preset was created by the player
};

/** @brief Row from the Utility table defining utility constructions. */
struct Utility {
	QString ID;     ///< Utility ID
	bool Buildable; ///< Whether this utility can be built
	QString Item;   ///< Item ID used to build this utility
};

/** @brief Row from the Words table for procedural name generation. */
struct Words {
	QString Word; ///< Base word
};

/** @brief Row from the Words_ActionNoun table for procedural name generation. */
struct Words_ActionNoun {
	QString Plural; ///< Plural form of the action noun
	QString Word;   ///< Singular form
};

/** @brief Row from the Words_Adjective table for procedural name generation. */
struct Words_Adjective {
	QString Word; ///< Adjective word
};

/** @brief Row from the Words_Noun table for procedural name generation. */
struct Words_Noun {
	QString Plural; ///< Plural form
	QString Word;   ///< Singular form
};

/** @brief Row from the Words_Numbers table mapping number words to values. */
struct Words_Numbers {
	QString Word; ///< Number as a word (e.g. "One", "Two")
	int Number;   ///< Numeric value
};

/** @brief Row from the Words_Verb table for procedural name generation with verb conjugations. */
struct Words_Verb {
	QString PastParticiple;    ///< Past participle form (e.g. "broken")
	QString PresentParticiple; ///< Present participle form (e.g. "breaking")
	QString SimplePast;        ///< Simple past form (e.g. "broke")
	QString SimplePresent;     ///< Simple present form (e.g. "breaks")
	QString Word;              ///< Base/infinitive form (e.g. "break")
};

/** @brief Component definition for a workshop tile, including sprite and placement info. */
struct Workshop_Component {
	int Amount = 0;       ///< Number of items required for this component
	QString ItemID;       ///< Item ID of the required component
	QString MaterialItem; ///< Material or item constraint
	Position Offset;      ///< Tile offset from the workshop origin
	QString Required;     ///< Required tile conditions at this offset
	QString Forbidden;    ///< Forbidden tile conditions at this offset
	QString SpriteID;     ///< Primary sprite for this component tile
	QString SpriteID2;    ///< Secondary/alternate sprite
	QString Type;         ///< Component type (e.g. "Craft", "Input", "Output")
	QString WallRotation; ///< Wall rotation constraint
	bool IsFloor = false; ///< Whether this component is a floor tile
};

/** @brief Complete workshop definition including layout, crafts, and I/O tiles. */
struct Workshop {
	QString ID;                        ///< Workshop ID
	QStringList Crafts;                ///< List of craft recipe IDs this workshop supports
	QString GUI;                       ///< GUI panel ID for the workshop interface
	Position InputTile;                ///< Tile where input items are delivered
	Position OutputTile;               ///< Tile where crafted items are placed
	QString Size;                      ///< Workshop footprint size (e.g. "3x3")
	bool NoAutoGenerate = false;       ///< Whether to skip auto-generating default crafts
	QString Icon;                      ///< Icon sprite for the workshop in menus
	QString Tab;                       ///< UI tab this workshop appears under

	QList<Workshop_Component> components; ///< List of component tiles that make up the workshop
};

}
