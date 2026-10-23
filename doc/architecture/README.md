---
package: modelEditor
summary: Tkinter/Pmw GUI (`modeleditor`) for creating and editing Likelihood XML source models, with catalog import, DS9 linkage and export to gtobssim format.
layer: python
depends_on: [tkinter, Pmw, pyds9, astropy, numpy]
used_by: []
last_reviewed_commit: ca3eab6
---

# modelEditor Architecture

**modelEditor** is a stand-alone, pure-Python desktop GUI for editing the
XML source-model files ("source libraries") read by `gtlike` and the other
`Likelihood` tools. It is installed as the `modeleditor` console script.
No other package imports it, and it does not import any other Fermitools
package: it parses and writes the XML with `xml.dom.minidom` and knows the
model vocabulary (spectrum and spatial-model types, their parameters) from
tables hard-coded in its own modules.

The package has no Doxygen mainpage, no release notes and no tests. The
in-repo `help.txt` and `modeleditor.txt` are user documentation, and both
are stale (see *Invariants and gotchas*). About 10,800 lines of Python in
21 modules, all at the top level of the repository.

## Purpose and scope

**Does:**
- Open, display, edit and save `<source_library>` XML files: sources
  (point or diffuse), their `<spectrum>` and `<spatialModel>`, and each
  `<parameter>`'s value, scale, min, max and free flag.
- Add stock sources: a generic point or diffuse source, and the old EGRET,
  GALPROP and extragalactic diffuse components.
- Import sources from a 1FGL, 2FGL or 3FGL catalog FITS file around the
  ROI of an FT1 file or `gtbin` image (`CatalogSourceExtractor`, a port of
  the old `make2FGLxml`/`make3FGLxml` scripts), together with Galactic and
  isotropic diffuse components.
- Sort sources (name, position, distance from ROI centre, types, index,
  flux).
- Talk to a running SAOImage DS9 through `pyds9`: load an image, draw the
  sources as regions, and pick up regions the user draws or selects in DS9.
- Export the model to the `gtobssim` (`observationSim`) XML dialect
  (*File → Export to ObsSim*), for point sources with simple power-law
  spectra only.

**Does not:**
- Validate against the Likelihood code. The list of model types and their
  parameter names is a private copy (`Spectrum._defaultParameters`,
  `SpatialModel._defaultParameters`) that has drifted from what
  `Likelihood` and `optimizers` actually support.
- Validate against the shipped XSD files. `source_library.xsd` and
  `source_library_obssim.xsd` are installed, but no code reads them.
- Evaluate models, compute fluxes or run any analysis.

## Position in the system

| Direction | Package | Why |
|-----------|---------|-----|
| Depends on (Fermi) | — | No Fermitools imports. CMake only installs files; nothing is compiled or linked. |
| Depends on (external) | `tkinter` (Python stdlib, needs Tk) | All widgets. |
| Depends on (external) | `Pmw` (Python megawidgets) | Imported unconditionally by `ModelEditor.py`, `HelpWindow.py`, `AddCatalogSourcesDialog.py` and every `*Editor.py` widget module. **Not a run requirement of the `fermitools` conda package** (`recipe/meta.yaml`); it is only in `environments/fermitools-testing.yml` and `fermitools-fermipy.yml`. |
| Depends on (external) | `pyds9` | Imported at module load by `DS9Connector.py`; if missing, the module prints a message and calls `sys.exit(-1)`. Also not a conda run requirement. |
| Depends on (external) | `astropy.io.fits`, `numpy` | Reading FT1/image/catalog FITS headers and tables (`FitsFile`, `CatalogSourceExtractor`). Both are run requirements. |
| Related format | `Likelihood` | Owner of the source-model XML format this tool edits. See `src/Likelihood/doc/architecture/` for the authoritative list of spectral and spatial types and parameter names. |
| Related format | `observationSim` | Target of the ObsSim export (`<source_library>` with `<spectrum escale="MeV"><particle><power_law .../>`). |
| Help text | `fermitools-fhelp` | Owns the installed `fhelp modeleditor` text (`fhelp_files/modeleditor.txt`). The copy in this repo is older and is not installed. |
| Used by | — | No package imports these modules (checked by `grep` over `src/*` for imports of every module name). |

