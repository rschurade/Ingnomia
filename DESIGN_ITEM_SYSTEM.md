# Item System Redesign — Design Document

## Status: Phase 1 IN PROGRESS

## Problem Statement

The current item system has multiple tracking structures (m_items, m_positionHash, m_hash, m_octrees) that must stay in sync manually. Items have 5 independent ownership flags (isInStockpile, isInJob, isHeldBy, isUsedBy, isInContainer) with no validation. This leads to:

- **Ghost items**: items stuck in "inJob" or "heldBy" state after the job/creature is gone
- **Phantom constructed items**: items marked "constructed" remain in position hash/octrees but are part of a building
- **Orphaned items**: exist in m_items but not reachable through any index
- **Position hash corruption**: operator[] creating empty entries outside contains() guards
- **Incomplete sanityCheck()**: only validates container contents, misses everything else

Additionally, installed items (containers, mechanisms, doors, furniture) remain as live Item objects in inventory, creating a hybrid state where they're neither free items nor proper world entities. Subsystems store item IDs and query inv() at runtime, creating tight coupling and fragile cross-system references.

The carry container system (wheelbarrow, bucket, sack, backpack) uses items-inside-items for hauling, adding unnecessary complexity.

## Design Principles

1. **Items are movable things.** If it's placed in the world, it becomes an entity owned by a subsystem, not an item.
2. **Two orthogonal axes**: Location (where the item is) and Claim (who reserved it)
3. **Destroy on construction**: ALL items used for building/installing are destroyed
4. **Carry containers are capacity modifiers**, not nested item storage
5. **Owner-indexed cleanup**: instant bulk cleanup when a job is cancelled or creature dies
6. **Validated transitions**: state machine rejects invalid transitions

## Core Concept: Items vs Installed Entities

When an item is installed/constructed into the world, it is **destroyed** and becomes an **entity** owned by the relevant subsystem. On deconstruction, a new item is created from the entity's stored properties.

```
ITEM LIFECYCLE:
  Crafted → [movable item in Inventory] → Installed → DESTROYED
  
ENTITY LIFECYCLE:
  Created on installation → [owned by subsystem] → Deconstructed → DESTROYED
  → New item created from stored properties
```

This mirrors how Workshops already work: source items are consumed, workshop entity is created, and deconstruction produces fresh items.

### Entity Types by Subsystem

| Subsystem | Entity | Current State | What Changes |
|-----------|--------|---------------|--------------|
| WorkshopManager | Workshop | Already an entity | Store SourceMaterial instead of live item IDs |
| MechanismManager | MechanismData | 90% entity already, still queries inv() | Cache itemSID/materialSID/spriteID at install, stop querying inv() |
| StockpileManager | ContainerEntity (NEW) | Live Item with m_extraData | New entity with containedItems, capacity, managed by stockpile |
| RoomManager | InstalledObject (NEW) | Live Item with isConstructed | New generic entity for doors, furniture, alarm bells |
| FarmingManager | InstalledObject (NEW) | Live Item | Same generic entity for troughs, sheds |
| World | Construction record (QVariantMap) | Already stores material info | Store SourceMaterial, stop referencing live items |

### SourceMaterial — universal record for all consumed/installed items

```cpp
struct SourceMaterial {
    QString itemSID;           // "Plank", "Block", "Anvil", etc.
    QString materialSID;       // "Pine", "Iron", etc.
    uint8_t quality;           // preserve craftsmanship quality
    unsigned int spriteID;     // cached at install time, needed for rendering installed items
};
// Stored as flat list — one entry per original item consumed.
// E.g. workshop built from 2 Pine Planks (quality 3 and 5) + 2 Oak Logs:
// [ {"Plank","Pine",3,42}, {"Plank","Pine",5,42}, {"Log","Oak",0,17}, {"Log","Oak",0,17} ]
// On deconstruction, one new item is created per entry.
```

### No separate entity types for most installations

Instead of dedicated InstalledObject / ContainerEntity types, each subsystem stores
a `SourceMaterial` on its existing struct. The SourceMaterial includes spriteID for rendering.

