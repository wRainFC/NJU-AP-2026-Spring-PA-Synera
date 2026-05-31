# Synera UI Design

## 1. Goals and Boundaries

The UI layer is responsible for window management, input projection, layout hit testing, rendering, texture loading, and visual fallback. It must not implement combat, shop, synergy, equipment, save, or placement rules. Rule changes continue to live in `core`, `board`, and `systems`.

The refactor targets three concrete capabilities:

- Resizable windows with reliable mouse hit testing.
- PNG sprite-sheet animation for units.
- Deterministic fallback to the current geometric/text rendering when image assets are missing.

The internal UI coordinate system remains a virtual `1280 x 720` canvas. The first implementation scales that canvas into any real window using letterboxing. Later, selected panels can become adaptive without changing game logic or input semantics.

## 2. Current Problems

- `Renderer.cpp` mixes high-level screen composition, low-level drawing helpers, colors, text offsets, unit fallback rendering, and hover panel logic.
- UI colors, font sizes, button states, and rectangle hit tests are repeated in multiple files.
- `Renderer.hpp` exposes implementation details by including resource management and by depending on drag state declarations from `InputController.hpp`.
- Input reads raw Raylib mouse state directly, so window scaling would break hit testing.
- Texture loading supports static PNGs, but there is no animation model or resource naming convention.

## 3. Target Architecture

Runtime flow:

```text
GameApp
  -> GameWindow         owns Raylib window, virtual render target, pointer projection
  -> InputController    consumes PointerInput and calls systems/GameState operations
  -> Renderer           reads RenderContext and draws to the virtual canvas
```

Rendering flow:

```text
Renderer
  -> GridItem           board hexes and bench slots
  -> UnitItem           unit sprite animation, fallback body, bars, labels, equipment
  -> UiDrawing          buttons, panels, text, bars, textured fallback rectangles
  -> RenderAssets       UI textures, equipment textures, unit textures and sprite sheets
```

Header boundaries:

- `UiState.hpp` owns `DragKind`, `DragState`, `InputResult`, and `PointerInput`.
- `Renderer.hpp` exposes only `RenderContext` and the `Renderer` facade. Asset and drawing details stay in `.cpp` files.
- `InputController.hpp` exposes input orchestration only. It takes a `PointerInput` created by `GameWindow`.
- `Layout` owns virtual-canvas geometry and hit testing. It does not know the real window size.

## 4. Window and Input Model

`GameWindow` creates a resizable Raylib window and a fixed-size `1280 x 720`
render texture. The texture is drawn to the window with continuous proportional
scaling and letterboxing, so common window sizes such as `1600 x 900` and
`1920 x 1080` use the available space instead of waiting for the next integer
scale. Each frame:

1. Begin drawing into the virtual render texture.
2. Draw the entire game at `1280 x 720` coordinates.
3. End texture drawing.
4. Clear the real window to the letterbox color.
5. Draw the render texture into the actual window with centered proportional scale.

Mouse input is converted from physical window coordinates into virtual canvas coordinates before it reaches `InputController`. If the pointer is in a letterbox region, the projected coordinates may be outside the virtual bounds and normal rectangle hit testing will reject actions.

## 5. Sprite Animation and Fallback

Unit animation uses sprite sheets under:

```text
assets/textures/units/<template_id>/<owner>_<state>.png
```

Where:

- `<owner>` is `player` or `enemy`.
- `<state>` is `idle`, `moving`, `attacking`, `casting`, `stunned`, or `dead`.
- A sprite sheet is a single-row PNG. Frame height equals image height, frame width equals image height, and frame count is `image_width / image_height`.
- The default playback rate is `8 fps`.

Lookup order:

1. Exact `<template_id>/<owner>_<state>.png`.
2. `<template_id>/<owner>_idle.png`.
3. Static legacy unit PNG loaded from `assets/textures/units/<template_id>.png`.
4. Owner default static PNG: `player_default.png` or `enemy_default.png`.
5. Geometric fallback drawing.

Missing resources are not errors. The game must remain playable with an empty `assets/textures` directory.

## 6. Visual and Interaction Rules

- Top bar shows HP, gold, population, round, and phase.
- Board hexes distinguish enemy and player halves.
- Bench slots remain visible even when empty.
- Units always show HP and mana bars, name, star level, and equipment marker.
- Shop cards show unit preview when available, otherwise text fallback.
- Disabled buttons are visibly muted and ignored by input.
- Drag preview follows projected virtual pointer coordinates, not raw window coordinates.
- Hover panels are placed near the virtual pointer and clamped to the virtual canvas.
- Victory/defeat overlay blocks interactions except load.

## 7. Migration Plan

1. Add `GameWindow`, `UiState`, `UiTheme`, `UiDrawing`, `GridItem`, and `UnitItem`.
2. Replace raw Raylib mouse access in `InputController` with `PointerInput`.
3. Rework `GameApp` to render through `GameWindow`.
4. Extend `RenderAssets` to load sprite sheets and expose animation views.
5. Rebuild `Renderer` as a facade that composes item-level drawing helpers.
6. Update CMake and remove obsolete UI helper drafts.
7. Build, run tests, and manually verify scaled input and asset fallback.

## 8. Acceptance Criteria

- `cmake --build build` succeeds.
- Existing tests still pass.
- The app opens in a resizable window.
- Board, bench, shop, buttons, drag/drop, hover panels, equipment, save/load, and outcome overlay still work.
- Mouse clicks and drags hit the same virtual UI controls after resizing the window.
- With no PNG assets, fallback rendering is stable and playable.
- With a valid unit sprite sheet in the documented location, that unit animates.