The Level 1 docs list modelEditor as Tier 4 `python` with no Fermi
dependencies. That is correct.

## Key abstractions

Two parallel class hierarchies: **model** classes that mirror the XML
elements, and **editor** widgets (one per model class) that display and
edit them.

```
Model (xml.dom.minidom <-> Python)          Editor widgets (tkinter/Pmw)
------------------------------------        ----------------------------------
SourceLibraryDocument                      SourceLibraryDocumentEditor
  └─ SourceLibrary  <source_library>         └─ SourceLibraryEditor
       └─ Source     <source>                     └─ SourceEditor
            ├─ Spectrum     <spectrum>                 ├─ SpectrumEditor
            │    └─ ParameterSet ─ Parameter           │    └─ ParameterEditor
            └─ SpatialModel <spatialModel>             └─ SpatialModelEditor
                 └─ ParameterSet ─ Parameter                └─ ParameterEditor
Element  (base of all model classes)        ElementEditor (base of the editors)

ModelEditorApp (ModelEditor.py)  – main window, menus, file I/O
AddCatalogSourcesDialog → CatalogSourceExtractor → FitsFile (astropy)
DS9Connector (pyds9, background polling thread + queue)
HelpWindow (Pmw.ScrolledText over a text file)
```

| Module | Role |
|--------|------|
| `Element.py` | Base class: tag name, `toDom`/`fromDom`, XML boolean conversion, equality. |
| `Parameter.py`, `ParameterSet.py` | A `<parameter>` (name, value, scale, min, max, free) and an ordered set of them. `ParameterSet.setParameters` rejects any set whose names do not match the type's defaults. |
| `Spectrum.py` | `<spectrum type=… file=…>`. `_defaultParameters` is the table of known types and their default parameters (16 types; see below). |
| `SpatialModel.py` | `<spatialModel type=… file=…>`; 4 types: `SkyDirFunction`, `SpatialMap`, `MapCubeFunction`, `ConstantValue`. |
| `Source.py` | `<source name= type=>`; types `PointSource`, `DiffuseSource`. Also holds the gtobssim converters (`toObssimDom`, `_powerLaw2Gtobssim`, …). |
| `SourceLibrary.py` | `<source_library xmlns= title=>`: the list of sources plus sorting. |
| `SourceLibraryDocument.py` | The file: `fromFile` (`minidom.parse`), `toFile`, `toObssimDom`, add/remove/sort sources. |
| `*Editor.py` | Tk widgets. `get()` builds a model object from the widget state, `set()` loads one, `commit()` marks the current state as unchanged. |
| `ModelEditor.py` | `ModelEditorApp(tkinter.Frame)` and `main()`: menus *File*, *Edit* (all items disabled), *Source*, *Sort*, *DS9*, *Help*. |
| `AddCatalogSourcesDialog.py` | Pmw dialog collecting catalog file, FT1/image file, diffuse model files and names, radius and significance limits, extended-template directory. Returns a dict. |
| `CatalogSourceExtractor.py` | Turns that dict into `Source` objects plus the Galactic (`MapCubeFunction`) and isotropic (`FileFunction` × `ConstantValue`) diffuse sources. |
| `FitsFile.py` | Classifies a FITS file as `FT1` (HDU 1 has `NDSKEYS`), `image` (HDU 0 has `NDSKEYS`) or catalog (`CDS-NAME` in `1FGL`/`2FGL`/`3FGL`), and extracts the ROI from the DSS keywords. |
| `DS9Connector.py` | DS9 link. A background thread (`_thread.start_new_thread`) polls DS9 regions every 0.1 s and passes GUI work back through a `queue.Queue` that the Tk main loop drains, since Tk is not thread-safe. |