- **Door**: `SourceMaterial` replaces `itemUID + materialUID` on the `Door` struct in RoomManager
- **Furniture**: `SourceMaterial` replaces `furnitureID` on `RoomTile` 
- **Farm utils**: `SourceMaterial` replaces `unsigned int util` on `PastureField`
- **Container on stockpile**: `SourceMaterial` + capacity/requireSame on `InventoryField`
  (container sprite is already on the Tile via constructItem; items are Ground at that position)
- **Mechanism**: `MechanismData` caches `itemSID`, `materialSID`, `spriteID` (already nearly an entity)
  needs `materialSID` at runtime for on/off sprite switching via DB lookup

### Stockpile membership

Stockpile membership remains explicit (not positional). Drawing a stockpile over an existing
item does NOT make it part of the stockpile. Tracked via:
- `InventoryField.items` set (authoritative)
- Reverse index `QHash<uint, uint> m_itemToStockpile` on StockpileManager for O(1) lookup
- The `m_isInStockpile` flag on Item is removed (resolved via reverse index)

### MechanismData — already exists, just needs decoupling

```cpp
struct MechanismData {
    // Already stored:
    unsigned int entityID;     // was itemID
    MechanismType type;
    Position pos;
    int rot;
    bool producePower, consumePower;
    int fuel, maxFuel, refuelThreshold;
    bool active, hasPower;
    QList<Position> connectsTo;
    
    // ADD — cached at install, stop querying inv():
    QString itemSID;
    QString materialSID;
    unsigned int spriteID;
};
```

## Carry Containers — Capacity Modifiers, Not Nested Storage

Carry containers (Wheelbarrow, Bucket, Sack, Backpack) are **equipped tools that expand a gnome's carry capacity**, not items that hold other items.

### Current system (being replaced)
- Wheelbarrow is a required tool on HauleMultipleItems jobs
- Items are put inside the wheelbarrow item via containedItems
- Backpack holds bandages/food/drinks as contained items

### New system
- Gnome has a **base carry capacity of 1**
- Equipping a carry container **adds bonus capacity** for specific item categories
- All carried items use `ItemLocation::Carried` with `locationOwner = creatureID`
- The carry container itself is just an equipped item (`Claim::Equipped`)
- No items-inside-items for anything that moves

### Carry container types

| Container | Category Filter | Capacity | Equip Slot |
|-----------|----------------|----------|------------|
| None | any | 1 (base) | - |
| Wheelbarrow | heavy (logs, planks, blocks, ore) | from DB | tool |
| Bucket | liquids (water, milk) | from DB | tool |
| Sack | small items (fruit, seeds) | from DB | tool |
| Backpack | personal (bandages, food, drinks) | from DB | back |

### Creature carry model

```cpp
// On Creature or Gnome:
int baseCarryCapacity() const { return 1; }

int bonusCarryCapacity() const {
    // check equipped carry container, return its capacity or 0
}

int totalCarryCapacity() const {
    return baseCarryCapacity() + bonusCarryCapacity();
}

int currentlyCarrying() const {
    return m_carriedItems.size();
}

bool canPickUp(unsigned int itemID) const {
    if (currentlyCarrying() >= totalCarryCapacity()) return false;
    // if bonus capacity is from a category-restricted container,
    // check that item matches the allowed category
    // (base capacity of 1 has no category restriction)
    return true;
}
```

### What this eliminates

- `Item::m_extraData->containedItems` for carried containers — gone entirely
- `Item::m_extraData->capacity` for carried containers — gone
- `Item::m_extraData->requireSame` for carried containers — gone
- `putItemInContainer()` / `removeItemFromContainer()` for hauling — gone
- Items-inside-items for anything that moves — gone

`m_extraData` is REMOVED from Item entirely. Container properties only exist on `ContainerEntity` (installed stockpile containers).

### DB columns used
- `Items.CarryContainer` — which carry container type can haul this item (e.g. "Wheelbarrow" for logs)
- `Containers.Capacity` — how many items the container holds
- The category filter can be derived from CarryContainer column: items that list "Bucket" as their CarryContainer are "bucket-compatible"

## Item Data Model

### Location (where the item physically is)

```cpp
enum class ItemLocation : uint8_t {
    Ground,       // loose on a tile, in position index + octree
    Carried,      // held by a creature, NOT in position index
    Container,    // inside a ContainerEntity on a stockpile, NOT directly in position index
};
```

