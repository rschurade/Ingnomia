import random

from ..db import db
from ..util import empty, sprite_offset3d
from .sprite import EmptySprite, Sprite, SpriteManager


def iso_projectx(x, y, z):
    return int((x - y) * 16)


def iso_projecty(x, y, z):
    return int((x + y) * 8 - z * 20)


def iso_project(pos):
    (x, y, z) = (pos["x"], pos["y"], pos["z"])
    return {"x": iso_projectx(x, y, z), "y": iso_projecty(x, y, z)}


class Layout:
    @classmethod
    def sprite(self, id):
        return Layout(
            [
                LayoutCell({"x": 0, "y": 0, "z": 0}, EmptySprite(), overrides={"is_floor": True}),
                LayoutCell({"x": 0, "y": 0, "z": 0}, id),
            ]
        )

    @classmethod
    def construction(self, id):
        sprites = list(db.construction_sprites(id))
        if len(sprites) == 0:
            sprites = [(id, "0 0 0")]

        return Layout(
            [
                LayoutCell({"x": 0, "y": 0, "z": 0}, EmptySprite(), overrides={"is_floor": True}),
                *[LayoutCell(sprite_offset3d(offset), spriteid) for (spriteid, offset) in sprites],
            ]
        )

    @classmethod
    def workshop(self, id):
        sprites = list(db.workshop_sprites(id))
        if len(sprites) == 0:
            raise ValueError(f"No sprites for workshop {id}")

        item_materials = {}
        input_materials = []
        for (_, _, _, itemid, _, amount, _) in sprites:
            if not empty(itemid) and amount > 0:
                if itemid not in item_materials:
                    (mats, mattypes) = db.item_materials(itemid)
                    item_materials[itemid] = sorted(db.materials_flat(mats, mattypes))

                for _ in range(int(amount)):
                    input_materials.append(item_materials[itemid])

        cells = []
        for (spriteid, offset, rotation, itemid, materialitem, _, isfloor) in sprites:
            pos = sprite_offset3d(offset)
            if pos["z"] == 0 and empty(isfloor):
                cells.append(LayoutCell(pos, EmptySprite(), overrides={"is_floor": True}))

            if empty(spriteid) or spriteid == "WorkshopInputIndicator":
                cells.append(LayoutCell(pos, EmptySprite()))
            else:
                overrides = {}

                if not empty(rotation):
                    overrides["rotation"] = rotation

                if not empty(itemid):
                    overrides["materials"] = item_materials[itemid]
                elif not empty(materialitem):
                    indices = [int(i) for i in materialitem.split("|")]
                    if len(indices) == 1:
                        overrides["materials"] = input_materials[indices[0]]
                    else:
                        overrides["multi_materials"] = [input_materials[i] for i in indices]

                cells.append(LayoutCell(pos, spriteid, overrides=overrides))

        return Layout(cells)

    @classmethod
    def tree(self, layoutid, large_floor=False):
        cells = []
        layout = list(db.tree_layout(layoutid))

        floorpos = set([(0, 0)])
        if large_floor and any([not offset.startswith("0 0") for (offset, _, _, _) in layout]):
            floorpos = [(x, y) for x in range(-1, 2) for y in range(-1, 2)]

        for (offset, rotation, spriteid, _) in layout:
            pos = sprite_offset3d(offset)

            if pos["z"] == 0:
                for x, y in floorpos:
                    cells.append(LayoutCell({"x": x, "y": y, "z": 0}, EmptySprite(), overrides={"is_floor": True}))

            overrides = {"random": "first"}
            if not empty(rotation):
                overrides["rotation"] = rotation

            cells.append(
                LayoutCell(
                    pos,
                    spriteid,
                    overrides=overrides,
                )
            )

        return Layout(cells)

    def __init__(self, cells):
        # 3D AABB
        axes = ["x", "y", "z"]
        mins = {axis: min([c.pos[axis] for c in cells]) for axis in axes}
        maxes = {axis: max([c.pos[axis] for c in cells]) for axis in axes}
        sizes = {axis: maxes[axis] - mins[axis] + 1 for axis in axes}

        # Project to 2D bbox
        mins2d = {
            "x": min(iso_projectx(c.pos["x"], c.pos["y"] + 1, c.pos["z"]) for c in cells),
            "y": min(iso_projecty(c.pos["x"], c.pos["y"], c.pos["z"] + 1) for c in cells),
        }

        maxes2d = {
            "x": max(iso_projectx(c.pos["x"] + 1, c.pos["y"], c.pos["z"]) for c in cells),
            "y": max(iso_projecty(c.pos["x"] + 1, c.pos["y"] + 1, c.pos["z"]) for c in cells),
        }

        self.bbox = {
            "width": maxes2d["x"] - mins2d["x"],
            "height": maxes2d["y"] - mins2d["y"],
            "minx": mins2d["x"] + 16,
            "miny": mins2d["y"] + 20,
        }

        # Sort cells for correct drawing order
        self.cells = sorted(
            sorted(sorted(cells, key=lambda c: c.pos["x"]), key=lambda c: c.pos["y"]), key=lambda c: c.pos["z"]
        )

    def set_floor(self, spriteid, tint=None, random=None, season=None, materials=None):
        sprite = SpriteManager.get(spriteid)

        overrides = {"is_floor": True}

        if tint is not None:
            overrides["materials"] = [tint]
        elif materials is not None:
            overrides["materials"] = materials

        if random is not None:
            overrides["random"] = random

        if season is not None:
            overrides["season"] = season

        for index in [self.cells.index(c) for c in self.cells if c.overrides.get("is_floor") == True]:
            self.cells[index] = LayoutCell(self.cells[index].pos, sprite, overrides=overrides)

    def select(self, **kwargs):
        return {"bbox": self.bbox, "cells": [c.select(**kwargs) for c in self.cells]}


class LayoutCell:
    def __init__(self, pos, sprite, overrides={}):
        self.sprite = sprite if isinstance(sprite, Sprite) else SpriteManager.get(sprite)
        self.pos = pos
        self.projected = iso_project(pos)
        self.overrides = overrides

    def select(self, **kwargs):
        if "materials" in self.overrides:
            kwargs["materials"] = self.overrides["materials"]

        if "multi_materials" in self.overrides:
            kwargs["multi_materials"] = self.overrides["multi_materials"]

        if "rotation" in self.overrides:
            kwargs["rotation"] = self.overrides["rotation"]

        if "random" in self.overrides:
            kwargs["random"] = self.overrides["random"]

        if "season" in self.overrides:
            kwargs["season"] = self.overrides["season"]

        return {"pos": self.pos, "projected": self.projected, "layers": self.sprite.select(**kwargs)}
