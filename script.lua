config = {
    window_title = "SDL3 Lua Node Editor",
    window_width = 800,
    window_height = 600,
    font_path = "Kenney Mini.ttf",
    font_size = 24,
    text = "Two Node2D Test",
    nodes = {
        { -- Node 1: Red square
            x = 400,
            y = 300,
            size = 100,
            r = 1.0,
            g = 0.0,
            b = 0.0
        },
        { -- Node 2: Blue square
            x = 450,
            y = 350,
            size = 80,
            r = 0.0,
            g = 0.0,
            b = 1.0
        }
    },
    camera = {
        x = 0,
        y = 0,
        scale = 1.0
    }
}