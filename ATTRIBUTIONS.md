# Attributions

All third-party assets must be CC0 or permissively licensed. Machine-readable records live in [assets/LICENSES.json](assets/LICENSES.json); run `python3 tools/check_licenses.py` to verify every shipped asset has an entry.

| Asset | Author | License | Source |
|---|---|---|---|
| Low poly medieval weapons (Dagger, ShortSword, LongSword, Katana, SimpleAxe, Club, Spear) | Quaternius | CC0 1.0 | https://opengameart.org/content/low-poly-medieval-weapons |

Code dependencies: raylib 5.5 (zlib/libpng license), Emscripten (MIT/UIUC).
Processing applied: OBJ n-gons fan-triangulated with `tools/triangulate_obj.py`; geometry otherwise unmodified.