Known spectrum types (`Spectrum._defaultParameters`): `PowerLaw`,
`BrokenPowerLaw`, `PowerLaw2`, `BrokenPowerLaw2`, `LogParabola`,
`ExpCutoff`, `Gaussian`, `ConstantValue`, `FileFunction`,
`PLSuperExpCutoff`, `PLSuperExpCutoff2`, `PLSuperExpCutoff3`,
`PLSuperExpCutoff4`, `SmoothBrokenPowerLaw`, and also `RadialDisk` and
`RadialGaussian`, which `Likelihood` treats as *spatial* models but this
package lists as spectra.

## Data flow and control flow

```
modeleditor [--debug] [--verbose] [model.xml]
  → fermitools.ModelEditor:main()          (conda entry point)
      tkinter.Tk(); Pmw.initialise(root)
      ModelEditorApp(root, path)
        SourceLibraryDocument(path)        minidom.parse → SourceLibrary.fromDom
                                           → Source.fromDom → Spectrum/SpatialModel.fromDom
                                           (any unknown type or parameter name → TypeError)
        SourceLibraryDocumentEditor.set(doc)
      root.mainloop()

File → Save:   editor.get() → SourceLibraryDocument.toFile(path)
               → toDom() → minidom writexml (2-space indent)
File → Export to ObsSim:  doc.toObssimDom() → Source.toObssimDom per source
Source → Add Fermi Catalog Sources:
               AddCatalogSourcesDialog → dict
               → CatalogSourceExtractor.getSources()
                   FitsFile(eventFile) → ROI ra/dec/radius (from DSS keywords)
                   FitsFile(catalogFile) → 1FGL/2FGL/3FGL branch
                   → [Source, ...] + galactic + isotropic diffuse
               → appended to the document; DS9 regions redrawn if connected
```

Editing is state-in-widgets: the model objects are rebuilt from the
widgets by `get()` whenever the application needs the document (save,
add, sort), and pushed back with `set()`.

## Public interfaces

### Applications

| Entry point | Defined by | Notes |
|-------------|-----------|-------|
| `modeleditor` | `recipe/meta.yaml` (umbrella repo): `modeleditor = fermitools.ModelEditor:main` | The only installed launcher. Optional argument: a model XML path. Flags `--debug`, `--verbose` (see gotchas). No par file; not an `st_app` tool, so the `st_app` exit-status contract does not apply. |
| `modeleditor` (bash script in this repo) | — | **Not installed.** Runs `$FERMI_INST_DIR/bin/python $FERMI_INST_DIR/lib/python/ModelEditor.py`, a pre-conda layout that no longer exists. |

User-facing help: `fhelp modeleditor` (text owned by `fermitools-fhelp`).

### Python

All 21 modules are installed flat into
`lib/python3.X/site-packages/fermitools/`. `fermitools.pth` (written by the
umbrella `CMakeLists.txt`) puts that directory itself on `sys.path`, so the
modules import each other, and can be imported by users, under bare
top-level names (`import Source`, `from Spectrum import Spectrum`). The
library classes (`SourceLibraryDocument`, `Source`, `Spectrum`, …) can be
used without the GUI, but `Pmw` is still needed if any editor module is
imported, and nothing in the Fermitools does so.

### Environment variables

