# Phase 155: Emboss 3MF Text Metadata Round-Trip

**Status:** Ready to execute
**Workstream:** CLOS (EMB-06 closure)
**Requirement:** CLOS-02

## Goal

Persist editable-text metadata via the upstream `TextConfigurationSerialization`
path so that a TextEmboss volume saved to 3MF reloads as a re-editable
TextEmboss (not just opaque geometry). Phase 146 shipped the geometry
round-trip (as MODEL_PART); this phase adds the `<slic3rpe:text>` block.

## Source-truth mapping

Upstream `third_party/OrcaSlicer/src/libslic3r/Format/bbs_3mf.cpp:7885-7887`
already writes `<slic3rpe:text ...>` automatically when `volume->text_configuration`
is populated — OWzx just has to build & attach the struct on save. Upstream
`bbs_3mf.cpp:3461-3462,4072,4084` already parses it back into
`volume->text_configuration` on load — OWzx just has to read it back.

Structs (`third_party/OrcaSlicer/src/libslic3r/TextConfiguration.hpp`):
- `TextConfiguration { EmbossStyle style; std::string text; }`
- `EmbossStyle { std::string name; std::string path; Type type; FontProp prop; }`
- `FontProp { ... float size_in_mm; std::optional<float> boldness; std::optional<std::string> family; ... }`

**Depth note:** `depth` is a projection property upstream (not a FontProp
field). The geometry itself encodes depth (ProjectZ(depth) creates a mesh
with that Z extent). Source-truth persistence of depth = the mesh Z range
which already round-trips. We additionally surface the original depth intent
via the volume name suffix in the mock layer (not upstream-mandated; purely
an in-session restoration affordance).

## Plan

### Wave 1 — Save side: attach TextConfiguration on volume creation

1. `src/core/services/ProjectServiceMock.cpp` `performEmbossVolumeAdd` (~line 2565):
   After `newVol->set_type(...)`, build and attach `TextConfiguration`:
   ```cpp
   Slic3r::TextConfiguration tc;
   tc.text = text.toStdString();
   tc.style.name = fontFamilyName (basename of fontPath without ext);
   tc.style.path = fontPath;
   tc.style.type = Slic3r::EmbossStyle::Type::file_path;
   tc.style.prop.size_in_mm = m_embossHeight;
   tc.style.prop.family = fontFamilyName;
   newVol->text_configuration = std::move(tc);
   ```
2. Apply the same to the async path (`addTextVolumeAsync` worker at ~2726-2737).
   Refactor into a small `attachEmbossMetadata(newVol, text, fontPath, height, depth)`
   helper to keep both paths identical.

### Wave 2 — Load side: restore MockVolumeEntry from text_configuration

3. In the `loadProject` rebuild lambda (~ProjectServiceMock.cpp:6242+), add a
   per-volume walk over `obj->volumes`. When `volume->text_configuration` has
   a value, tag the rebuilt `MockVolumeEntry` as `MockVolumeType::TextEmboss`
   (so the loaded volume is editable in the Emboss panel, not just geometry).

### Wave 3 — Test anchor

4. `tests/QmlUiAuditTests.cpp`: add `v51EmbossTextMetadataRoundTripWired`
   regression slot asserting:
   - `performEmbossVolumeAdd` / async path assigns `text_configuration`
   - the load rebuild lambda checks `text_configuration` and tags the
     MockVolumeEntry as TextEmboss
   - `TextConfiguration.hpp` is included
   - `EmbossStyle::Type::file_path` is referenced (the source-truth font path type)

## Verification

- Canonical build (`scripts/auto_verify_with_vcvars.ps1`) exits 0
- 5/5 ctest groups PASS (PrepareScene / PartPlate / ViewModel / UI / PreviewParser)
- 12 v5.0 regression slots still pass
- The new `v51EmbossTextMetadataRoundTripWired` slot passes
