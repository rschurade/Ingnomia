from datetime import datetime
import os
import shutil
from jinja2 import Environment, FileSystemLoader

from ...store import store
from ...util import DOCDIR, log, sort_translations, format_css
from .sprites import StyleManager

THEME_PATH = os.path.dirname(__file__)
ASSET_PATH = os.path.join(THEME_PATH, "assets")
TEMPLATE_PATH = os.path.join(THEME_PATH, "templates")
CONTENT_PATH = os.path.realpath(os.path.join(DOCDIR, "..", "content"))


class DefaultTheme:
    content_assets = ["icon.png"]
    assets = []

    def __init__(self):
        self.assets = os.listdir(ASSET_PATH)
        self.env = Environment(loader=FileSystemLoader(TEMPLATE_PATH))

    def render(self, output, build_id):
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

        menu = [
            {"label": "Items", "href": "items.html"},
            {"label": "Food", "href": "food.html"},
            {"label": "Plants", "href": "plants.html"},
            {"label": "Constructions", "href": "constructions.html"},
            {"label": "Workshops", "href": "workshops.html"},
        ]

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

        log("Building sprites")

        for item in items:
            StyleManager.create_item_sprite(item)

        for item in items:
            for craft in [*item["crafts"], *item["rcrafts"]]:
                if craft["id"]:
                    StyleManager.add_item_materialset(craft["id"], craft["material_set"])
                for component in craft["components"]:
                    StyleManager.add_item_materialset(component["id"], component["material_set"])

        for cat in constructions:
            for constr in cat["constructions"]:
                StyleManager.create_construction_sprite(constr)
                for component in constr["components"]:
                    StyleManager.add_item_materialset(component["id"], component["material_set"])

        for workshop in workshops:
            StyleManager.create_workshop_sprite(workshop)

        for type in plants:
            if type["type"] == "Plant":
                floor = "farm"
            elif type["type"] == "Tree":
                floor = "grass"
            elif type["type"] == "Mushroom":
                floor = "mushroom"

            for plant in type["plants"]:
                StyleManager.create_plant_sprite(plant, floor=floor)

        for item in menu:
            StyleManager.create_menu_sprite(item)

        for w in sorted(StyleManager.warnings):
            log(w, "! ")

        (sprite_anims, sprite_rules) = StyleManager.styles()

        log("Rendering")

        global_context = {
            "menu": menu,
            "navtable": navtable,
            "index": search_index,
            "build": {"id": build_id, "date": datetime.utcnow().isoformat(" ", timespec="minutes")},
            "sm": StyleManager,
        }

        # Prerender navtable
        navtable_rendered_root = self.env.get_template("navtable.html.j2").render(**global_context)
        navtable_rendered_nest = self.env.get_template("navtable.html.j2").render(base="../", **global_context)

        # Generate sprites css
        with open(os.path.join(output, "assets", "sprites.css"), mode="w") as f:
            f.write(
                format_css(
                    self.env.get_template("sprites.css.j2").render(
                        tints=tints, sprite_anims=sprite_anims, sprite_rules=sprite_rules
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
        for asset in StyleManager.tilesheets:
            shutil.copy(os.path.join(CONTENT_PATH, "tilesheet", asset), os.path.join(output, "assets", asset))
