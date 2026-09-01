# Input Actions and Intent

Status: scalar movement and jump actions, SDL mapping, and human and injected
action-to-character conversion implemented; typed action values and vehicle
intent planned

Input crosses three boundaries before it affects simulation:

```text
keyboard / gamepad / injected state
    -> physical input adapter
    -> named actions
    -> gameplay intent
    -> character, vehicle, camera, or interaction system
```

## Physical input

`inputSdl` owns SDL device discovery, controller identifiers, button and axis
enums, event polling, dead zones, and device lifetime. None of those types or
identifiers may appear in `inputCore` or a domain library.

Mappings convert equivalent controls to the same action. For example,
DualSense Cross, Switch B, and keyboard Space can all produce `jump`. Runtime
code observes `jump`; it does not branch on the originating device.

Injected input passes through the same normalization and mapping boundary so
tests do not create a second control path.

## Named actions

Actions use stable semantic names such as:

```text
move
look
jump
interact
accelerate
brake
```

The initial implementation exposes scalar `move.x` and `move.y` actions plus a
`jump` action represented as zero or one. `inputSdl` maps Space and the first
gamepad's south button to the same name, and deterministic injection traverses
that same physical-state mapper. As later slices need richer transitions,
`inputCore` should support the minimum useful typed action value set:

| Value | Meaning | Examples |
| --- | --- | --- |
| `Button` | Pressed, released, and held state. | `jump`, `interact` |
| `Axis1D` | One normalized scalar. | `accelerate`, `brake` |
| `Axis2D` | One normalized two-dimensional value. | `move`, `look` |

Backends may expose richer physical information internally, but additions to
the core value model require a demonstrated gameplay use case.

## Gameplay intent

Named actions describe what a user requested; domain intent describes what a
runtime system should attempt. Controller logic converts actions into types
such as `CharacterIntent`, `VehicleIntent`, or camera-rig input. AI and behavior
systems produce those same intent types directly.

Intent is consumed at the fixed-step boundary. The standalone host currently
converts movement and jump actions into `CharacterIntent`. Character and
vehicle systems translate it into backend-neutral physics commands rather than
authoring USD transforms. This keeps keyboard, gamepad, deterministic tests,
AI, and future network input on one simulation path.
