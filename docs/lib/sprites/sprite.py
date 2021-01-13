from random import choice

from ..db import db
from ..util import empty, log, sprite_offset, sprite_rect

from .render import Render


def base_or_sprite(base, sprite):
    return BaseSprite(base) if not empty(base) else SpriteManager.get(sprite, internal=True)


class MissingMaterials(Exception):
    def __init__(self, id, materials):
        super().__init__(f"Missing materials for '{id}': {', '.join(materials)}")
        self.materials = materials


class Sprite:
    def __init__(self, id, data, tint=None, offset=None):
        self.id = id
        self.data = data
        self.tint = tint
        self.offset = offset
        self.setup()

    def setup(self):
        raise NotImplementedError()

    # Possible kwargs:
    #  rotation: one of 'FR' (default), 'FL', 'BR', 'BL' or 'all'
    #  season: one of 'Spring' (default), 'Summer', 'Winter', 'Autumn' or 'all'
    #  random: 'all' (default), 'random' or 'first'
    #  frames: 'all' (default), any int, any range
    #  materials: 'any' (default) or a list of material IDs
    #  multi_materials: None (default) or a list of values suitable for 'materials' (for CombinedSprites)
    def select(self, tint=None, offset=None, **kwargs):
        if self.tint is not None:
            tint = self.tint

        if self.offset is not None:
            offset = self.offset

        return self._select(tint=tint, offset=offset, **kwargs)

    def _select(self, **kwargs):
        raise NotImplementedError()

    def __repr__(self):
        return repr({"sprite": type(self).__name__, "attrs": self.__dict__})


class EmptySprite(Sprite):
    def __init__(self):
        super().__init__("empty", None)

    def setup(self):
        pass

    def _select(self, **kwargs):
        return Render.empty()


class BaseSprite(Sprite):
    def __init__(self, id, **kwargs):
        super().__init__(id, None, **kwargs)

    def setup(self):
        base = db.basesprite(self.id)
        if base is None:
            raise ValueError(f"No BaseSprite with id '{self.id}'")

        (rect, self.tilesheet) = base
        self.rect = sprite_rect(rect)

    def _select(self, tint=None, materials=None, effects=[], offset=None, anim_frame=0, anim_length=1, **kwargs):
        params = {"effects": effects, "anim_frame": anim_frame, "anim_length": anim_length}

        if offset is not None:
            params["ox"] = offset["x"]
            params["oy"] = offset["y"]

        if (tint == "Material" or self.tint == "Material") and materials is not None:
            return Render.alternatives(
                [Render.base(self.id, self.tilesheet, self.rect, material=mat, **params) for mat in materials]
            )

        return Render.base(self.id, self.tilesheet, self.rect, **params)


class AnimatedSprite(Sprite):
    def setup(self):
        self.frames = [BaseSprite(base) if isinstance(base, str) else base for (base,) in self.data]

    def _select(self, frames="all", **kwargs):
        if frames == "all":
            indices = [i for i in range(len(self.frames))]
        elif isinstance(frames, int):
            indices = [frames]
        elif isinstance(frames, range):
            indices = frames
        else:
            raise ValueError(f"Invalid frames spec for '{self.id}': {frames}")

        return Render.alternatives(
            [self.frames[i].select(anim_frame=indices.index(i), anim_length=len(indices), **kwargs) for i in indices]
        )


class RandomSprite(Sprite):
    def setup(self):
        self.choices = [base_or_sprite(base, sprite) for (base, sprite) in self.data]

    def _select(self, random="all", **kwargs):
        if random == "first":
            return self.choices[0].select(**kwargs)
        elif random == "random":
            return choice(self.choices).select(**kwargs)
        elif random == "all":
            return Render.alternatives([c.select(**kwargs) for c in self.choices])
        else:
            raise ValueError(f"Invalid random spec for '{self.id}': {random}")


class RotatingSprite(Sprite):
    def setup(self):
        self.rotations = {}
        self.effects = {}

        for (base, sprite, rotation, effect) in self.data:
            self.rotations[rotation] = base_or_sprite(base, sprite)
            self.effects[rotation] = effect

    def _select(self, rotation="FR", effects=[], **kwargs):
        if rotation == "all":
            return Render.alternatives(
                [self.rotations[r].select(effects=[*effects, self.effects[r]], **kwargs) for r in self.rotations.keys()]
            )
        elif rotation in self.rotations:
            return self.rotations[rotation].select(effects=[*effects, self.effects[rotation]], **kwargs)
        else:
            raise ValueError(f"Invalid rotation spec for '{self.id}': {rotation}")


