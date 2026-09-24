---
title: "Appearance Settings"
description:
  How to customise the primary colour on your EspControl panel.
---

# Appearance

These settings control the active colour used across your panel. You'll find them in the **Settings** tab on the [Setup](/features/setup) page, under the **Appearance** section.

- **Primary** — the colour cards show when an entity is active. Use the colour picker or type a colour code (for example, `FF8C00` for orange). For the optional glass theme, this remains the fallback active colour for card types without a category accent.
- **Glass card appearance** — inactive grid tiles use a translucent slate tint with a fine light rim. Active tiles keep the translucent treatment and use category accents: lights are amber, garage doors/locks/covers are red, climate is blue, and media players are green. Gates and other types retain the configured Primary fallback.
- Category accents preserve the existing state rules: lights highlight when on; garage doors when open or moving; locks when unlocked, unlocking, open/opening, or jammed; covers when open or moving. Unknown/unavailable states are not intentionally treated as active.
- The glass treatment uses LVGL opacity and a one-pixel border only. It does not use blur, animations, or background sampling, to keep rendering and memory overhead low.

Secondary inactive cards and tertiary information cards use fixed panel colours so setup stays simpler and modal styling remains consistent.

Disabled Home Assistant cards keep their background colour and show dark grey labels and icons. Their normal text and icon colours return when the card becomes available again.

Colour changes apply to the panel automatically after a brief pause (about 200 ms), plus the time needed to redraw the cards, including cards on subpages. You do not need to restart the panel. **Reset colours** applies the default colour in the same way.
