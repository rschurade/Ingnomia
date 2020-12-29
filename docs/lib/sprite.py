import itertools

from .db import db
from .util import empty, log, sprite_offset, sprite_offset3d, sprite_rect
from .material import MaterialSet


# Sprites offset overrides for stuff that looks misplaced when using DB data
# There is most likely an explanation and a better option :)
offset_overrides = {
    "Sapling": "0 0",
    "SoilRampTop": "0 0",
    "SoilStairsTop": "0 0",
    "StoneRampTop": "0 0",
    "StoneStairsTop": "0 0",
}

# Material set override for sprites that have no default material set
materialset_overrides = {"Chest": "TWood"}


class Sprite:
    sprites = {}
    export = set()

    @classmethod
    def all(cls):
        return [s.render() for s in cls.sprites.values() if s.id in cls.export]

    @classmethod
    def get(cls, id, export=True):
        if empty(id):
            raise ValueError("Cannot create sprite without an id")

        if id not in cls.sprites:
            try:
                cls.sprites[id] = cls._build(id)
            except ValueError as ex:
                log(f"In sprite {id}", "! ")
                raise ex

        if export:
            cls.export.add(id)

        return cls.sprites[id]

    @classmethod
    def _build(cls, id):
        try:
            (offset, base, tint) = db.sprite(id)
        except:
            log(f"No sprite with id={id}", "! ")
            return EmptySprite(id)

        if id in offset_overrides:
            offset = offset_overrides[id]

        if empty(tint):
            tint = None

        variations = []
        animation = False

        for (mbase, mmat, msprite, effect) in db.sprite_by_material(id):
            variations.append(
                SpriteVariation(
                    BaseSprite.get(mbase) if not empty(mbase) else Sprite.get(msprite, export=False),
                    material=mmat,
                    effect=effect,
                )
            )

        for (tbase, tmat, tsprite) in db.sprite_by_material_type(id):
            variations.append(
                SpriteVariation(
                    BaseSprite.get(tbase) if not empty(tbase) else Sprite.get(tsprite, export=False), material=tmat
                )
            )

        for (rbase, rsprite) in db.sprite_random(id):
            variations.append(
                SpriteVariation(
                    BaseSprite.get(rbase) if not empty(rbase) else Sprite.get(rsprite, export=False),
                )
            )

        # Only keep FR rotation (or first available rotation if missing)
        rotations = list(db.sprite_rotations(id))
        if len(rotations):
            fr = [r for r in rotations if r[2] == "FR"]
            if len(fr) > 0:
                rotations = fr
            for (rbase, rsprite, rot) in rotations:
                variations.append(
                    SpriteVariation(BaseSprite.get(rbase) if not empty(rbase) else Sprite.get(rsprite, export=False))
                )
                break

        # Only keep Spring (or first available season) for season sprites, and only FR (or first rotation) when season rotations are available
        seasons = list(db.sprite_seasons(id))
        if len(seasons):
            spring = [s for s in seasons if s[1] == "Spring"]
            if len(spring) > 0:
                seasons = spring
            for (sbase, season) in seasons:
                if empty(sbase):
                    seasons_rotations = list(db.sprite_seasons_rotations(id, season))
                    if len(seasons_rotations):
                        fr = [r for r in seasons_rotations if r[1] == "FR"]
                        if len(fr) > 0:
                            seasons_rotations = fr
                        for (srbase, rot) in seasons_rotations:
                            variations.append(SpriteVariation(BaseSprite.get(srbase)))
                            break
                        break
                else:
                    variations.append(SpriteVariation(BaseSprite.get(sbase)))
                    break

        frames = list(db.sprite_frames(id))
        for (fbase,) in frames:
            animation = len(frames) > 1
            variations.append(SpriteVariation(BaseSprite.get(fbase)))

        layers = [
            SpriteLayer(
                BaseSprite.get(cbase) if not empty(cbase) else Sprite.get(csprite, export=False),
                sprite_offset(coffset),
            )
            for (cbase, coffset, csprite) in db.sprite_combine(id)
        ]

        if len(variations) > 0 and len(layers) > 0:
            raise ValueError(f"Sprite {id} has both variations and combined layers")

        if len(variations) == 0 and len(layers) == 0:
            variations.append(SpriteVariation(BaseSprite.get(id if empty(base) else base)))

        if len(layers):
            return LayeredSprite(id, layers, sprite_offset(offset), tint)
        else:
            return VariationSprite(id, variations, sprite_offset(offset), tint, animation=animation)

    def __init__(self, id):
        self.id = id
        self.material_sets = {}
        self.default_material_set = None

    def uses_material_set(self, material_set, is_default=False):
        self.material_sets[material_set.id] = material_set

        if is_default:
            self.default_material_set = material_set
            for material in material_set.all_materials:
                ms = MaterialSet(material, None)
                self.uses_material_set(ms)

    def uses_material_sets(self, material_sets):
        self.layer_material_sets = material_sets

    def render(self):
        layers = [RenderLayer(l) for l in self._render()]

        basesprites = {
            alt.base: {"rect": alt.rect, "tilesheet": alt.tilesheet} for layer in layers for alt in layer.alternatives
        }

        def by_material_set(layer, matset):
            matching_alts = [alt for alt in layer.alternatives if alt.material in matset.all_materials]
            tintable_alts = [alt for alt in layer.alternatives if alt.material is None and alt.tint == "Material"]
            if len(matching_alts) == 0 and len(tintable_alts) == 0:
                return RenderLayer(layer.alternatives)
            else:
                return RenderLayer(
                    [
                        *matching_alts,
                        *[alt.tinted(m) for m in matset.unique_tints for alt in tintable_alts],
                    ]
                )

        if hasattr(self, "layer_material_sets"):
            # Different material sets for each layer
            return {
                "id": self.id,
                "basesprites": basesprites,
                "layer_count": len(layers),
                "material_sets": [
                    {
                        "id": "any",
                        "names": ["any"],
                        "layers": [by_material_set(layer, s) for (layer, s) in zip(layers, self.layer_material_sets)],
                    }
                ],
            }
        else:
            if self.id in materialset_overrides:
                self.material_sets["any"] = self.material_sets[materialset_overrides[self.id]]
                self.default_material_set = self.material_sets["any"]

            return {
                "id": self.id,
                "basesprites": basesprites,
                "layer_count": len(layers),
                "material_sets": sorted(
                    [
                        {
                            "id": id,
                            "layers": [by_material_set(layer, s) for layer in layers],
                            "names": ["any", id] if self.default_material_set == s else [id],
                        }
                        for (id, s) in self.material_sets.items()
                    ],
                    key=lambda ms: ms["id"],
                ),
            }

    def _render(self, **kwargs):
        raise NotImplemented()

    def __repr__(self):
        return repr({"type": type(self).__name__, **self.__dict__})


