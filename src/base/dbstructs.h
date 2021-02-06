#include "../base/position.h"

#include <QString>



namespace DBS
{

struct Actions {
	QString ID;
	QString Job;
	QString ConstructionType;
	bool ConstructionSelect;
	bool IsFloor;
	bool Multi;
	bool MultiZ;
	bool Rotate;
};

struct Actions_Tiles {
	QString ID;
	QString Offset;
	QString SpriteID;
	bool IsFloor;
	QString Required;
	QString Forbidden;
};

struct AI {
	QString ID;
	QString BehaviorTree;
};

struct Anatomy {
	QString ID;
	int Blood;
	QString Root;
	QString LayerOrder;
};

struct Anatomy_Parts {
	QString ID;
	QString ID2;
	QString Parent;
	bool IsInside;
	QString Height;
	QString Side;
	QString Facing;
	int HP;
	bool IsVital;
};

struct Animals {
	QString ID;
	bool AllowInWild;
	bool Aquatic;
	QString BehaviorTree;
	QString Biome;
	bool Embark;
	QString Food;
	int GestationDays;
	bool Pasture;
	int PastureSize;
	QString Prey;
	bool IsMulti;
};

struct Animals_OnButcher {
	QString ID;
	int Amount;
	QString ItemID;
	QString Type;
};

struct Animals_States {
	QString ID;
	QString ID2;
	QString SpriteID;
	int DaysToNextState;
	bool Immobile;
	QString BehaviorTree;
	bool IsAggro;
	int Attack;
	int Damage;
	QString Anatomy;
};

struct Animals_States_Behavior {
	QString ID;
	QString ID2;
	QString RequiredGender;
	int Amount;
	QString CreatureID;
	int DaysBetween;
	int EatTime;
	QString EggID;
	int FoodValue;
	float HungerPerTick;
	QString ItemID;
	float Speed;
};

struct Attributes {
	QString ID;
};

struct Automaton_Cores {
	QString ID;
	QString BehaviorTree;
};

struct Automaton_Cores_Skills {
	QString ID;
	QString SkillID;
	int SkillValue;
};

struct BaseItems {
	QString ID;
};

struct BaseSprites {
	QString ID;
	QString SourceRectangle;
	QString Tilesheet;
};

struct Constructions {
	QString ID;
	bool oConstruction;
	bool Rotation;
	QString Type;
	QString Category;
};

struct Constructions_Components {
	QString ID;
	int Amount;
	QString ItemID;
	QString MaterialTypes;
	QString Materials;
};

struct Constructions_IntermediateSprites {
	QString ID;
	QString Offset;
	float Percent;
	QString SpriteID;
	QString Type;
};

struct Constructions_Sprites {
	QString ID;
	QString Offset;
	QString SpriteID;
	QString SpriteIDOverride;
	QString Type;
};

struct ConstructionTypes {
	QString ID;
};

struct Containers {
	QString ID;
	bool Buildable;
	int Capacity;
	bool RequireSame;
	QString Type;
	QString Item;
};

struct Containers_Components {
	QString ID;
	QString ItemID;
};

struct Containers_Tiles {
	QString ID;
	bool Construction;
	bool Job;
	QString Offset;
	QString SpriteID;
	bool Stockpile;
};

struct Crafts {
	QString ID;
	int Amount;
	QString ConversionMaterial;
	QString ItemID;
	int ProductionTime;
	QString ResultMaterial;
	QString ResultMaterialTypes;
	QString SkillID;
};

struct Crafts_Components {
	QString ID;
	QString AllowedMaterial;
	QString AllowedMaterialType;
	int Amount;
	QString ItemID;
	bool RequireSame;
};

struct Crafts_Prereqs {
	QString ID;
	QString Category;
	QString TechGroup;
	float Value;
};

struct Crafts_SkillGain {
	QString ID;
	float Value;
};

struct Crafts_TechGain {
	QString ID;
	QString TechID;
	float Value;
};

struct Creature_Layouts {
	QString ID;
	QString Offset;
	QString Sprite;
};

struct Creature_Parts {
	QString ID;
	QString Part;
	QString BaseSprite;
	int Order;
	QString Tint;
	QString Conceales;
};

struct EmbeddedMaterials {
	QString ID;
	int Highest;
	int Lowest;
	QString Type;
	QString WallSprite;
};

struct Events {
	QString ID;
	QString Trigger_;
	int TriggerCount;
	QString TriggerUnit;
	int NotBeforeDay;
	bool NotInPeaceful;
};

struct Events_Expires {
	QString ID;
	int After;
	QString Unit;
	QString Title;
	QString Message;
};

struct Events_Init {
	QString ID;
	QString Title;
	QString Message;
	float Min;
	float Max;
	bool Pause;
	QString Require;
	QString Location;
};

struct Events_OnFailure {
	QString ID;
	QString Message;
	QString Title;
};

struct Events_OnSuccess {
	QString ID;
	QString Action;
	bool CenterCamera;
	QString Count;
	QString Message;
	bool Pause;
	QString Title;
};

struct FarmUtils {
	QString ID;
	bool Buildable;
	QString Item;
};

struct Food_Values {
	QString ID;
	QString MaterialID;
	float Fruit;
	float Vegetable;
	float Meat;
	float Dairy;
	float Grain;
};

struct FormationPerks {
	QString ID;
};

struct Furniture {
	QString ID;
	bool Buildable;
	QString Item;
};

struct Gamestart {
	int Amount;
	QString Color;
	QString ItemID;
	QString MaterialID;
	QString Offset;
	QString Type;
};

struct HairColors {
	QString ID;
	QString Color;
};

struct Hydraulics {
	QString ID;
	bool Buildable;
	QString Item;
	QString Sprite;
};

struct ItemGrouping {
	QString ID;
	QString Color;
	QString SpriteID;
};

struct ItemGrouping_Groups {
	QString ID;
	QString GroupID;
	QString SpriteID;
};

struct Items {
	QString ID;
	QString SpriteID;
	QString Category;
	QString ItemGroup;
	int StackSize;
	bool HasQuality;
	int Value;
	int EatValue;
	int DrinkValue;
	bool IsContainer;
	bool IsTool;
	int LightIntensity;
	bool HasComponents;
	QString AllowedMaterialTypes;
	QString AllowedMaterials;
	QString AllowedContainers;
	QString CarryContainer;
	int AttackValue;
	int BurnValue;
};

struct Items_Components {
	QString ID;
	QString ItemID;
	bool NoMaterial;
};

struct Items_Tiles {
	QString ID;
	QString Forbidden;
	QString Required;
	QString Location;
	QString Offset;
};

struct Job_SpriteID {
	Position Offset;
	bool Rotate;
	QString SpriteID;
	QString Type;
};

struct Job_Task {
	QString ConstructionID;
	int Duration;
	QString Material;
	Position Offset;
	QString Task;
};

struct Job {
	QString ID;
	QString ConstructionType;
	bool MayTrapGnome;
	QString RequiredToolItemID;
	QString RequiredToolLevel;
	QString SkillGain;
	QString SkillID;
	QString TechGain;
	QList<Position> WorkPositions;
	QList<Job_Task> tasks;
	QList<Job_SpriteID>sprites;
};

struct Lairs {
	QString ID;
	QString Type;
	QString Size;
	QString Layout;
};

struct Lairs_Spawns {
	QString ID;
	QString Type;
	QString Offset;
	int Level;
	int Rotation;
	int Gender;
};

struct Lairs_Tiles {
	QString ID;
	QString Offset;
	QString Type;
};

struct Magic {
	QString ID;
	QString Color;
	QString Spells;
};

struct Materials {
	QString ID;
	QString Color;
	float Strength;
	QString Type;
	float Value;
};

struct MaterialToToolLevel {
	QString ID;
	int RequiredToolLevel;
	int ToolLevel;
};

struct Mechanism {
	QString ID;
	QString GUI;
	bool Buildable;
	QString Item;
	QString Sprite;
	int MaxFuel;
	int ProducePower;
	int ConsumePower;
	bool Anim;
	QString WallSpriteOn;
	QString WallSpriteOff;
	QString FloorSpriteOn;
	QString FloorSpriteOff;
	QString EffectOn;
	QString EffectOff;
};

struct Missions {
	QString ID;
	int TypeInt;
	int MinGnomes;
	int MaxGnomes;
	QString Target;
	QString Actions;
};

struct Monsters {
	QString ID;
	QString BehaviorTree;
	QString Food;
};

struct Monsters_Levels {
	QString ID;
	int Level;
	QString Sprite;
};

struct MoveSpeed {
	QString Creature;
	int Skill;
	int Speed;
};

struct Namerules {
	QString ID;
};

struct Namerules_Rule {
	QString ID;
	QString Part;
};

struct Names {
	QString ID;
	QString Gender;
};

struct Needs {
	QString ID;
	QString BarColor;
	float DecayPerMinute;
	float GainFromSleep;
	float Max_;
	QString Creature;
};

struct Needs_States {
	QString ID;
	QString Action;
	QString ID2;
	int Priority;
	QString ThoughtBubble;
	float Threshold;
};

struct Needs_States_Modifiers {
	QString ID;
	QString Attribute;
	QString Type;
	float Value_;
};

struct Plants {
	QString ID;
	bool AllowInWild;
	QString FruitItemID;
	QString GrowsIn;
	QString GrowsInSeason;
	QString IsKilledInSeason;
	bool IsLarge;
	QString LosesFruitInSeason;
	QString Material;
	int NumFruitsPerSeason;
	QString SeedItemID;
	QString ToolButtonSprite;
	QString Type;
};

struct Plants_OnFell {
	QString ID;
	QString ItemID;
	QString MaterialID;
	int Random;
};

struct Plants_OnHarvest {
	QString ID;
	QString Action;
};

struct Plants_OnHarvest_HarvestedItem {
	QString ID;
	float Chance;
	QString ItemID;
	QString MaterialID;
};

struct Plants_States {
	QString ID;
	bool Fell;
	float GrowTime;
	float GrowTimeDeviation;
	bool arvest;
	QString ID2;
	QString Layout;
	QString SpriteID;
};

struct PositionPerks {
	QString ID;
};

struct Quality {
	QString ID;
	int Rank;
	float Modifier;
};

struct RandomMetals {
	QString ID;
	int Copper;
	int Tin;
	int Malachite;
	int Iron;
	int Lead;
	int Silver;
	int Gold;
	int Platinum;
};

struct Seasons {
	QString ID;
	QString NextSeason;
	int NumDays;
	QString SunRiseFirst;
	QString SunsetFirst;
};

struct SkillGroups {
	QString ID;
	QString Color;
	int Position;
	QString SkillID;
	QString Text;
};

struct Skills {
	QString ID;
	QString RequiredToolItemID;
	QString SkillGroup;
};

struct Sounds {
	QString ID;
	QString SoundFile;
};

struct Spells {
	QString ID;
	QString EffectRequirements;
	QString Effects;
	QString Radius;
	QString SkillID;
};

struct Sprites {
	QString ID;
	bool Anim;
	QString BaseSprite;
	bool HasRandom;
	QString Offset;
	bool Rot90;
	QString Tint;
	QString DefaultMaterial;
	bool HasTransp;
};

struct Sprites_ByMaterials {
	QString ID;
	QString BaseSprite;
	QString Effect;
	QString MaterialID;
	QString Sprite;
};

struct Sprites_ByMaterialTypes {
	QString ID;
	QString BaseSprite;
	QString MaterialType;
	QString Sprite;
};

struct Sprites_Combine {
	QString ID;
	QString BaseSprite;
	QString Offset;
	QString Sprite;
	QString Tint;
};

struct Sprites_Frames {
	QString ID;
	QString BaseSprite;
};

struct Sprites_Random {
	QString ID;
	QString BaseSprite;
	QString Sprite;
	float Weight;
};

struct Sprites_Rotations {
	QString ID;
	QString BaseSprite;
	QString Effect;
	QString Rotation;
	QString Sprite;
};

struct Sprites_Seasons {
	QString ID;
	QString BaseSprite;
	QString Season;
};

struct Sprites_Seasons_Rotations {
	QString ID;
	QString BaseSprite;
	QString Rotation;
};

struct Tech {
	QString ID;
};

struct TerrainMaterials {
	QString ID;
	QString FloorSprite;
	int Highest;
	int Lowest;
	QString ShortWallSprite;
	QString Type;
	QString WallSprite;
};

struct Time {
	QString ID;
	int Value_;
};

struct Traders {
	QString ID;
};

struct Traders_Items {
	QString ID;
	QString Gender;
	QString Item;
	QString Material;
	int Max_;
	int Min_;
	QString Type;
	int Value_;
};

struct Translation {
	QString ID;
	QString Text;
};

struct TreeLayouts {
	QString ID;
};

struct TreeLayouts_Layout {
	QString ID;
	bool FruitPos;
	QString Offset;
	QString Rotation;
	QString SpriteID;
};

struct Uniform {
	QString ID;
	int Sides;
};

struct Uniform_Slots {
	QString ID;
	QString Type;
	QString ItemID;
	QString MaterialType;
};

struct Uniforms {
	int ID;
	QString Name;
	bool UserDefined;
};

struct Utility {
	QString ID;
	bool Buildable;
	QString Item;
};

struct Words {
	QString Word;
};

struct Words_ActionNoun {
	QString Plural;
	QString Word;
};

struct Words_Adjective {
	QString Word;
};

struct Words_Noun {
	QString Plural;
	QString Word;
};

struct Words_Numbers {
	QString Word;
	int Number;
};

struct Words_Verb {
	QString PastParticiple;
	QString PresentParticiple;
	QString SimplePast;
	QString SimplePresent;
	QString Word;
};

struct Workshop_Component {
	int Amount = 0;
	QString ItemID;
	QString MaterialItem;
	Position Offset;
	QString Required;
	QString Forbidden;
	QString SpriteID;
	QString SpriteID2;
	QString Type;
	QString WallRotation;
	bool IsFloor = false;
};

struct Workshop {
	QString ID;
	QStringList Crafts;
	QString GUI;
	Position InputTile;
	Position OutputTile;
	QString Size;
	bool NoAutoGenerate = false;
	QString Icon;
	QString Tab;

	QList<Workshop_Component> components;
};

}