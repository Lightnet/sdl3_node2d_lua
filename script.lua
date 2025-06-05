config = {
    window_title = "SDL3 Lua Node Editor",
    window_width = 800,
    window_height = 600,
    font_path = "Kenney Mini.ttf",
    font_size = 24,
    text = "Two Node2D Test",
    camera = {
        x = 0,
        y = 0,
        scale = 1.0
    }
}

nodes = {
    {
        x = 255,
        y = 225,
        size = 100,
        r = 1.0,
        g = 0.0,
        b = 0.0,
        text = "Node 1",
        inputs = 1,  -- One input connector
        outputs = 1  -- One output connector
    },
    {
        x = 586,
        y = 337,
        size = 80,
        r = 0.0,
        g = 0.0,
        b = 1.0,
        text = "Node 2",
        inputs = 1,  -- One input connector
        outputs = 1  -- One output connector
    }
}