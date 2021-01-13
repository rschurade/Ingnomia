from .themes import themes


def render(theme, output, build_id):
    try:
        ThemeCtor = themes[theme]
    except KeyError:
        raise KeyError(f"Theme '{theme}' was not found")

    t = ThemeCtor()
    t.render(output, build_id)
