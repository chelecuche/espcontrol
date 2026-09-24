# Local glass-card appearance maintenance

This customization is isolated to the existing firmware card-style layer and does not change the ESPHome entry-point workflow. The device YAML selects the fork through a tested immutable tag; ESPHome Device Builder still resolves the package graph, compiles, and installs the firmware.

## Incorporating upstream ESPControl updates

1. Fetch `upstream/main` and record the currently deployed immutable tag/commit as the rollback point.
2. Create a candidate branch from the fork's `glass-cards` development branch and merge the desired upstream commit (or rebase the small local appearance commits onto it).
3. Resolve conflicts without dropping these ownership boundaries:
   - centralized glass and category tokens in `components/espcontrol/button_grid_style.h`;
   - per-card palette selection and application in `components/espcontrol/button_grid_grid.h`;
   - state semantics in the existing card state helper;
   - this appearance documentation and the corresponding regression checks.
4. Run the generated-output check, card-runtime checks, firmware tests, and compile the exact wall-tablet configuration in ESPHome Device Builder.
5. Only after every check passes, update the device YAML's pinned fork ref to the tested commit and install it. Keep the previous commit/YAML as the immediate rollback target until the panel is online and validated.
6. If an upstream conflict or compile regression remains unresolved, leave the device YAML pinned to the last working commit. Do not force-update the deployed ref or remove the local patch to make the merge appear clean.

Do not edit `.esphome`, `build/`, downloaded ESPHome package caches, or generated firmware output to maintain this feature; those files are rebuilt and are not authoritative sources.