class SeasonalSprite(Sprite):
    def setup(self):
        self.seasons = {}

        for (base, season) in self.data:
            if empty(base):
                rotations = list(db.sprite_seasons_rotations(self.id, season))
                if len(rotations) == 0:
                    raise ValueError(f"No season rotations in {season} for sprite '{self.id}'")

                self.seasons[season] = RotatingSprite(
                    self.id, [(base, None, rotation, None) for (base, rotation) in rotations]
                )
            else:
                self.seasons[season] = BaseSprite(base)

    def _select(self, season="Spring", **kwargs):
        if season == "all":
            return Render.alternatives([self.seasons[s].select(**kwargs) for s in self.seasons.keys()])
        elif season in self.seasons:
            return self.seasons[season].select(**kwargs)
        else:
            raise ValueError(f"Invalid season spec for '{self.id}': {season}")


class CombinedSprite(Sprite):
    def setup(self):
        self.layers = []
        self.offsets = []
        self.tints = []

        for (base, offset, sprite, tint) in self.data:
            self.layers.append(base_or_sprite(base, sprite))
            self.offsets.append(sprite_offset(offset))
            self.tints.append(None if empty(tint) else tint)

    def _select(self, tint=None, multi_materials=None, offset=None, **kwargs):
        if multi_materials is not None:
            if len(multi_materials) != len(self.layers):
                raise ValueError(
                    f"Invalid multi materials for '{self.id}': expected {len(self.layers)}, got {len(multi_materials)}"
                )

            return Render.layers(
                [
                    layer.select(offset=offset, tint=ctint if tint is None else tint, materials=mats, **kwargs)
                    for (layer, offset, ctint, mats) in zip(self.layers, self.offsets, self.tints, multi_materials)
                ]
            )

        return Render.layers(
            [
                layer.select(offset=offset, tint=ctint if tint is None else tint, **kwargs)
                for (layer, offset, ctint) in zip(self.layers, self.offsets, self.tints)
            ]
        )


class MaterialSprite(Sprite):
    def setup(self):
        self.materials = {}
        self.effects = {}

        for (base, mat, sprite, effect) in self.data:
            self.materials[mat] = base_or_sprite(base, sprite)
            self.effects[mat] = effect

    def _select(self, materials="any", effects=[], **kwargs):
        if materials == "any":
            materials = self.materials.keys()
        else:
            missing = [m for m in materials if m not in self.materials]
            if len(missing):
                raise MissingMaterials(self.id, sorted(missing))

        return Render.alternatives(
            [self.materials[m].select(materials=[m], effects=[*effects, self.effects[m]], **kwargs) for m in materials]
        )


class MaterialTypeSprite(MaterialSprite):
    def __init__(self, id, data, **kwargs):
        super().__init__(id, [(base, mat, sprite, None) for (base, mat, sprite) in data], **kwargs)


class SpriteManager:
    empty = EmptySprite()
    sprites = {}
    exports = set()

    @classmethod
    def get(cls, id, internal=False):
        if empty(id):
            raise ValueError("Cannot create sprite without an id")

        if id not in cls.sprites:
            try:
                cls.sprites[id] = cls._build(id)
            except Exception as ex:
                log(f"Error building sprite '{id}': {ex}", "! ")
                cls.sprites[id] = cls.empty

        if not internal:
            cls.exports.add(id)

        return cls.sprites[id]

    @classmethod
    def random(cls, id, spriteids):
        return RandomSprite(id, [(None, id) for id in spriteids])

    @classmethod
    def _build(cls, id):
        try:
            (offset, base, tint) = db.sprite(id)
        except:
            log(f"No sprite with id '{id}'", "! ")
            return cls.empty

        if empty(tint):
            tint = None

        if empty(offset):
            offset = None

        if empty(base):
            base = id

        kwargs = {"tint": tint, "offset": sprite_offset(offset)}

        by_material = list(db.sprite_by_material(id))
        if len(by_material):
            return MaterialSprite(id, by_material, **kwargs)

        by_material_type = list(db.sprite_by_material_type(id))
        if len(by_material_type):
            return MaterialTypeSprite(id, by_material_type, **kwargs)

        random = list(db.sprite_random(id))
        if len(random):
            return RandomSprite(id, random, **kwargs)

        rotations = list(db.sprite_rotations(id))
        if len(rotations):
            return RotatingSprite(id, rotations, **kwargs)

        seasons = list(db.sprite_seasons(id))
        if len(seasons):
            return SeasonalSprite(id, seasons, **kwargs)

        frames = list(db.sprite_frames(id))
        if len(frames):
            return AnimatedSprite(id, frames, **kwargs)

        combine = list(db.sprite_combine(id))
        if len(combine):
            return CombinedSprite(id, combine, **kwargs)

        return BaseSprite(base, **kwargs)
