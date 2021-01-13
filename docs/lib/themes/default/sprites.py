from collections import defaultdict

from ...sprites import SpriteManager, MissingMaterials, Layout
from ...util import empty, log


class Animation:
    anims = []

    @classmethod
    def get(cls, id, frames):
        for anim in cls.anims:
            if anim.has_frames(frames):
                anim.ids.append(id)
                return anim
        anim = cls(id, frames)
        cls.anims.append(anim)
        return anim

    def has_frames(self, frames):
        return frames == self.frames

    def __init__(self, id, frames):
        self.ids = [id]
        self.frames = frames

    def id(self):
        return f"anim_{Animation.anims.index(self)}"


class StyleRule:
    rules = []

    @classmethod
    def set(cls, selectors, props):
        for rule in cls.rules:
            if rule.has_props(props):
                rule.selectors |= set(selectors)
                return rule
        rule = cls(selectors, props)
        cls.rules.append(rule)
        return rule

    def __init__(self, selectors, props={}):
        self.selectors = set(selectors)
        self.props = props

    def has_props(self, props):
        return props == self.props


def sprite_style(alt, common={}):
    props = {}

    if "tilesheet" not in common:
        props["background-image"] = f"url({alt.tilesheet})"

    if "rectx" not in common or "recty" not in common:
        props["background-position"] = f"-{alt.rect['x']}px -{alt.rect['y']}px"

    if "rectw" not in common:
        props["width"] = f"{alt.rect['w']}px"

    if "recth" not in common:
        props["height"] = f"{alt.rect['h']}px"

    if "ox" not in common and alt.ox != 0:
        props["left"] = f"{alt.ox}px"

    if "oy" not in common and alt.oy != 0:
        props["top"] = f"{alt.oy}px"

    if alt.material:
        props["filter"] = f"url(tints.svg#{alt.material})"

    if "effects" not in common and len(alt.effects) > 0:
        transforms = []

        for effect in alt.effects:
            if effect == "Rot90":
                transforms.append("rotate(-90deg)")
            elif effect == "FlipHorizontal":
                transforms.append("rotateY(180deg)")

        props["transform"] = " ".join(transforms)

    return props


def common_style(common):
    props = {}

    if "tilesheet" in common:
        props["background-image"] = f"url({common['tilesheet']})"

    if "rectx" in common and "recty" in common:
        props["background-position"] = f"-{common['rectx']}px -{common['recty']}px"

    if "rectw" in common:
        props["width"] = f"{common['rectw']}px"

    if "recth" in common:
        props["height"] = f"{common['recth']}px"

    if "ox" in common and common["ox"] != 0:
        props["left"] = f"{common['ox']}px"

    if "oy" in common and common["oy"] != 0:
        props["top"] = f"{common['oy']}px"

    if "effect" in common and len(common["effects"]) > 0:
        transforms = []

        for effect in common["effects"]:
            if effect == "Rot90":
                transforms.append("rotate(-90deg)")
            elif effect == "FlipHorizontal":
                transforms.append("rotateY(180deg)")

        props["transform"] = " ".join(transforms)

    return props


