=============================================================================
  Assignment 5 — Bezier Curves & Revolution Surfaces
  CSCI 557 / Computer Graphics
=============================================================================

BUILDING
--------
Linux/Mac:
    bash compile-unx.bat

Windows:
    compile-win.bat

Requires: Vulkan SDK, GLFW, GLM.


PROGRAM USAGE
-------------

The program starts in EDIT MODE (orthographic view, Y-up, looking along +Z).
The horizontal blue line is the axis of revolution (X axis).
Draw control points in the upper half of the window to define the profile curve.


KEY BINDINGS
------------

--- Edit / Navigation ---
SPACE       Toggle between Edit Mode and Navigation Mode
ESC         Quit

--- Edit Mode (SPACE to enter) ---
Left-click  Add a control point above the axis (Y > 0)
            The Bezier profile curve updates automatically after 2+ points.

--- Surface Generation ---
B           Build the revolution surface from the current Bezier profile
N           Show / hide normal vectors on the surface (green arrows)
M           Reset — clear all control points, curve, and surface

--- Camera Navigation (Navigation Mode only, activated by SPACE) ---
Arrow keys          Rotate (left/right = azimuth, up/down = elevation)
Shift + Arrows      Pan (shift the look-at target)
= or KP_+          Zoom in
- or KP_-          Zoom out
Numpad 4/6/8/2      Same as arrow keys (rotate)
F                   Fit-all (auto-frame the entire surface)
C                   Toggle Perspective ↔ Orthographic projection

Mouse (optional — works if you have one):
  Press R then drag   Rotate
  Press P then drag   Pan
  Press Z then drag   Zoom
  Press T then drag   Twist / roll

--- Phase 5: Creativity Features ---
X           Toggle TWIST: wraps the surface into a 360° helical spiral.
            Press B to rebuild the surface after toggling.
G           Toggle COLOR GRADIENT: rainbow axial coloring ON/OFF.
            Press B to rebuild after toggling.


WORKFLOW EXAMPLE
----------------
1. Launch → you are in Edit Mode.
2. Click several points above the blue axis to draw a profile (e.g. a vase shape).
3. Press B → the surface of revolution appears.
4. Press N → green normal arrows confirm correct lighting normals.
5. Press SPACE → switch to Navigation Mode.
6. Hold R and drag to rotate; hold P to pan; hold Z to zoom; press F to fit.
7. Press SPACE again → back to Edit Mode to add more points.
8. Press M to reset and start over.
9. Press X then B → see the helical twist effect.
10. Press G then B → toggle rainbow coloring.


PHASE DESCRIPTIONS
------------------

Phase 1 — Bezier Curve
  Left-click adds control points (screen coords mapped to NDC [-1,1]).
  _allBernstein computes all Bernstein basis values at u; the curve
  is re-evaluated at resolution 200 after every new point.

Phase 2 — Revolution Surface
  The 2D profile (X-axis = axial, Y-axis = radius) is revolved 360°
  around the X-axis using 50x50 resolution by default.
  Each vertex position: (px, pr·cosθ, pr·sinθ)
  Indices build two CCW triangles per quad cell.

Phase 3 — Normal Vectors
  The 2D tangent is computed via the Bezier derivative.
  The 2D outward normal is the perpendicular: (−ty, tx).
  3D normal: (−ty, tx·cosθ, tx·sinθ) — analytically unit-length.
  Press N to display 5%-length green arrows on every surface vertex.

Phase 4 — User Interaction
  SPACE toggles Edit ↔ Navigation modes.
  Edit mode uses orthographic projection for accurate point placement.
  Navigation mode enables orbit/pan/zoom via mouse + R/P/Z keys.
  M resets both CPU data and GPU buffers (waits for device idle first).

Phase 5 — Creativity: Twist + Color Gradient
  TWIST (key X, then B):
    theta_actual = j·Δθ + u·2π
    Each axial ring is rotated by an additional angle proportional to its
    u parameter, creating a helical twist from one end of the surface to
    the other. This is a direct extension of the surface-of-revolution
    parametric formula and demonstrates how a small change to the angular
    parameter produces a topologically distinct surface class.

  COLOR GRADIENT (key G, then B):
    Each ring of vertices is assigned a color by mapping u ∈ [0,1] to
    hue ∈ [0°, 300°] in HSV space (s=0.85, v=0.95), then converting to
    RGB. This makes it easy to visually verify the axial parameterization
    and distinguish individual rings when debugging the surface topology.
    It also looks great.


FILES MODIFIED / ADDED
----------------------
my_bezier_curve_surface.h   — Added twist/color fields and accessors
my_bezier_curve_surface.cpp — Implemented Phases 1, 2, 3, 5
my_application.h            — Added toggleTwist, toggleColorGradient
my_application.cpp          — Implemented Phases 4 (reset, edit/nav modes),
                              camera bounding box update, Phase 5 wiring
my_window.cpp               — Added X, G key bindings
README.txt                  — This file


AUTHOR NOTES
------------
Normal vectors are analytically correct for smooth surfaces:
the analytic perpendicular avoids finite-difference approximation errors
that can cause shading artifacts near high-curvature regions of the profile.

The UV coordinates stored in each vertex (u = axial, v = angular/2π) are
ready for texture mapping if a sampler descriptor is added to the pipeline.