class EmptySprite(Sprite):
    def _render(self, **kwargs):
        return []

    def __repr__(self):
        return "Empty"


class BaseSprite(Sprite):
    bases = {}

    @classmethod
    def get(cls, id):
        if empty(id):
            raise ValueError("Cannot create base sprite without an id")

        if id not in cls.bases:
            cls.bases[id] = BaseSprite(id)
        return cls.bases[id]

    def __init__(self, id):
        super().__init__(id)
        base = db.basesprite(id)
        if base is None:
            log(f"No BaseSprite with id={id}", "! ")
            self.missing = True
        else:
            self.missing = False
            (rect, self.tilesheet) = base
            self.rect = sprite_rect(rect)

    def _render(self, **kwargs):
        return [] if self.missing else [[RenderAlternative(self.id, self.tilesheet, self.rect, **kwargs)]]


class SpriteLayer(Sprite):
    def __init__(self, sprite, offset):
        self.sprite = sprite
        self.offset = offset
        if offset is not None:
            self.ox = offset["x"]
            self.oy = offset["y"]
            self.oz = offset["z"]

    def _render(self, ox=0, oy=0, oz=0, **kwargs):
        if self.offset is not None:
            ox += self.ox
            oy += self.oy
            oz += self.oz
        return self.sprite._render(ox=ox, oy=oy, oz=oz, **kwargs)


class LayeredSprite(Sprite):
    def __init__(self, id, layers, offset, tint):
        super().__init__(id)
        self.layers = layers
        self.tint = tint
        self.offset = offset
        if offset is not None:
            self.ox = offset["x"]
            self.oy = offset["y"]
            self.oz = offset["z"]

    def _render(self, ox=0, oy=0, oz=0, tint=None, **kwargs):
        if self.tint is not None:
            tint = self.tint
        if self.offset is not None:
            ox += self.ox
            oy += self.oy
            oz += self.oz
        return [l for layer in self.layers for l in layer._render(ox=ox, oy=oy, oz=oz, tint=tint, **kwargs)]


