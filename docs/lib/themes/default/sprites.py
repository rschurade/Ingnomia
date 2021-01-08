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
        if len(self.ids) > 1:
            return f"anim_{Animation.anims.index(self)}"
        else:
            return self.ids[0]


class StyleRule:
    rules = []

    @classmethod
    def get(cls, selectors, props):
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

    if "rectw" not in common and alt.rect["w"] != 32:
        props["width"] = f"{alt.rect['w']}px"

    if "recth" not in common and alt.rect["h"] != 32:
        props["height"] = f"{alt.rect['h']}px"

    if "ox" not in common and alt.ox != 0:
        props["left"] = f"{alt.ox}px"

    if "oy" not in common and alt.oy != 0:
        props["top"] = f"{alt.oy}px"

    if alt.tint == "Material" and alt.material:
        props["filter"] = f"url(tints.svg#{alt.material})"

    if alt.effect == "Rot90":
        props["transform"] = "rotate(-90deg)"
    elif alt.effect == "FlipHorizontal":
        props["transform"] = "rotateY(180deg)"

    return props


def common_style(common):
    props = {}

    if "tilesheet" in common:
        props["background-image"] = f"url({common['tilesheet']})"

    if "rectx" in common and "recty" in common:
        props["background-position"] = f"-{common['rectx']}px -{common['recty']}px"

    if "rectw" in common and common["rectw"] != 32:
        props["width"] = f"{common['rectw']}px"

    if "recth" in common and common["recth"] != 32:
        props["height"] = f"{common['recth']}px"

    if "ox" in common and common["ox"] != 0:
        props["left"] = f"{common['ox']}px"

    if "oy" in common and common["oy"] != 0:
        props["top"] = f"{common['oy']}px"

    return props


def generate_sprite_styles(sprites, backgrounds):
    """Generate style rules and keyframes, and deduplicate them"""

    for bg in backgrounds:
        if "material" in bg:
            alt = bg["base"].alternative(tint="Material").tinted(bg["material"])
        else:
            alt = bg["base"].alternative()
        StyleRule.get([f".bg-{bg['id']}"], sprite_style(alt))

    for sprite in sprites:
        for matset in sprite["material_sets"]:
            for layer in matset["layers"]:
                alternatives = layer.unique()
                alt_count = len(alternatives)

                layer_number = matset["layers"].index(layer) + 1
                layer_selectors = [
                    f".s-{sprite['id']}.m-{name} .layer:nth-child({layer_number})" for name in matset["names"]
                ]

                if alt_count > 1:
                    common = layer.common()
                    anim = Animation.get(
                        f"{sprite['id']}-{matset['id']}-{layer_number}",
                        [sprite_style(alt, common) for alt in alternatives],
                    )

                    if len(list(filter(lambda a: not a.animation, alternatives))) == 0:
                        # Only animation frames, make animation faster
                        duration = alt_count / 4
                    else:
                        duration = alt_count

                    StyleRule.get(
                        layer_selectors,
                        {"animation": f"{duration}s infinite step-end", "animation-name": anim, **common_style(common)},
                    )
                elif alt_count == 1:
                    StyleRule.get(layer_selectors, sprite_style(alternatives[0]))

    return (Animation.anims, StyleRule.rules)
