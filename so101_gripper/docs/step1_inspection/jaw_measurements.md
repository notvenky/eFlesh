# Step 1.2: SO-101 jaw measurements (from CAD)

Source files (SO-ARM100 repo):
- `STL/SO101/Individual/Moving_Jaw_SO101.stl` (rigid moving jaw)
- `STL/SO101/Individual/Wrist_Roll_Follower_SO101.stl` (gripper body + fixed jaw)
- `Optional/Compliant_Gripper/stl/Compliant_*_SO101.stl` (TPU versions)
- Assembly placement and joint range: `Simulation/SO101/so101_new_calib.urdf`
  (the simulation meshes match the printable STLs to within 0.07 mm, so the
  URDF placement applies directly to the print files)

All values are from CAD. The real arm's calibration and range are measured later.

## Coordinate frames
- **Jaw frame** (moving-jaw STL as exported): pivot axis = z through (x=0, y=0);
  finger points toward -y; grip face faces -x.
- **Gripper frame** (URDF `gripper_link`): pivot at (20.2, 18.8, -23.4) mm,
  axis along -y; jaw mid-plane at y = 0.

## Moving jaw (rigid)
| Item | Value |
|---|---|
| Overall size | 22.3 x 92.0 x 48.0 mm (48 mm = clevis width along pivot axis) |
| Main grip face | flat plane at x = -8.3 mm (8.3 mm from pivot axis), y = -22 to -62 (40 mm long) |
| Grip face height (along pivot axis) | 22 mm at y=-25, 17.4 at y=-40, 14.6 at y=-60 |
| Tip "nub" | two steps toward other jaw: x=-10.3 (y -62..-72), x=-12.3 (y -72..-81); tip at 82 mm from pivot |
| Finger thickness (x) | 15.7 mm at y=-25 to 11.8 mm at y=-60 |
| Internal features | 1.5 mm slanted slot layer at mid-height (z = +/-0.75) splitting the finger into 6 segments; five 1.8 x 1.5 mm teardrop channels along z at y = -19, -29, -39, -49, -59 (the one at y=-19 is fully sealed). Likely a slicer trick for extra internal walls. |

## Fixed jaw (part of Wrist_Roll_Follower)
Stepped grip face, all steps facing +x (toward the moving jaw), gripper frame:

| x (mm) | z range (mm) | face width along pivot axis (mm) |
|---|---|---|
| -15.0 | -45 to -37 | 30 |
| -13.3 | -65 to -45 | 22 |
| -11.6 | -84 to -65 | 15 |
| -9.9 | -94 to -84 | 12 |
| -7.9 | -103 to -94 (tip) | 10 |

## Motion (q = gripper joint angle, URDF convention; positive opens)
| q (deg) | angle between main flats | tip-to-tip (mm) | note |
|---|---|---|---|
| -11.4 | 11.4 (V, tips closer) | 3.1 | jaws first touch (just past URDF lower limit) |
| -10.0 | 10.0 | 3.0 | URDF lower limit (~2 mm min gap) |
| 0 | 0 (parallel) | 15.8 | main flats 25.2 mm apart |
| 10 | 10 | 30.1 | |
| 30 | 30 | 58.1 | |
| 45 | 45 | 78.0 | |
| 100 | 100 | 137.2 | URDF upper limit |

- Moving jaw only collides with the body between q = 110 and 120 deg.
- The main grip flats are parallel only at q = 0. For q > 0 the jaws form a V
  that opens toward the tips; for q < 0 a V that closes toward the tips.
- Because both faces step toward each other near the tips, the outer "envelope"
  of each face is tilted (~6 deg fixed, ~4 deg moving), so a flat object is
  contacted mainly at the tip steps unless the jaw opens ~10 deg past q = 0.

## Compliant (TPU) moving jaw
- Same outer geometry (identical bounding box and finger profile); 37% less volume
  (fixed-jaw body: 23% less).
- Finger is a Fin Ray-style shell: 1.2 mm front wall (grip face), 1.2 mm slanted
  back wall, joined by thin straight cross-ribs at y = -25 (solid root), -37,
  -49, -60. Rib spacing ~12 mm, ribs perpendicular to the finger.
- Cavities are open through the full height (top and bottom of the finger).
- Tip (y -62..-81) is a hollow wedge: thick front (2 mm) plus a thin diagonal back wall.

Figures: `gripper_motion.png`, `moving_jaw_rigid_vs_compliant.png`.
