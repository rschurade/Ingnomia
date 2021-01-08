from datetime import datetime
import os
import shutil
from jinja2 import Environment, FileSystemLoader

from ...sprite import BaseSprite
from ...util import DOCDIR, log, sort_translations, format_css
from .sprites import generate_sprite_styles

THEME_PATH = os.path.dirname(__file__)
ASSET_PATH = os.path.join(THEME_PATH, "assets")
CONTENT_PATH = os.path.realpath(os.path.join(DOCDIR, "..", "content"))


class DefaultTheme:
    content_assets = ["icon.png"]
    assets = []
    backgrounds = [
        {"id": "plain", "base": BaseSprite("SolidSelectionFloor")},
        {"id": "farm", "base": BaseSprite("TilledSoil"), "material": "Dirt"},
        {"id": "stone", "base": BaseSprite("BlockStoneFloor"), "material": "Marble"},
        {"id": "grass", "base": BaseSprite("Grass_1_5")},
        {"id": "wood", "base": BaseSprite("LogFloor"), "material": "Pine"},
    ]

    def __init__(self):
        self.assets = os.listdir(ASSET_PATH)
        self.env = Environment(loader=FileSystemLoader(THEME_PATH))

    def render(self, store, output, build_id):
        # Setup directories
        shutil.rmtree(output, ignore_errors=True)

        for subdir in ["assets", "workshop", "item"]:
            os.makedirs(os.path.join(output, subdir))

        log("Gathering data")

        categories = store.categories()
        items = store.items()
        food = store.food()
        drinks = store.drinks()
        plants = store.plants()
        tints = store.tints()
        workshops = store.workshops()
        constructions = store.constructions()
        sprites = store.sprites()

        tilesheets = set([v["tilesheet"] for sprite in sprites for v in sprite["basesprites"].values()]) | set(
            [bg["base"].tilesheet for bg in self.backgrounds]
        )

        (sprite_anims, sprite_rules) = generate_sprite_styles(sprites, self.backgrounds)

        # Setup navigation table
        navtable = [
            {"label": "Items", "categories": categories, "path": "item"},
            {
                "label": "Workshops",
                "categories": sort_translations(
                    [
                        {
                            "translation": g,
                            "groups": [{"hide": True, "items": list(filter(lambda w: w["tab"] == g, workshops))}],
                        }
                        for g in set(map(lambda w: w["tab"], workshops))
                    ]
                ),
                "path": "workshop",
            },
        ]

        # Setup search index
        search_index = [
            *[
                {
                    "label": i["translation"],
                    "href": f"item/{i['id']}.html",
                    "type": "item",
                    "kw": (
                        f"{i['id']} {i['translation']} "
                        f"{i['category']['id']} {i['category']['translation']} "
                        f"{i['group']['id']} {i['group']['translation']}"
                    ),
                }
                for i in items
            ],
            *[
                {
                    "label": w["translation"],
                    "href": f"workshop/{w['id']}.html",
                    "type": "workshop",
                    "kw": f"{w['id']} {w['translation']}",
                }
                for w in workshops
            ],
        ]

        global_context = {
            "navtable": navtable,
            "index": search_index,
            "sprites": sprites,
            "build": {"id": build_id, "date": datetime.utcnow().isoformat(" ", timespec="minutes")},
        }

        log("Rendering")

        # Prerender navtable
        navtable_rendered_root = self.env.get_template("navtable.html.j2").render(**global_context)
        navtable_rendered_nest = self.env.get_template("navtable.html.j2").render(base="../", **global_context)

        # Generate sprites css
        with open(os.path.join(output, "assets", "sprites.css"), mode="w") as f:
            f.write(
                format_css(
                    self.env.get_template("sprites.css.j2").render(
                        tints=tints, sprites=sprites, sprite_anims=sprite_anims, sprite_rules=sprite_rules
                    )
                )
            )

        # Generate tint filters
        with open(os.path.join(output, "assets", "tints.svg"), mode="w") as f:
            f.write(self.env.get_template("tints.svg.j2").render(tints=tints))

        # Generate item index
        with open(os.path.join(output, "items.html"), mode="w") as f:
            f.write(
                self.env.get_template("items.html.j2").render(
                    base="", navtable_rendered=navtable_rendered_root, **global_context
                )
            )

        # Generate item pages
        for item in items:
            with open(os.path.join(output, "item", f"{item['id']}.html"), mode="w") as f:
                f.write(
                    self.env.get_template("item.html.j2").render(
                        item=item, base="../", navtable_rendered=navtable_rendered_nest, **global_context
                    )
                )

        # Generate food index
        with open(os.path.join(output, "food.html"), mode="w") as f:
            f.write(
                self.env.get_template("food.html.j2").render(
                    base="", navtable_rendered=navtable_rendered_root, food=food, drinks=drinks, **global_context
                )
            )

        # Generate plant index
        with open(os.path.join(output, "plants.html"), mode="w") as f:
            f.write(
                self.env.get_template("plants.html.j2").render(
                    base="", navtable_rendered=navtable_rendered_root, plants=plants, **global_context
                )
            )

        # Generate workshop index
        with open(os.path.join(output, "workshops.html"), mode="w") as f:
            f.write(
                self.env.get_template("workshops.html.j2").render(
                    base="", navtable_rendered=navtable_rendered_root, **global_context
                )
            )

        # Generate workshop pages
        for workshop in workshops:
            with open(os.path.join(output, "workshop", f"{workshop['id']}.html"), mode="w") as f:
                f.write(
                    self.env.get_template("workshop.html.j2").render(
                        workshop=workshop, base="../", navtable_rendered=navtable_rendered_nest, **global_context
                    )
                )

        # Generate construction index
        with open(os.path.join(output, "constructions.html"), mode="w") as f:
            f.write(
                self.env.get_template("constructions.html.j2").render(
                    base="", navtable_rendered=navtable_rendered_root, constructions=constructions, **global_context
                )
            )

        # Generate index
        with open(os.path.join(output, "index.html"), mode="w") as f:
            f.write(
                self.env.get_template("index.html.j2").render(
                    base="", navtable_rendered=navtable_rendered_root, **global_context
                )
            )

        # Copy assets
        for asset in self.content_assets:
            shutil.copy(os.path.join(CONTENT_PATH, asset), os.path.join(output, "assets", asset))
        for asset in self.assets:
            shutil.copy(os.path.join(ASSET_PATH, asset), os.path.join(output, "assets", asset))
        for asset in tilesheets:
            shutil.copy(os.path.join(CONTENT_PATH, "tilesheet", asset), os.path.join(output, "assets", asset))
