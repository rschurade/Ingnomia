from ..db import db
from ..util import empty

colors = {mat: color for (mat, color) in db.materials()}


class RenderBase:
    animation = False

    def __init__(self, base, tilesheet, rect, material=None, effects=[], ox=0, oy=0, anim_frame=0, anim_length=1):
        self.base = base
        self.tilesheet = tilesheet
        self.rect = rect
        self.material = material
        self.effects = [e for e in effects if not empty(e) and e != "none"]
        self.ox = ox
        self.oy = oy
        self.anim_frame = anim_frame
        self.anim_length = anim_length

    def __repr__(self):
        return repr(self.__dict__)

    def uid(self, use_color=False):
        dct = self.__dict__

        if use_color and (self.material is not None):
            dct = {**dct, "material": colors[self.material]}

        return repr(dct)


class RenderLayers:
    def __init__(self, layers):
        self.layers = layers

    def __repr__(self):
        return repr(self.layers)


class RenderAlternatives:
    def __init__(self, alternatives):
        self.alternatives = alternatives

    def unique(self, use_color=False):
        unique_uids = set()
        unique_alts = []

        for alt in self.alternatives:
            uid = alt.uid(use_color=use_color)
            if uid not in unique_uids:
                unique_uids.add(uid)
                unique_alts.append(alt)

        return unique_alts

    def common(self):
        common = {}
        attrs = ["tilesheet", "material", "ox", "oy"]
        rattrs = ["x", "y", "w", "h"]

        for attr in attrs:
            values = set([getattr(alt, attr) for alt in self.alternatives])
            if len(values) == 1:
                common[attr] = values.pop()

        effects = set([",".join(alt.effects) for alt in self.alternatives])
        if len(values) == 1:
            common["effects"] = effects.pop().split(",")

        for attr in rattrs:
            values = set([alt.rect[attr] for alt in self.alternatives])
            if len(values) == 1:
                common[f"rect{attr}"] = values.pop()

        return common

    def __repr__(self):
        return repr(self.alternatives)


class Render:
    @classmethod
    def base(cls, *args, **kwargs):
        return cls.layers([cls.alternatives([RenderBase(*args, **kwargs)])])

    @classmethod
    def layers(cls, layers):
        # Flatten layers
        if any([isinstance(layer, RenderLayers) for layer in layers]):
            return cls.layers(
                [sub for layer in layers for sub in (layer.layers if isinstance(layer, RenderLayers) else [layer])]
            )

        return RenderLayers(layers)

    @classmethod
    def alternatives(cls, alternatives):
        # Flatten alternatives
        if any([isinstance(alt, RenderAlternatives) for alt in alternatives]):
            sub_alts = [
                sub
                for alt in alternatives
                for sub in (alt.alternatives if isinstance(alt, RenderAlternatives) else [alt])
            ]

            anim_lengths = set([sub.anim_length for sub in sub_alts])

            if len(anim_lengths) > 1:
                raise ValueError(f"Alternative with multiple animation lengths")

            anim_length = anim_lengths.pop()

            # Interleave animation frames
            anim_frames = set([sub.anim_frame for sub in sub_alts])
            if anim_length > 1 and len(sub_alts) > anim_length and len(anim_frames) == anim_length:
                interleaved = []
                while len(sub_alts):
                    for frame in range(anim_length):
                        next_frame = [sub for sub in sub_alts if sub.anim_frame == frame][0]
                        interleaved.append(next_frame)
                        sub_alts.pop(sub_alts.index(next_frame))
                sub_alts = interleaved

            return cls.alternatives(sub_alts)

        # Make layers toplevel
        if any([isinstance(alt, RenderLayers) for alt in alternatives]):
            layers = []
            num_layers = max([len(alt.layers) for alt in alternatives if isinstance(alt, RenderLayers)])

            for i in range(num_layers):
                layer_alts = []
                for alt in alternatives:
                    if isinstance(alt, RenderLayers) and i < len(alt.layers):
                        layer_alts.append(alt.layers[i])
                    elif isinstance(alt, RenderBase) and i == 0:
                        layer_alts.append(alt)

                layers.append(cls.alternatives(layer_alts))

            return cls.layers(layers)

        return cls.layers([RenderAlternatives(alternatives)])

    @classmethod
    def empty(cls):
        return cls.layers([])