No `Installed` state — installed items are destroyed and become entities.

### Claim (who has reserved the item)

```cpp
enum class ItemClaim : uint8_t {
    None,         // free to be claimed
    Job,          // reserved for a job (claimOwner = jobID)
    Equipped,     // equipped by a creature (claimOwner = creatureID)
};
```

### Ownership struct

```cpp
struct ItemOwnership {
    ItemLocation location = ItemLocation::Ground;
    unsigned int locationOwner = 0;  // creatureID or containerEntityID (0 for Ground)

    ItemClaim claim = ItemClaim::None;
    unsigned int claimOwner = 0;     // jobID or creatureID (0 for None)
};
```

### Container-in-Stockpile hierarchy

An item's stockpile membership is resolved by walking up:
- If item is on Ground → check if tile is a stockpile tile
- If item is in Container → ContainerEntity knows its stockpile

### What gets removed from Item

- `m_isConstructed` — gone, items are destroyed on construction
- `m_isInStockpile` — gone, resolved from container/position chain
- `m_isUsedBy` — gone, merged into Claim::Equipped
- `m_isInJob` — replaced by ItemClaim::Job
- `m_isHeldBy` — replaced by ItemLocation::Carried
- `m_isInContainer` — replaced by ItemLocation::Container
- `m_extraData` — gone entirely. No item holds other items. Container properties live on ContainerEntity.

## Valid Transitions

```
Location transitions:
  Ground    → Carried     (gnome picks up)
  Ground    → Container   (hauled into stockpile container entity)
  Carried   → Ground      (put down)
  Carried   → Container   (gnome puts item in stockpile container)
  Container → Carried     (gnome takes from stockpile container)
  Container → Ground      (removed from container onto tile)

Claim transitions (independent of location):
  None     → Job          (job claims item)
  Job      → None         (job cancelled or finished)
  None     → Equipped     (creature equips weapon/armor/carry container)
  Equipped → None         (creature unequips)

Destruction:
  Any state → destroyed   (consumed, eaten, installed, etc.)

Installation (item → entity):
  Any state → destroyed + entity created in subsystem
  
Deconstruction (entity → item):
  Entity destroyed in subsystem + new item created in Inventory
```

## Index Structure

```cpp
class Inventory {
    // Master storage
    QHash<unsigned int, Item> m_items;

    // Location indices
    QHash<unsigned int, QSet<unsigned int>> m_byPosition;       // tileID → itemIDs (Ground items only)
    QHash<unsigned int, QSet<unsigned int>> m_byLocationOwner;  // creatureID/containerEntityID → itemIDs

    // Claim index
    QHash<unsigned int, QSet<unsigned int>> m_byClaimOwner;     // jobID/creatureID → itemIDs

    // Spatial index for closest-item queries
    QHash<QString, QHash<QString, Octree*>> m_octrees;          // itemSID → matSID → Octree

    // Type index for counting/filtering
    QHash<QString, QHash<QString, QSet<unsigned int>>> m_byType; // itemSID → matSID → itemIDs
};
```

## Transactional API

All state changes go through validated operations:

```cpp
// Location changes
bool pickUpItem(unsigned int itemID, unsigned int creatureID);
bool putDownItem(unsigned int itemID, Position pos);
bool putInContainer(unsigned int itemID, unsigned int containerEntityID);
bool takeFromContainer(unsigned int itemID, unsigned int creatureID);

// Claim changes
bool claimItem(unsigned int itemID, unsigned int jobID);
bool unclaimItem(unsigned int itemID);
bool equipItem(unsigned int itemID, unsigned int creatureID);
bool unequipItem(unsigned int itemID);

// Lifecycle
unsigned int createItem(Position pos, QString itemSID, QString materialSID);
void destroyItem(unsigned int itemID);      // remove from all indices

// Bulk cleanup
void freeAllClaimedBy(unsigned int ownerID);                    // job cancelled
void dropAllCarriedBy(unsigned int creatureID, Position pos);   // creature dies

// Installation bridge — destroys item, returns properties for entity creation
SourceMaterial consumeForInstallation(unsigned int itemID);
// Deconstruction bridge — creates new item from entity properties
unsigned int createFromDeconstruction(Position pos, const SourceMaterial& source);
```

