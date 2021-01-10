from .db import db
from .util import empty


class MaterialSet:
    colors = {id: color for (id, color) in db.materials()}

    def __init__(self, materials, types, conversion=None):
        if not isinstance(materials, list):
            materials = [] if empty(materials) else materials.split("|")

        if not isinstance(types, list):
            types = [] if empty(types) else types.split("|")

        if "RandomMetal" in materials:
            materials.remove("RandomMetal")
            types.append("Metal")

        types_materials = [m for t in types for m in db.materials_of_type(t)]

        if not empty(conversion) and conversion in materials or conversion in types_materials:
            materials = [conversion]
            types = []
            types_materials = []

        self.all_materials = sorted(set([*materials, *types_materials]))
        self.labels = [
            *[f"any {t}" for t in sorted(types)],
            *sorted([db.translation(f"$MaterialName_{m}") for m in materials]),
        ]

        idparts = [*[f"M{m}" for m in sorted(materials)], *[f"T{t}" for t in sorted(types)]]

        if len(idparts):
            self.id = "".join(idparts)
        else:
            self.id = "any"

        unique_colors = set()
        unique_materials = set()
        for m in self.all_materials:
            color = MaterialSet.colors[m]
            if color not in unique_colors:
                unique_colors.add(color)
                unique_materials.add(m)

        self.unique_tints = sorted(unique_materials)

    def covers(self, itemid):
        (mats, mattypes) = db.item_materials(itemid)
        all_materials = set(db.materials_flat(mats, mattypes))

        return all_materials == set(self.all_materials)

    def __eq__(self, other):
        return other is not None and self.id == other.id

    def __repr__(self):
        return repr({"type": "MaterialSet", "id": self.id})