| Variable | Where | Effect |
|----------|-------|--------|
| `FERMI_DIR` | `AddCatalogSourcesDialog.py` (`getenv` at import) | Prefix for the default Galactic and isotropic diffuse file paths in the catalog dialog. Set by conda activation (see "Runtime environment" in ScienceTools' `docs/architecture/README.md`). If unset, building the dialog fails (`None + str`). |
| `FERMI_INST_DIR` | `ModelEditor._onHelpHelp` | *Help → Help* opens `$FERMI_INST_DIR/help/help.txt`. `KeyError` if unset. |
| (literal) `$(FERMI_DIR)` | `ModelEditorApp._EgretDiffuseSourcePath`, `_GalpropDiffuseSourcePath` | Written verbatim into the `file=` attribute of the EGRET and GALPROP diffuse sources; expanded later by `Likelihood`'s `facilities::Util::expandEnvVar`, not here. |

### File formats

- **Read and written:** Likelihood source-model XML (`<source_library>`).
  Output is `minidom` pretty-printed; `free` is written as
  `"true"`/`"false"` (both `Likelihood` and `optimizers` accept this as
  well as `1`/`0`).
- **Written:** gtobssim source XML (*Export to ObsSim*).
- **Read:** FT1 event files and `gtbin` images (ROI from DSS keywords),
  1FGL/2FGL/3FGL catalog FITS (`CDS-NAME` keyword), extended-source
  template directories, DS9 regions via XPA.
- **Installed but unused:** `source_library.xsd` and
  `source_library_obssim.xsd`, installed to both
  `share/fermitools/data/modelEditor` and
  `share/fermitools/refdata/fermi/modelEditor`. As written they are not
  even valid XSD (`xsd:attribute` precedes `xsd:sequence`).

## Build and test

- **CMake:** `CMakeLists.txt` only has `install()` rules: the 21 `.py`
  files to `${FERMI_INSTALL_PYTHON}` (`site-packages/fermitools`) and the
  two XSDs to `${FERMI_INSTALL_DATADIR}/modelEditor` and
  `${FERMI_INSTALL_REFDATADIR}/modelEditor`. Nothing is built.
- **Not installed:** `help.txt`, `modeleditor.txt`, the `modeleditor` bash
  script.
- **Launcher:** the `modeleditor` console script is generated by conda
  from `recipe/meta.yaml` `build.entry_points`, so it exists only in conda
  builds, not in a plain CMake install.
- **Tests:** none. There is no test directory, no CTest registration, and
  `recipe/tests/ST-unit-test` has no `modelEditor` check. `recipe/meta.yaml`
  `test.imports` does not import any modelEditor module.
- **Legacy:** `SConscript` (old SCons build) still builds
  `src/modeleditor.cxx`, which was deleted in commit `30db9ef`; its header
  comment wrongly names the package `pyBurstAnalysisGUI-01-04-03`.
  `azure-pipelines.yml` is the old Azure CI template (current CI lives in
  the umbrella repo).
- **Branches:** the remote branches `f26-cpp17-compliance`,
  `feature/f26_linuxBuild`, `master_testing` and `testing_gha` contain no
  commits that are not on `master`.

## Invariants and gotchas

**Launching (the installed tool does not start as shipped)**
- `modeleditor` fails at import with `ModuleNotFoundError: No module named
  'Pmw'` in a stock `fermitools` conda environment, because `Pmw` is not a
  run requirement (confirmed at runtime with the FT-2.5.1 environment).
  `pyds9` is in the same position; without it `DS9Connector` calls
  `sys.exit(-1)` during import, so the GUI cannot start even if no DS9
  feature is used (confirmed at runtime with a stub `Pmw`).
- `modeleditor model.xml` (a path, no `--debug`) crashes with
  `UnboundLocalError: cannot access local variable 'debug'`. `main()`
  assigns `debug = True` inside the function without `global`, so `debug`
  is local and unset when `--debug` is absent (confirmed at runtime). The
  same defect means `--debug`/`--verbose` never affect the module-level
  flags that the rest of the code reads. Starting without a path, and
  then using *File → Open*, avoids the crash.

**XML round-trip loses information**
- Only the attributes the model classes know are kept. Attributes such as
  `apply_edisp` on `<spectrum>`, or `flux`/`ROI_Center_Distance` on
  `<source>` (written by catalog scripts), are **silently dropped on
  save** (confirmed at runtime).
- A file whose `<source_library>` has no `xmlns` attribute opens, but
  *Save* then fails with `AttributeError: 'SourceLibrary' object has no
  attribute '_xmlns'` (`SourceLibrary.fromDom` only sets it when present;
  confirmed at runtime via `SourceLibraryDocument`). Many hand-written and
  script-generated models omit `xmlns`.
- Any spectrum or spatial type not in the private tables (e.g.
  `BandFunction`, `DMFitFunction`, `EblAtten::*`, `RadialProfile`,
  `CompositeSource`) or any parameter set whose names differ from the
  defaults (a missing `Scale`, a renamed parameter) raises `TypeError` while
  loading. `_open` catches only `ExpatError`, so in the GUI this is an
  unhandled exception rather than a dialog (from reading the code; the
  `TypeError` itself was confirmed at runtime).
- A document must contain at least one source: an empty
  `<source_library>` cannot be loaded, and the GUI refuses to delete the
  last source.

**Stale defaults and expired data**
- Catalog import supports only 1FGL, 2FGL and 3FGL (`FitsFile.validCatTypes`).
  4FGL (in use since 2019, now DR4) is rejected as "not a supported file
  type".
- The catalog dialog defaults to `galdiffuse/gll_iem_v06.fits` (not
  shipped by `fermitools-data`; only `gll_iem_v07.fits` is) and
  `isotrop_4years_P7_v9_repro_source_v1.txt` (Pass 7). Both should be
  replaced for any current analysis.
- *Add EGRET/GALPROP Diffuse Source* points at
  `$(FERMI_DIR)/refdata/fermi/EGRET_diffuse_cel.fits` and `GP_gamma.fits`;
  the installed files are under `refdata/fermi/galdiffuse/`, so those
  paths do not resolve.
- *Help → Help* opens `$FERMI_INST_DIR/help/help.txt`, which nothing
  installs (the `help/` directory holds only the `fhelp` `gt*.txt` files
  and `modeleditor.txt`), so the help window fails to load.
- *Help → About* says "Copyright NASA/GSFC 2008-2014", version 1.2.

**ObsSim export**
- Supported only for `PowerLaw`, `BrokenPowerLaw`, `PowerLaw2`,
  `BrokenPowerLaw2` spectra with a `SkyDirFunction` spatial model; anything
  else raises `TypeError` from `Source.toObssimDom`. `PowerLaw`/`BrokenPowerLaw`
  use a fixed 30 MeV–200 GeV range (`Source._Emin`, `_Emax`) to convert
  the prefactor to an integral flux.

**Catalog import**
- When an extended-source template cannot be found,
  `CatalogSourceExtractor` calls `showError(...)`, which is undefined
  (the module imports `showerror`), so a `NameError` is raised instead of
  a warning (from reading the code; not checked at runtime).
- The ROI is taken from the FT1/image DSS keywords (`NDSKEYS`), so the
  event file must have been through `gtselect` with a position cut.

**Namespace**
- Because modules are installed under bare top-level names
  (`Element`, `Parameter`, `Source`, `Spectrum`, `FitsFile`), they can
  shadow, or be shadowed by, same-named modules elsewhere on `sys.path`.

## Extension points

- **New spectral or spatial type:** add an entry to
  `Spectrum._defaultParameters` or `SpatialModel._defaultParameters` with
  the exact parameter names `Likelihood` expects (check
  `src/Likelihood/doc/architecture/` and the function's C++ source). The
  editors build their widgets from those tables, so no editor change is
  normally needed.
- **New catalog version:** add the `CDS-NAME` value to
  `FitsFile.validCatTypes` and a `_addNFGLSources` method in
  `CatalogSourceExtractor` mapping the catalog's spectral columns to
  `Spectrum` objects, then dispatch it in `getSources`.
- **New source attribute that must survive a save:** add it to
  `Source.toDom`/`fromDom` (or the relevant class); unknown attributes are
  otherwise discarded.
- **New ObsSim conversion:** add a `_xxx2Gtobssim` method to `Source` and
  dispatch on the spectrum type in `Source.toObssimDom`.

There are no tests; check changes by loading and saving a representative
model (for example one produced by `make4FGLxml`) and diffing the output.

## Further reading

- `help.txt` (this repo): the original user guide. Not installed, and
  partly stale (refers to GLAST).
- `fhelp modeleditor` / `src/fermitools-fhelp/fhelp_files/modeleditor.txt`:
  the installed help text.
- `src/Likelihood/doc/architecture/`: the source-model XML format and the
  spectral and spatial functions it supports.
- `src/observationSim/doc/architecture/`: the gtobssim source format.
- "Runtime environment" in ScienceTools' `docs/architecture/README.md`
  (`FERMI_DIR`, `FERMI_INST_DIR`).