Each operation:
1. Validates the transition
2. Removes from old indices
3. Updates item state
4. Adds to new indices
5. Updates world sprite
6. Emits signal if needed

## Sanity Check (on every load)

```cpp
void Inventory::sanityCheck() {
    // 1. Items in m_byPosition not in m_items → remove from index
    // 2. Ground items not in m_byPosition → add or destroy
    // 3. Items with claim owners pointing to dead jobs/creatures → unclaim
    // 4. Items with location owners pointing to dead creatures/containers → force to Ground
    // 5. Octree rebuild from m_items (cheap, authoritative rebuild)
    // 6. m_byType rebuild from m_items
}

// Each subsystem validates its own entities:
void StockpileManager::sanityCheck() {
    // ContainerEntity contents: items that don't exist → remove from containedItems
    // ContainerEntity on tiles that aren't stockpiles → remove entity, create item
}

void MechanismManager::sanityCheck() {
    // MechanismData with invalid connections → remove connections
}
```

## Implementation Phases

### Phase 1: SourceMaterial + destroy on construction (walls, floors, workshops)
- Add `SourceMaterial` struct
- Workshop stores material specs instead of live item IDs
- Items destroyed after construction (remove `isConstructed` check in canwork.cpp)
- Deconstruction creates fresh items from specs
- Apply to walls, floors, ramps (mostly already work this way)
- `consumeForInstallation()` / `createFromDeconstruction()` API

### Phase 2: InstalledObject entity for simple installations
- Add `InstalledObject` struct
- Doors, furniture, lights, farm utils: destroy item on install, create entity
- Subsystems cache all needed properties at install time
- Deconstruction creates fresh items
- RoomManager, FarmingManager stop querying inv() for installed items

### Phase 3: ContainerEntity
- Add `ContainerEntity` struct to StockpileManager
- Container item destroyed on stockpile installation, entity created
- StockpileManager owns container contents directly
- Items use `ItemLocation::Container` with `locationOwner = containerEntityID`

### Phase 4: Carry container simplification
- Remove items-inside-items for hauling
- Carry containers (Wheelbarrow, Bucket, Sack, Backpack) become pure capacity modifiers
- Gnome base carry capacity = 1, equipped container adds bonus
- All carried items use `ItemLocation::Carried` directly
- Remove `m_extraData` from Item entirely

### Phase 5: MechanismManager decoupling
- Cache `itemSID`, `materialSID`, `spriteID` in MechanismData at install time
- Remove all `g->inv()->*()` calls from MechanismManager
- Mechanism item destroyed on install, entity self-contained

### Phase 6: ItemOwnership state machine
- Add `ItemLocation` + `ItemClaim` enums alongside existing flags (dual-write)
- New transition methods validate and update both old flags and new state
- Log mismatches at runtime

### Phase 7: Add cleanup indices + wire up
- Add `m_byClaimOwner` and `m_byLocationOwner` indices
- Wire `freeAllClaimedBy()` into job cancellation (JobManager::cancelJob)
- Wire `dropAllCarriedBy()` into creature death (Creature::die, Gnome::die)
- Fix position hash corruption bugs (contains() guards)

### Phase 8: Consolidate — remove old flags and API
- Remove `m_isConstructed`, `m_isInStockpile`, `m_isUsedBy`, `m_isInJob`, `m_isHeldBy`, `m_isInContainer`
- All callers use transactional API only
- Comprehensive sanity check on load

### Phase 9: Save format migration
- Serialize `ItemLocation` + `ItemClaim` instead of old flags
- Old saves: drop compatibility (confirmed OK by Ralph)
- Entity serialization in each subsystem

## Open Questions

- [ ] Should quality degrade on deconstruction? (Currently preserving exact quality)
- [ ] What about items in trade? (TradeInventory queries — need to check if trade items have special states)
- [ ] Equipment: gnome carries weapon (Carried + Equipped). On death, should the weapon drop as a free item, or be destroyed? Currently it drops.
- [ ] ID space: should entity IDs share the same counter as item IDs to avoid collisions? (ContainerEntity ID used as locationOwner must not collide with creature IDs used as locationOwner for Carried items)
- [ ] Carry capacity: should base capacity be configurable per creature type? (e.g. automatons carry more)
