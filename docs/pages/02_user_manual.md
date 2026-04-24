@page user_manual User Manual
@brief Step-by-step guide for users

# User Manual

This guide explains how to install and use FastScope.  No programming knowledge is required.

---

## Table of Contents

1. [Installation](#installation)
2. [Launching the Application](#launching)
3. [Loading HDF5 Files](#loading-files)
4. [The Three-Pane Layout](#layout)
5. [Browsing the Signal Tree](#signal-tree)
6. [Adding and Removing Signals](#add-remove)
7. [Renaming Files](#renaming)
8. [Plot Interactions](#plot-interactions)
9. [Multi-Y-Axis Plots](#multi-y-axis)
10. [Crosshair Tooltip](#crosshair)
11. [Switching Plot Types](#plot-types)
12. [Annotation Tool](#annotation-tool)
13. [Ruler Tool](#ruler-tool)
14. [Derived Signals](#derived-signals)
15. [Exporting Data](#exporting)
16. [Session Save and Restore](#session)
17. [Collapsing Panes](#pane-collapse)
18. [Keyboard Shortcuts Reference](#shortcuts)

---

## 1. Installation {#installation}

### Prerequisites

FastScope runs on **macOS 13 Ventura or later** (Apple Silicon and Intel both supported).
It requires HDF5 libraries, which you can install using Homebrew:

```bash
# Install Homebrew if you haven't already:
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install HDF5:
brew install hdf5
```

### Building from Source

```bash
# Clone the repository
git clone https://github.com/your-org/fastscope.git
cd fastscope

# Configure (Release mode for best performance)
cmake -B build -DCMAKE_BUILD_TYPE=Release

# Build (uses all available CPU cores)
cmake --build build -j

# Optional: run tests
cmake --build build --target test
```

After a successful build, the application binary is at `build/fastscope`.

### Pre-built Application Bundle *(when available)*

Download `FastScope.dmg` from the Releases page, open it, and drag `FastScope.app` into your
`/Applications` folder.  On first launch, macOS may warn about an unidentified developer;
right-click the app icon and choose **Open** to bypass the check.

---

## 2. Launching the Application {#launching}

**From the command line:**
```bash
./build/fastscope
```

You can also pass an HDF5 file as an argument and it will be loaded automatically:
```bash
./build/fastscope /path/to/my_data.h5
```

**From Finder:** double-click the app bundle if you placed it in `/Applications`.

---

## 3. Loading HDF5 Files {#loading-files}

FastScope can open one or more HDF5 (`.h5` / `.hdf5`) files at the same time.  Each file
becomes an independent *dataset* shown in the left **Files** pane.

### Method A — Drag and Drop

Drag one or more `.h5` files from Finder directly onto the FastScope window.
The files are opened immediately and appear in the Files pane.

### Method B — Open Dialog (Cmd+O)

Press **Cmd+O** (or go to **File → Open**) to open a standard macOS file browser.
You can select multiple files by holding **Shift** or **Cmd** while clicking.

### Time Axis Selection

When a file is opened, FastScope automatically detects likely time-axis datasets
(datasets whose name contains "time" or "t" and that are one-dimensional).
The detected time axis is shown in the Files pane.  You can change it by clicking
the dropdown next to the file name and selecting a different dataset, or by
selecting **(index)** to use sample numbers (0, 1, 2, …) as the X axis.

---

## 4. The Three-Pane Layout {#layout}

The main window is divided into three areas side by side:

```
┌──────────────┬──────────────────┬──────────────────────────────┐
│  Files Pane  │  Signals Pane    │  Plot Area                   │
│  (left)      │  (middle)        │  (right)                     │
│              │                  │                              │
│  Lists open  │  Shows the       │  Renders the active plot.    │
│  dataset     │  HDF5 signal     │  Drag, zoom, annotate here.  │
│  files and   │  tree for the    │                              │
│  time axis   │  selected sim.   │                              │
│  controls.   │  Search & add    │                              │
│              │  signals here.   │                              │
└──────────────┴──────────────────┴──────────────────────────────┘
```

Each pane has a **collapse button** (▶/◀) at the top.  Click it to collapse the pane
into a narrow icon strip, giving more space to the plot.  Click again to expand.

---

## 5. Browsing the Signal Tree {#signal-tree}

The middle **Signals** pane shows the hierarchical HDF5 dataset tree for whichever
dataset is selected in the Files pane (click a simulation name to select it).

### Expanding Groups

Click the **▶** arrow next to a group name (e.g. `/attitude/`) to expand it and
reveal the datasets inside.  Click again to collapse.

### Searching / Filtering

Type in the **Search** box at the top of the Signals pane.  The tree instantly
filters to show only datasets whose name or full H5 path contains your search text.
Press **Esc** or clear the box to show everything again.

### Signal Metadata

Hover over a signal name to see a tooltip with:
- **Full HDF5 path** (e.g. `/attitude/quaternion_body`)
- **Shape** (e.g. `[10000 × 4]`)
- **Data type** (e.g. `Float64`)
- **Units** (e.g. `rad`, `m/s`)

---

## 6. Adding and Removing Signals {#add-remove}

### Adding a Signal

Click the **[+]** button to the right of any signal name in the tree.  The signal
is added to the current plot and a colored line appears immediately.

For vector signals (e.g. a quaternion with 4 components), a **component selector**
appears below the signal name in the legend.  You can choose:
- **All** — plots each component as a separate line
- **0, 1, 2, …** — plots only the selected component

### Removing a Signal

Click the **[–]** button in the plot legend next to a signal name, or right-click
the legend entry and choose **Remove**.  The line disappears and the color is
returned to the pool for future signals.

### Toggling Visibility

Click the colored square (the legend icon) next to a signal name to temporarily
hide the line without removing the signal.  Click again to show it.

---

## 7. Renaming Files {#renaming}

By default, files are named after their HDF5 filename (without the directory
path or `.h5` extension).  To give a file a friendlier name:

1. In the **Files** pane, double-click the file name.
2. Type the new name.
3. Press **Enter** to confirm, or **Esc** to cancel.

The new name appears in the legend prefix for all signals from that file.

---

## 8. Plot Interactions {#plot-interactions}

### Zooming

- **Scroll wheel** (or two-finger trackpad swipe) over the plot to zoom in/out
  on both axes simultaneously.
- Hold **Ctrl** while scrolling to zoom the **X axis** only.
- Hold **Shift** while scrolling to zoom the **Y axis** only.
- Draw a **selection rectangle** by left-click-dragging to zoom into a specific region.

### Panning

Click and drag with the **right mouse button** (or **two-finger drag** on trackpad)
to pan the view.

### Fitting the View

| Button | Action |
|--------|--------|
| **Fit All** | Zoom both X and Y axes to show all visible data |
| **Fit X** | Auto-range the X (time) axis only |
| **Fit Y** | Auto-range all Y axes only |
| **Double-click** on the plot | Same as Fit All |

The Fit buttons are located in the toolbar above the plot area.

---

## 9. Multi-Y-Axis Plots {#multi-y-axis}

FastScope supports up to **three independent Y axes** on a single Time Series plot,
which is essential when comparing signals with very different units or scales
(e.g. a position in km alongside an angular rate in deg/s).

To assign a signal to a Y axis:

1. Right-click the signal entry in the plot legend.
2. Choose **Y Axis → Left (Y1)**, **Right (Y2)**, or **Third (Y3)**.

The axis labels update automatically based on the units of the assigned signals.

---

## 10. Crosshair Tooltip {#crosshair}

Move the mouse cursor over the plot area to activate the **crosshair**.
A vertical line follows your cursor across all signals simultaneously, and a
floating tooltip shows:
- The **time value** at the cursor position
- The **interpolated value** of every visible signal at that time

This is useful for quickly reading exact values at a specific point in the data.

---

## 11. Switching Plot Types {#plot-types}

Click the **plot-type selector** in the toolbar (top-right area of the plot panel)
to switch between:

| Plot Type | Description |
|-----------|-------------|
| **Time Series** | Default. Plots one or more signals against the time axis. |

When you switch plot types, the signals already added remain selected; the new plot
type will render them in its own way.

---

## 12. Annotation Tool {#annotation-tool}

The **Annotation** tool (toolbar icon: **A**) lets you pin text labels to specific
points in the plot.

**To add an annotation:**
1. Click **A** in the toolbar to activate the tool.
2. Click anywhere on the plot at the time and value position you want to annotate.
3. A text dialog appears.  Type your annotation text (e.g. `"Manoeuvre start"`).
4. Press **Enter** to confirm.  A label with an optional arrow is placed at that point.

**To remove an annotation:** right-click the annotation label and choose **Delete**.

**To clear all annotations:** click **Clear** in the Annotation tool settings.

---

## 13. Ruler Tool {#ruler-tool}

The **Ruler** tool (toolbar icon: **R**) measures the distance between two points.

**Usage:**
1. Click **R** in the toolbar.
2. Click the **first point** on the plot (start of the measurement).
3. Click the **second point** on the plot (end of the measurement).
4. A line is drawn between the two points with a tooltip showing:
   - Δt (time difference)
   - ΔY (value difference)
   - Euclidean distance in plot coordinates

Click the first point again to reset and start a new measurement.

---

## 14. Derived Signals {#derived-signals}

FastScope can compute new signals on the fly from existing ones, without any scripting.

To create a derived signal:

1. Open the **Derived Signals** panel from the toolbar or the **View** menu.
2. Click **New Derived Signal**.
3. Choose an **operation** from the dropdown:
   - **Add** — sum of two scalar signals, sample by sample
   - **Subtract** — difference of two scalar signals
   - **Multiply** — product of two scalar signals
   - **Scale** — multiply a signal by a numeric constant
   - **Magnitude** — √(x₀² + x₁² + … + xₙ₋₁²) across N component signals
4. Select the **input signals** from the loaded files.
5. Give the derived signal a **display name** (e.g. `"‖r_eci‖"`).
6. Click **Compute**.

The result appears in the Signals pane under a **Derived** section and can be added
to the plot like any other signal.  The computation result is cached; click
**Recompute** if the inputs have changed.

---

## 15. Exporting Data {#exporting}

### Export as CSV

Go to **File → Export CSV** (or Cmd+Shift+E).  A save dialog appears.  The exported
file contains:
- A header row with signal names and units
- One row per time sample (using the first visible signal's time grid)
- One column per component of each visible signal

### Export as PNG Screenshot

Go to **File → Export PNG** (or Cmd+Shift+S).  macOS's built-in screencapture tool
is invoked in interactive mode, letting you drag a selection rectangle over any part
of the screen.  The saved image is in full resolution.

---

## 16. Session Save and Restore {#session}

FastScope can save and restore your complete workspace.

### Auto-save on Exit

If **Restore session on startup** is enabled in Preferences (`Cmd+,`), the session
is saved automatically when you quit the application (`Cmd+Q`).

### Manual Save / Load

- **File → Save Session** (`Cmd+S`) — save to the current session file.
- **File → Save Session As…** — choose a custom location.
- **File → Open Session…** — restore a previously saved session.

A session file (`.json`) remembers:
- Which HDF5 files were open (file paths)
- Which signals were plotted, with their colors, aliases, and Y-axis assignments
- Plot zoom levels
- Annotations
- Pane layout (widths, collapsed state)

---

## 17. Collapsing Panes {#pane-collapse}

Each of the three panes can be collapsed to make more room for the others.

- Click the **◀** button in the header of the **Files** pane to collapse it to a narrow strip.
- Click the **◀** button in the header of the **Signals** pane to collapse it.
- Click the **▶** button on the collapsed strip to expand it again.

When a pane is collapsed, you can still see its icon so you remember it's there.

---

## 18. Keyboard Shortcuts Reference {#shortcuts}

| Shortcut | Action |
|----------|--------|
| **Cmd+O** | Open file(s) |
| **Cmd+W** | Close selected file |
| **Cmd+S** | Save session |
| **Cmd+Shift+E** | Export CSV |
| **Cmd+Shift+S** | Export PNG |
| **Cmd+,** | Open Preferences |
| **Cmd+Q** | Quit |
| **Esc** | Clear search / cancel active tool |
| **Double-click plot** | Fit all axes to data |
| **Space** | Toggle crosshair |
