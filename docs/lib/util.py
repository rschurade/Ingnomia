import os

DOCDIR = os.path.realpath(os.path.dirname(os.path.dirname(__file__)))


def empty(value):
    return value is None or value == ""


def sprite_rect(rect):
    (x, y, w, h) = map(int, rect.split(" "))
    return {"x": x, "y": y, "w": w, "h": h}


def sprite_offset(offset):
    (x, y) = (0, 0) if empty(offset) else map(int, offset.replace(",", "").strip().split(" "))
    return {"x": x, "y": y}


def sprite_offset3d(offset):
    (x, y, z) = (0, 0, 0) if empty(offset) else map(int, offset.strip().split(" "))
    return {"x": x, "y": y, "z": z}


def sprite_layers(sprite):
    return sprite


def sort_translations(lst):
    return sorted(lst, key=lambda item: item["translation"] if "translation" in item else item["id"])


def total_amount(chance):
    return sum(map(lambda x: 1 if float(x) == 0 else float(x), chance.split("|")))


def amount_hint(chance):
    chances = list(map(float, chance.split("|")))
    guaranteed = len(list(filter(lambda x: x == 0, chances)))
    optional = filter(lambda x: x > 0, chances)

    if guaranteed == len(chances):
        return None

    return ", ".join([f"{guaranteed} guaranteed", *map(lambda c: f"{int(c*100)}% chance to get 1 more", optional)])


def format_css(css):
    """Trivial formatter to make reading output easier"""
    lines = []
    level = 0

    for line in css.splitlines():
        line = line.strip()
        if len(line):
            if "{" in line:
                lines.append(f"{'  ' * level}{line}")
                level = level + 1
            elif "}" in line:
                level = level - 1
                lines.append(f"{'  ' * level}{line}")
            else:
                lines.append(f"{'  ' * level}{line}")

    return "\n".join(lines)


def log(msg, prefix="* "):
    print(f"{prefix}{msg}")
