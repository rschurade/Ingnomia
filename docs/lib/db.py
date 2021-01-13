import os
import sqlite3

from .util import DOCDIR, empty, log

DB_PATH = os.path.join(DOCDIR, "..", "content", "db", "ingnomia.db.sql")


class Database:
    def __init__(self):
        self.conn = sqlite3.connect(":memory:", uri=True)
        with open(DB_PATH) as f:
            log("Loading database")
            self.conn.executescript(f.read())

    def basesprite(self, id):
        for row in self.conn.execute("SELECT sourcerectangle, tilesheet FROM BaseSprites WHERE id = ?", (id,)):
            return row
        return None

    def basesprites(self):
        return [id for (id,) in self.conn.execute("SELECT id FROM BaseSprites")]

    def categories(self):
        return [cat for (cat,) in self.conn.execute("SELECT DISTINCT category FROM Items")]

    def construction(self, id):
        for row in self.conn.execute("SELECT category, type FROM Constructions WHERE id = ?", (id,)):
            return row
        return None

    def construction_components(self, id):
        return self.conn.execute(
            "SELECT itemid, amount, materials, materialtypes FROM Constructions_Components WHERE id = ?", (id,)
        )

    def construction_sprites(self, id):
        return self.conn.execute(
            "SELECT COALESCE(spriteidoverride, spriteid), offset FROM Constructions_Sprites WHERE id = ?",
            (id,),
        )

    def constructions(self):
        return [id for (id,) in self.conn.execute("SELECT id FROM Constructions")]

    def constructions_for_item(self, item):
        return [
            id for (id,) in self.conn.execute("SELECT id FROM Constructions WHERE type = 'Item' AND id = ?", (item,))
        ]

    def constructions_using_item(self, item):
        return [id for (id,) in self.conn.execute("SELECT id FROM Constructions_Components WHERE itemid = ?", (item,))]

    def container(self, id):
        for row in self.conn.execute("SELECT capacity, requiresame FROM Containers WHERE id = ?", (id,)):
            return row
        return None

    def container_items(self, id):
        return self.conn.execute(
            """SELECT id, spriteid, category, itemgroup
               FROM Items
               WHERE '|' || allowedcontainers || '|' LIKE '%|' || ? || '|%'
                  OR '|' || carrycontainer || '|' LIKE '%|' || ? || '|%'""",
            (id, id),
        )

    def craft(self, craft):
        for row in self.conn.execute(
            """SELECT id, itemid, amount, productiontime, skillid, resultmaterial, resultmaterialtypes, conversionmaterial
               FROM Crafts c
               WHERE id = ?
                 AND (amount = 0 OR EXISTS (SELECT 1 FROM Items i WHERE i.id = c.itemid))""",
            (craft,),
        ):
            return row
        return None

    def craft_components(self, craft):
        return self.conn.execute(
            "SELECT amount, itemid, allowedmaterial, allowedmaterialtype FROM Crafts_Components WHERE id = ?",
            (craft,),
        )

    def crafts_using_item(self, item):
        return [
            id for (id,) in self.conn.execute("SELECT DISTINCT id FROM Crafts_Components WHERE itemid = ?", (item,))
        ]

    def drink_items(self):
        return self.conn.execute(
            """SELECT id, spriteid, category, itemgroup, drinkvalue
               FROM Items
               WHERE drinkvalue > 0"""
        )

    def food_items(self):
        return self.conn.execute(
            """SELECT id, spriteid, category, itemgroup, eatvalue
               FROM Items
               WHERE eatvalue > 0"""
        )

    def group_items(self, cat, group):
        return self.conn.execute(
            "SELECT id, spriteid, allowedmaterials, allowedmaterialtypes FROM Items WHERE category = ? AND itemgroup = ?",
            (cat, group),
        )

    def item_crafts(self, item):
        return self.conn.execute(
            "SELECT id, amount, productiontime, skillid, resultmaterial, resultmaterialtypes, conversionmaterial FROM Crafts WHERE itemid = ?",
            (item,),
        )

    def item_groups(self, cat):
        return [
            group
            for (group,) in self.conn.execute(
                "SELECT DISTINCT itemgroup FROM Items WHERE category = ?",
                (cat,),
            )
        ]

    def item_materials(self, item):
        for (mat, mattypes) in self.conn.execute(
            "SELECT allowedmaterials, allowedmaterialtypes FROM Items WHERE id = ?", (item,)
        ):
            return (mat, mattypes)
        return (None, None)

    def item_sprite(self, item):
        for (sprite,) in self.conn.execute("SELECT spriteid FROM Items WHERE id = ?", (item,)):
            return sprite
        return None

    def items(self):
        return self.conn.execute(
            """SELECT id, category, itemgroup, stacksize, value, eatvalue, drinkvalue, lightintensity,
                      allowedmaterials, allowedmaterialtypes, allowedcontainers, carrycontainer, spriteid
               FROM Items"""
        )

    def items_craftable_with(self, items, workshops):
        values = [*workshops, *items]
        wparams = ",".join(map(lambda x: "?", workshops))
        iparams = ",".join(map(lambda x: "?", items))

        return [
            id
            for (id,) in self.conn.execute(
                f"""SELECT DISTINCT c.itemid FROM Crafts c
                    WHERE EXISTS (
                        SELECT 1 FROM Workshops w
                        WHERE w.id in ({wparams})
                        AND '|' || w.crafts || '|' LIKE '%|' || c.id || '|%'
                    ) AND NOT EXISTS (
                        SELECT 1 FROM Crafts_Components cc
                        WHERE cc.id = c.id
                        AND cc.itemid NOT IN ({iparams})
                    )""",
                values,
            )
        ]

    def materials(self):
        return self.conn.execute("SELECT id, color FROM Materials")

    def materials_of_type(self, type):
        return [mat for (mat,) in self.conn.execute("SELECT id FROM Materials WHERE type = ?", (type,))]

    def materials_flat(self, mats=None, mattypes=None):
        mats = [] if empty(mats) else mats.split("|")
        mattypes = [] if empty(mattypes) else mattypes.split("|")
        return [*mats, *[m for t in mattypes for m in self.materials_of_type(t)]]

    def plant_harvest_action(self, id):
        for (action,) in self.conn.execute("SELECT action FROM Plants_OnHarvest WHERE id = ?", (id,)):
            return action
        return None

    def plant_produces(self, id):
        return self.conn.execute(
            """SELECT GROUP_CONCAT(chance, '|'), itemid, materialid
               FROM Plants_OnHarvest_HarvestedItem
               WHERE id = ?
               GROUP BY itemid, materialid""",
            (id,),
        )

    def plant_fell(self, id):
        return self.conn.execute("SELECT itemid, materialid, random FROM Plants_OnFell WHERE id = ?", (id,))

    def plant_types(self):
        return [type for (type,) in self.conn.execute("SELECT DISTINCT type FROM Plants")]

    def plant_sprites(self, id):
        return [
            (spriteid, layout)
            for (spriteid, layout) in self.conn.execute(
                "SELECT DISTINCT spriteid, layout FROM Plants_States WHERE id = ?", (id,)
            )
        ]

    def plants(self, type):
        return self.conn.execute(
            "SELECT id, growsin, growsinseason, iskilledinseason, losesfruitinseason, material, numfruitsperseason FROM Plants WHERE type = ?",
            (type,),
        )

    def sprite(self, id):
        # Keep last row only
        row = None
        for r in self.conn.execute("SELECT offset, basesprite, tint FROM Sprites WHERE id = ?", (id,)):
            row = r
        return row

    def sprite_by_material(self, id):
        return self.conn.execute(
            "SELECT basesprite, materialid, sprite, effect FROM Sprites_ByMaterials WHERE id = ?",
            (id,),
        )

    def sprite_by_material_type(self, id):
        return self.conn.execute(
            """SELECT bmt.basesprite, m.id, bmt.sprite
               FROM Sprites_ByMaterialTypes bmt
               JOIN Materials m ON m.type = bmt.materialtype
               WHERE bmt.id = ?""",
            (id,),
        )

    def sprite_combine(self, id):
        return self.conn.execute(
            "SELECT basesprite, offset, sprite, tint FROM Sprites_Combine WHERE id = ?",
            (id,),
        )

    def sprite_frames(self, id):
        return self.conn.execute("SELECT DISTINCT basesprite FROM Sprites_Frames WHERE id = ?", (id,))

    def sprite_random(self, id):
        return self.conn.execute("SELECT basesprite, sprite FROM Sprites_Random WHERE id = ?", (id,))

    def sprite_rotations(self, id):
        return self.conn.execute(
            "SELECT basesprite, sprite, rotation, effect FROM Sprites_Rotations WHERE id = ?", (id,)
        )

    def sprite_seasons(self, id):
        return self.conn.execute("SELECT basesprite, season FROM Sprites_Seasons WHERE id = ?", (id,))

    def sprite_seasons_rotations(self, id, season):
        return self.conn.execute(
            "SELECT basesprite, rotation FROM Sprites_Seasons_Rotations WHERE id = ?", (f"{id}{season}",)
        )

    def sprites(self):
        return [id for (id,) in self.conn.execute("SELECT id FROM Sprites")]

    def translation(self, key):
        for (text,) in self.conn.execute(
            "SELECT text FROM Translation WHERE id = ?",
            (key,),
        ):
            return text

    def tree_layout(self, id):
        return self.conn.execute(
            "SELECT offset, rotation, spriteid, fruitpos FROM TreeLayouts_Layout WHERE id = ?", (id,)
        )

    def uncraftable_items(self):
        return [id for (id,) in self.conn.execute("SELECT id FROM Items EXCEPT SELECT itemid FROM Crafts")]

    def workshop_components(self, id):
        return self.conn.execute(
            "SELECT itemid, amount FROM Workshops_Components WHERE id = ? AND amount > 0",
            (id,),
        )

    def workshop_requires(self, id):
        return

    def workshops(self):
        return self.conn.execute("SELECT id, crafts, tab FROM Workshops")

    def workshops_craftable_with(self, itemids):
        params = ", ".join(map(lambda x: "?", itemids))
        return [
            id
            for (id,) in self.conn.execute(
                f"""SELECT w.id FROM Workshops w
                    WHERE NOT EXISTS (
                        SELECT 1
                        FROM Workshops_Components c
                        WHERE c.id = w.id
                          AND c.amount > 0
                          AND c.itemid NOT IN ({params})
                    )""",
                tuple(itemids),
            )
        ]

    def workshops_for_craft(self, craft):
        return [
            id
            for (id,) in self.conn.execute(
                "SELECT id FROM Workshops WHERE '|' || crafts || '|' LIKE '%|' || ? ||  '|%'",
                (craft,),
            )
        ]

    def workshop_sprites(self, id):
        return self.conn.execute(
            """SELECT spriteid, offset, wallrotation, itemid, materialitem, amount, isfloor
               FROM Workshops_Components
               WHERE id = ?""",
            (id,),
        )

    def workshops_using_item(self, item):
        return [
            id
            for (id,) in self.conn.execute(
                "SELECT DISTINCT id FROM Workshops_Components WHERE itemid = ?",
                (item,),
            )
        ]


db = Database()
