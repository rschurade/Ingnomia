from .db import db
from .material import MaterialSet
from .util import empty, log, sort_translations, total_amount, amount_hint


class Store:
    def category_groups(self, cat):
        return sort_translations(
            [
                {
                    "id": group,
                    "translation": self.translate("group", group),
                    "items": self.group_items(cat, group),
                }
                for group in db.item_groups(cat)
            ]
        )

    def categories(self):
        return sort_translations(
            [
                {
                    "id": cat,
                    "translation": self.translate("category", cat),
                    "groups": self.category_groups(cat),
                }
                for cat in db.categories()
            ]
        )

    def categorize(self, items):
        return sort_translations(
            [
                {
                    "id": cat,
                    "translation": self.translate("category", cat),
                    "groups": sort_translations(
                        [
                            {
                                "id": group,
                                "translation": self.translate("group", group),
                                "items": [item for item in items if item["cat"] == cat and item["group"] == group],
                            }
                            for group in set([item["group"] for item in items if item["cat"] == cat])
                        ]
                    ),
                }
                for cat in set([item["cat"] for item in items])
            ]
        )

    def construction(self, id):
        (category, type) = db.construction(id)
        if category is None:
            category = type

        components = self.construction_components(id)
        if len(components) == 0:
            log(f"Empty construction {id}", "! ")
            return None

        matset = components[0]["material_set"]

        return {
            "id": id,
            "translation": self.translate("construction", id),
            "material_set": matset,
            "components_unsorted": components,
            "components": sort_translations(components),
            "category": category,
            "type": type,
        }

    def construction_component(self, itemid, amount, mats, mattypes):
        matset = MaterialSet(mats, mattypes)

        if matset.id == "any":
            (materials, mattypes) = db.item_materials(itemid)
            matset = MaterialSet(materials, mattypes)

        return {
            "id": itemid,
            "translation": self.translate("item", itemid),
            "spriteid": self.item_spriteid(itemid),
            "material_set": matset,
            "amount": int(amount),
        }

    def construction_components(self, id):
        return [self.construction_component(*row) for row in db.construction_components(id)]

    def constructions(self):
        constructions = [c for c in [self.construction(id) for id in db.constructions()] if c is not None]

        return [
            {"category": cat, "constructions": sort_translations([c for c in constructions if c["category"] == cat])}
            for cat in sorted(set([c["category"] for c in constructions]))
        ]

    def container(self, id):
        if empty(id):
            return None

        row = db.container(id)
        if row is None:
            return None

        (capacity, same) = row

        return {
            "id": id,
            "translation": self.translate("item", id),
            "spriteid": self.item_spriteid(id),
            "capacity": int(capacity),
            "same": int(same) == 1,
        }

    def container_items(self, id):
        return self.categorize(
            [
                {
                    "id": item,
                    "cat": cat,
                    "group": group,
                    "translation": self.translate("item", item),
                    "spriteid": self.item_spriteid(item),
                }
                for (item, sprite, cat, group) in db.container_items(id)
            ]
        )

    def craft_component(self, amount, itemid, mats, mattypes):
        matset = MaterialSet(mats, mattypes)
        return {
            "id": itemid,
            "translation": self.translate("item", itemid),
            "spriteid": self.item_spriteid(itemid),
            "material_set": matset,
            "amount": int(amount),
        }

    def craft_components(self, craft):
        return sort_translations([self.craft_component(*row) for row in db.craft_components(craft)])

    def craft_workshops(self, craft):
        return sort_translations(
            [{"id": id, "translation": self.translate("workshop", id)} for id in db.workshops_for_craft(craft)]
        )

    def craft(self, id, itemid, amount, time, skill, rmats, rmattypes, rconv):
        matset = MaterialSet(rmats, rmattypes, rconv)
        return {
            "id": itemid,
            "translation": "none" if itemid is None else self.translate("item", itemid),
            "spriteid": None if itemid is None else self.item_spriteid(itemid),
            "material_set": matset,
            "time": int(time),
            "skill": self.translate("skill", skill),
            "components": self.craft_components(id),
            "workshops": self.craft_workshops(id),
            "amount": int(amount),
        }

    def drinks(self):
        return self.categorize(
            [
                {
                    "id": item,
                    "cat": cat,
                    "group": group,
                    "translation": self.translate("item", item),
                    "spriteid": self.item_spriteid(item),
                    "drinkvalue": drinkval,
                }
                for (item, sprite, cat, group, drinkval) in db.drink_items()
            ]
        )

    def food(self):
        return self.categorize(
            [
                {
                    "id": item,
                    "cat": cat,
                    "group": group,
                    "translation": self.translate("item", item),
                    "spriteid": self.item_spriteid(item),
                    "eatvalue": eatval,
                }
                for (item, sprite, cat, group, eatval) in db.food_items()
            ]
        )

    def group_items(self, cat, group):
        return sort_translations(
            [
                {"id": item, "translation": self.translate("item", item), "spriteid": self.item_spriteid(item)}
                for (item, _, _, _) in db.group_items(cat, group)
            ]
        )

    def item(self, id, cat, group, stack, value, evalue, dvalue, light, mats, mattypes, cnt, ccnt, sprite):
        containerinfo = {}
        if cat == "Containers":
            containerinfo = self.container(id)
            containerinfo["can_contain"] = self.container_items(id)

        (materials, material_tree) = self.material_tree(mats, mattypes)

        return {
            "id": id,
            "translation": self.translate("item", id),
            "group": {
                "id": group,
                "translation": self.translate("group", group),
            },
            "category": {
                "id": cat,
                "translation": self.translate("category", cat),
            },
            "stack": stack,
            "container": self.container(cnt),
            "carry": self.container(ccnt),
            "spriteid": self.item_spriteid(id),
            "materials": materials,
            "material_tree": material_tree,
            "value": value,
            "evalue": evalue,
            "dvalue": dvalue,
            "light": light,
            "crafts": self.item_crafts(id),
            "rcrafts": self.item_rcrafts(id),
            "wcrafts": self.item_wcrafts(id),
            "constructions": self.item_constructions(id),
            "rconstructions": self.item_rconstructions(id),
            **containerinfo,
        }

    def item_constructions(self, item):
        constructions = [self.construction(id) for id in db.constructions_for_item(item)]

        return [
            {"category": cat, "constructions": sort_translations([c for c in constructions if c["category"] == cat])}
            for cat in sorted(set([c["category"] for c in constructions]))
        ]

    def item_rconstructions(self, item):
        constructions = [self.construction(id) for id in db.constructions_using_item(item)]

        return [
            {"category": cat, "constructions": sort_translations([c for c in constructions if c["category"] == cat])}
            for cat in sorted(set([c["category"] for c in constructions]))
        ]

    def item_crafts(self, item):
        return [
            self.craft(id, item, amount, time, skill, rmats, rmattypes, rconv)
            for (id, amount, time, skill, rmats, rmattypes, rconv) in db.item_crafts(item)
        ]

    def item_rcraft(self, craft):
        row = db.craft(craft)
        return None if row is None else self.craft(*row)

    def item_rcrafts(self, item):
        return list(filter(lambda c: c is not None, [self.item_rcraft(craft) for craft in db.crafts_using_item(item)]))

    def item_wcrafts(self, item):
        return [{"id": id, "translation": self.translate("workshop", id)} for id in db.workshops_using_item(item)]

    def item_spriteid(self, item):
        sprite = db.item_sprite(item)
        return item if sprite is None else sprite

    def items(self):
        return sort_translations([self.item(*row) for row in db.items()])

    def materials(self, mats, mattypes):
        return sort_translations(
            [{"id": id, "translation": self.translate("material", id)} for id in db.materials_flat(mats, mattypes)]
        )

    def material_tree(self, mats, mattypes):
        mats = [] if mats is None else mats.split("|")
        mattypes = [] if mattypes is None else mattypes.split("|")

        tree = sort_translations(
            [
                *(
                    []
                    if len(mats) == 0
                    else [
                        {
                            "id": "Other",
                            "materials": sort_translations(
                                [{"id": mat, "translation": self.translate("material", mat)} for mat in mats]
                            ),
                        }
                    ]
                ),
                *[
                    {
                        "id": type,
                        "materials": sort_translations(
                            [
                                {"id": mat, "translation": self.translate("material", mat)}
                                for mat in db.materials_of_type(type)
                            ]
                        ),
                    }
                    for type in mattypes
                ],
            ]
        )

        return ([*mats, *[mat for type in mattypes for mat in db.materials_of_type(type)]], tree)

    def plant_produce(self, chance, itemid, materialid, fruits_per_season=0):
        matset = MaterialSet(materialid, None)

        if fruits_per_season == 0:
            amount = total_amount(chance)
            hint = amount_hint(chance)
        else:
            amount = int(fruits_per_season)
            hint = "Per season"

        return {
            "id": itemid,
            "translation": self.translate("item", itemid),
            "spriteid": self.item_spriteid(itemid),
            "material_set": matset,
            "amount": amount,
            "amount_hint": hint,
            "type": "harvest",
        }

    def plant_fell(self, plantid, itemid, materialid, random):
        matset = MaterialSet(materialid, None)

        if random > 0:
            amount = f"1-{int(random)}"
        else:
            layout = list(db.tree_layout(plantid))
            amount = 1 if len(layout) == 0 else len(layout)

        return {
            "id": itemid,
            "translation": self.translate("item", itemid),
            "spriteid": self.item_spriteid(itemid),
            "material_set": matset,
            "amount": amount,
            "type": "fell",
        }

    def plant_produces(self, id, fruits_per_season=0):
        return [
            *[self.plant_produce(*row, fruits_per_season) for row in db.plant_produces(id)],
            *[self.plant_fell(id, *row) for row in db.plant_fell(id)],
        ]

    def plant(self, id, growsin, growseason, killseason, loseseason, material, fruits):
        harvest_action = db.plant_harvest_action(id)
        fruits_per_season = 0
        if harvest_action == "ReduceFruitCount":
            fruits_per_season = fruits

        return {
            "id": id,
            "sprites": db.plant_sprites(id),
            "material": material,
            "growsin": growsin,
            "growseason": [] if growseason is None else growseason.split("|"),
            "killseason": [] if killseason is None else killseason.split("|"),
            "loseseason": [] if loseseason is None else loseseason.split("|"),
            "harvestproduces": self.plant_produces(id, fruits_per_season),
        }

    def plants(self):
        floors = {"Tree": "grass", "Mushroom": "grass"}

        return [
            {
                "type": type,
                "floor": floors.get(type, "farm"),
                "plants": sort_translations([self.plant(*row) for row in db.plants(type)]),
            }
            for type in db.plant_types()
        ]

    def tints(self):
        return [
            {
                "id": id,
                "r": int(color.split(" ")[0]),
                "g": int(color.split(" ")[1]),
                "b": int(color.split(" ")[2]),
                "a": int(color.split(" ")[3]),
            }
            for (id, color) in db.materials()
        ]

    def translate(self, type, id):
        text = db.translation(f"${type.capitalize()}Name_{id}")
        return id if text is None else text

    def workshop_components(self, id):
        return sort_translations(
            [
                {
                    "id": itemid,
                    "translation": self.translate("item", itemid),
                    "spriteid": self.item_spriteid(itemid),
                    "amount": int(amount),
                    "workshops": sort_translations(
                        map(
                            lambda w: {"id": w, "translation": self.translate("workshop", w)},
                            set(
                                [
                                    wid
                                    for (craftid, amount, time, skill, _, _, _) in db.item_crafts(itemid)
                                    for wid in db.workshops_for_craft(craftid)
                                    if id != wid
                                ],
                            ),
                        )
                    ),
                }
                for (itemid, amount) in db.workshop_components(id)
            ]
        )

    def workshop_craft(self, craft):
        row = db.craft(craft)
        return None if row is None else self.craft(*row)

    def workshop_crafts(self, crafts):
        if crafts is None:
            return []

        return sort_translations(
            filter(
                lambda c: c is not None,
                [self.workshop_craft(craft) for craft in crafts.split("|")],
            )
        )

    def workshops(self):
        return sort_translations(
            [
                {
                    "id": id,
                    "translation": self.translate("workshop", id),
                    "crafts": self.workshop_crafts(crafts),
                    "components": self.workshop_components(id),
                    "tab": tab,
                }
                for (id, crafts, tab) in db.workshops()
            ]
        )


store = Store()