class SpriteVariation(Sprite):
    def __init__(self, sprite, material=None, effect=None, animation=False):
        self.sprite = sprite
        self.material = material
        self.effect = None if effect == "none" else effect

    def _render(self, material=None, effect=None, **kwargs):
        if self.material is not None:
            material = self.material
        if self.effect is not None:
            effect = self.effect
        return self.sprite._render(material=material, effect=effect, **kwargs)


class VariationSprite(Sprite):
    def __init__(self, id, variations, offset, tint, animation=False):
        super().__init__(id)
        self.variations = variations
        self.tint = tint
        self.offset = offset
        if offset is not None:
            self.ox = offset["x"]
            self.oy = offset["y"]
            self.oz = offset["z"]
        self.animation = animation

    def _render(self, ox=0, oy=0, oz=0, tint=None, animation=None, **kwargs):
        if self.tint is not None:
            tint = self.tint
        if self.offset is not None:
            ox += self.ox
            oy += self.oy
            oz += self.oz

        vlayers = [
            v._render(ox=ox, oy=oy, oz=oz, tint=tint, animation=self.animation, **kwargs) for v in self.variations
        ]
        layers = []
        nlayers = max(map(len, vlayers))
        for l in range(nlayers):
            layer = []
            for vls in vlayers:
                if len(vls) > l:
                    layer.extend(vls[l])

            layers.append(layer)
        return layers


class PlantSprite(Sprite):
    @classmethod
    def get(cls, id):
        return super().get(f"plant-{id}")

    @classmethod
    def _build(cls, id):
        plantid = id.replace("plant-", "")
        material = db.plant_material(plantid)
        spriteids = db.plant_sprites(plantid)

        if len(spriteids) > 0:
            sprite = VariationSprite(
                id,
                [SpriteVariation(Sprite.get(spriteid, export=False)) for spriteid in spriteids],
                None,
                None,
            )

            sprite.uses_material_set(MaterialSet(material, None), is_default=True)
            return sprite
        else:
            log(f"No sprite for plant id={plantid}", "! ")
            return EmptySprite(id)


class ConstructionSprite(Sprite):
    @classmethod
    def get(cls, id):
        return super().get(f"constr-{id}")

    @classmethod
    def _build(cls, id):
        constrid = id.replace("constr-", "")
        sprites = list(db.construction_sprites(constrid))

        if len(sprites) > 0:
            sprite = LayeredSprite(
                id,
                [
                    SpriteLayer(Sprite.get(spriteid, export=False), sprite_offset3d(offset))
                    for (spriteid, offset) in sprites
                ],
                None,
                None,
            )

            return sprite
        else:
            return Sprite.get(constrid)


class RenderLayer:
    def __init__(self, alternatives):
        self.alternatives = alternatives

    def common(self):
        """Return a dict with info that is common for all alternatives in the layer"""

        attrs = ["tilesheet", "ox", "oy", "effect"]
        rect = ["w", "h", "x", "y"]
        common = {}

        for attr in attrs:
            values = set([getattr(alt, attr) for alt in self.alternatives])
            if len(values) == 1:
                common[attr] = values.pop()

        for attr in rect:
            values = set([alt.rect[attr] for alt in self.alternatives])
            if len(values) == 1:
                common[f"rect{attr}"] = values.pop()

        return common

    def unique(self):
        """Returns a list of unique alternatives, taking tint colors into account"""
        unique_reprs = set()
        unique_alts = []

        for alt in self.alternatives:
            r = repr(alt.tinted(MaterialSet.colors[alt.material])) if alt.material in MaterialSet.colors else repr(alt)
            if r not in unique_reprs:
                unique_reprs.add(r)
                unique_alts.append(alt)

        return unique_alts

    def __repr__(self):
        return repr({"type": "RenderLayer", "alts": self.alternatives})


class RenderAlternative:
    def __init__(self, base, tilesheet, rect, ox=0, oy=0, oz=0, tint=None, material=None, effect=None, animation=False):
        self.base = base
        self.tilesheet = tilesheet
        self.rect = rect
        self.ox = ox
        self.oy = oy - 20 * oz
        self.tint = tint
        self.material = material
        self.effect = effect
        self.animation = animation

    def tinted(self, material):
        return RenderAlternative(
            self.base,
            self.tilesheet,
            self.rect,
            self.ox,
            self.oy,
            0,
            self.tint,
            material,
            self.effect,
            self.animation,
        )

    def __eq__(self, other):
        return hash(self) == hash(other)

    def __hash__(self):
        return hash((self.base, self.ox, self.oy, self.tint, self.material, self.effect))

    def __repr__(self):
        return repr({"type": "RenderAlternative", **self.__dict__})
