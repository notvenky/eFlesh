# eFlesh tactile gripper for the SO-101 arm

This folder holds our work on adding eFlesh tactile sensing to the gripper of a
SO-101 robot arm (LeRobot). Everything outside this folder is the unmodified
eFlesh project from NYU (github.com/notvenky/eFlesh), which this repo forks.

## Goal
Measure contact location, grip force, and slip on the SO-101 gripper jaws, and
ideally recognize texture from sliding contact.

## Phases
- **Phase 1 (current):** sensorize the SO-101 gripper jaws.
- **Phase 2 (later):** eFlesh fingertips for the Aero Hand Open, which mounts on
  the same SO-101 wrist. Phase 1 scripts and procedures are written to be reused
  for this.

## Upstream projects used
| Project | Used for | License |
|---|---|---|
| [eFlesh](https://github.com/notvenky/eFlesh) (this fork) | Lattice/pouch generation, Arduino sketch | MIT |
| [SO-ARM100](https://github.com/TheRobotStudio/SO-ARM100) | Gripper CAD (SO-101 jaws) | Apache 2.0 |
| [reskin_sensor](https://github.com/raunaqbhirangi/reskin_sensor) | 5X ReSkin magnetometer board | MIT |
| [arduino-MLX90393](https://github.com/tesshellebrekers/arduino-MLX90393) (eFlesh submodule) | Magnetometer chip library | MIT |

SO-ARM100 and reskin_sensor are not committed here. To get local reference
copies, run from the repo root:

```bash
git clone --depth 1 https://github.com/TheRobotStudio/SO-ARM100 third_party/SO-ARM100
```

```bash
git clone --depth 1 https://github.com/raunaqbhirangi/reskin_sensor third_party/reskin_sensor
```

(`third_party/` is git-ignored.)

## Status
Step 1 (setup and inspection) in progress.