class StyleManager:
    item_sprites = {}
    plant_sprites = {}
    construction_sprites = {}
    workshop_sprites = {}
    menu_sprites = {}

    warnings = set()
    tilesheets = set()

    select_args = {"random": "all", "frames": "all"}

    floors = defaultdict(
        lambda: {"id": "SolidSelectionFloor", "material": "None"},
        farm={"id": "TilledSoil", "material": "Dirt"},
        grass={"id": "Grass", "material": "None", "season": "Summer"},
        mushroom={"id": "MushroomGrass", "material": "None"},
        logs={"id": "LogFloor", "material": "Pine"},
    )

    menu = {
        "Items": {
            "source": lambda: Layout.sprite("Crate"),
            "args": {"materials": ["Oak"]},
        },
        "Food": {"source": lambda: Layout.sprite("ShepherdsPie"), "floor": "logs"},
        "Plants": {
            "source": lambda: Layout.sprite("PlantLargeWithFruit"),
            "args": {"materials": ["Tomato"]},
            "floor": "farm",
        },
        "Constructions": {
            "source": lambda: Layout.construction("BlockStairs"),
            "args": {"materials": ["Marble"]},
        },
        "Workshops": {
            "source": lambda: Layout.sprite("Workbench"),
            "args": {"materials": ["Pine"]},
            "floor": "grass",
        },
    }

    @classmethod
    def set_floor(cls, layout, id):
        floor = cls.floors[id]
        layout.set_floor(
            floor["id"],
            tint=floor["material"],
            random="random",
            season=None if "season" not in floor else floor["season"],
        )

    @classmethod
    def create_item_sprite(cls, item, floor="item"):
        (id, spriteid, materials) = (item["id"], item["spriteid"], item["materials"])

        layout = Layout.sprite(spriteid)
        cls.set_floor(layout, floor)

        sprite_data = {"layout": layout, "materials": {}}

        try:
            try:
                sprite_data["materials"]["any"] = layout.select(materials=sorted(materials), **cls.select_args)
            except MissingMaterials as mm:
                cls.warnings.add(mm.args[0])
                present = [m for m in materials if m not in mm.materials]
                sprite_data["materials"]["any"] = layout.select(materials=sorted(present), **cls.select_args)
        except Exception as ex:
            cls.warnings.add(ex.args[0])

        for mat in materials:
            try:
                sprite_data["materials"][f"M{mat}"] = layout.select(materials=[mat], **cls.select_args)
            except Exception as ex:
                cls.warnings.add(ex.args[0])

        cls.item_sprites[id] = sprite_data

    @classmethod
    def add_item_materialset(cls, item, matset):
        sprite_data = cls.item_sprites[item]
        (layout, materials) = (sprite_data["layout"], sprite_data["materials"])

        if matset.id not in materials:
            try:
                try:
                    materials[matset.id] = layout.select(materials=matset.all_materials, **cls.select_args)
                except MissingMaterials as mm:
                    cls.warnings.add(mm.args[0])
                    present = [m for m in matset.all_materials if m not in mm.materials]
                    materials[matset.id] = layout.select(materials=present, **cls.select_args)
            except Exception as ex:
                cls.warnings.add(ex.args[0])

    @classmethod
    def create_plant_sprite(cls, plant, floor="farm"):
        (id, sprites, material) = (plant["id"], plant["sprites"], plant["material"])

        tree_layout = set(layout for (_, layout) in sprites if not empty(layout))
        if len(tree_layout):
            layout = Layout.tree(tree_layout.pop(), large_floor=True)
        else:
            layout = Layout.sprite(SpriteManager.random(f"plant-{id}", [sprite for (sprite, _) in sprites]))

        cls.set_floor(layout, floor)

        try:
            cls.plant_sprites[id] = {"materials": {"any": layout.select(materials=[material], **cls.select_args)}}
        except Exception as ex:
            cls.warnings.add(ex.args[0])

    @classmethod
    def create_construction_sprite(cls, construction, floor="construction"):
        (id, matset) = (construction["id"], construction["material_set"])

        try:
            layout = Layout.construction(id)
        except Exception as ex:
            cls.warnings.add(ex.args[0])
            return

        cls.set_floor(layout, floor)

        sprite_data = {"layout": layout, "materials": {}}
        select_args = {**cls.select_args}

        if len(construction["components"]) == 1:
            select_args["materials"] = matset.all_materials
        else:
            select_args["multi_materials"] = [
                c["material_set"].all_materials for c in construction["components_unsorted"]
            ]

        try:
            try:
                sprite_data["materials"]["any"] = layout.select(**select_args)
            except MissingMaterials as mm:
                cls.warnings.add(mm.args[0])
                select_args["materials"] = sorted([m for m in select_args["materials"] if m not in mm.materials])
                sprite_data["materials"]["any"] = layout.select(**select_args)
        except Exception as ex:
            cls.warnings.add(ex.args[0])
            return

        cls.construction_sprites[id] = sprite_data

    @classmethod
    def create_workshop_sprite(cls, workshop, floor="grass"):
        id = workshop["id"]

        try:
            layout = Layout.workshop(id)
        except Exception as ex:
            cls.warnings.add(ex.args[0])
            return

        cls.set_floor(layout, floor)

        try:
            cls.workshop_sprites[id] = {"materials": {"any": layout.select(**cls.select_args)}}
        except Exception as ex:
            cls.warnings.add(ex.args[0])

    @classmethod
    def create_menu_sprite(cls, item):
        id = item["label"]
        if id not in cls.menu:
            return

        sdef = cls.menu[id]
        layout = sdef["source"]()

        cls.set_floor(layout, sdef.get("floor", "default"))
        cls.menu_sprites[id] = {"layout": layout, "materials": {"any": layout.select(**(sdef.get("args", {})))}}

    @classmethod
    def styles(cls):
        """Generate style rules and keyframes, and deduplicate them"""

        all_sprites = {
            **{f"i-{k}": v for k, v in cls.item_sprites.items()},
            **{f"p-{k}": v for k, v in cls.plant_sprites.items()},
            **{f"k-{k}": v for k, v in cls.construction_sprites.items()},
            **{f"w-{k}": v for k, v in cls.workshop_sprites.items()},
            **{f"m-{k}": v for k, v in cls.menu_sprites.items()},
        }

        for classname, sprite_data in all_sprites.items():
            for material, rendered in sprite_data["materials"].items():
                (bbox, cells) = (rendered["bbox"], rendered["cells"])

                sprite_data["rule"] = StyleRule.set(
                    [f".l-{bbox['width']}-{bbox['height']}"],
                    {"width": f"{bbox['width']}px", "height": f"{bbox['height']}px"},
                )

                StyleRule.set(
                    [f".l-{bbox['width']}-{bbox['height']}.zoomed"],
                    {"width": f"{bbox['width'] * 2}px", "height": f"{bbox['height'] * 2}px"},
                )

                for cell in cells:
                    (x, y, z) = (cell["pos"]["x"], cell["pos"]["y"], cell["pos"]["z"])
                    (left, top) = (
                        cell["projected"]["x"] - bbox["minx"],
                        cell["projected"]["y"] - bbox["miny"],
                    )

                    StyleRule.set(
                        [f".l-{bbox['width']}-{bbox['height']} .cp-{x}-{y}-{z}"],
                        {"left": f"{left}px", "top": f"{top}px", "--minx": bbox["minx"], "--miny": bbox["miny"]},
                    )

                    cell_number = cells.index(cell) + 1
                    layers = cell["layers"].layers

                    for layer in layers:
                        alternatives = layer.unique(use_color=True)
                        alt_count = len(alternatives)

                        layer_number = layers.index(layer) + 1
                        layer_selectors = [f".{classname}.m-{material} .cn-{cell_number} .y-{layer_number}"]

                        for alt in alternatives:
                            cls.tilesheets.add(alt.tilesheet)

                        if alt_count > 1:
                            common = layer.common()
                            anim = Animation.get(
                                f"{classname}-{material}-{cell_number}-{layer_number}",
                                [sprite_style(alt, common) for alt in alternatives],
                            )

                            duration = sum([1 / alt.anim_length for alt in alternatives])
                            StyleRule.set(
                                layer_selectors,
                                {
                                    "animation": f"{duration}s infinite step-end",
                                    "animation-name": anim,
                                    **common_style(common),
                                },
                            )
                        elif alt_count == 1:
                            StyleRule.set(layer_selectors, sprite_style(alternatives[0]))

        return (Animation.anims, StyleRule.rules)
