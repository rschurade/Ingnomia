from .themes import themes


def render(store, theme, output, build_id):
    try:
        ThemeCtor = themes[theme]
    except KeyError:
        raise KeyError(f"Theme '{theme}' was not found")

    t = ThemeCtor()
    t.render(store, output, build_id)
