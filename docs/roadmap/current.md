# Milestone 6: Vehicle Composition

Status: in progress

The first backend-neutral vehicle slice is implemented in `vehicleCore`.
Normalized throttle, brake, steering, and handbrake intent is validated and
distributed into per-wheel steering, drive-torque, service-brake, and
handbrake commands. Wheel roles are expressed as independent ratios and
weights, so the contract supports front-, rear-, and all-wheel drive, rear
steering, and wheel counts other than four.

## Outcome

```text
named actions or behavior
    -> VehicleIntent
    -> vehicleCore wheel-command composition
    -> physics vehicle capability (next)
    -> Runtime World transform changes
    -> incremental USD synchronization
```

The current slice covers the first two arrows. It deliberately does not expose
Jolt types or author USD transforms directly.

## Remaining scope

### Physics application

- Add the minimum backend-neutral physics capability needed to consume wheel
  steering and torque commands.
- Implement that capability in `physicsJolt` without leaking Jolt types into
  `physicsCore` or `vehicleCore`.
- Compose chassis, wheel, suspension, steering, powertrain, and braking behavior
  without a monolithic four-wheel-only runtime object.

### USD declarations and Stage integration

- Introduce `RunnerVehicleAPI` and `RunnerWheelAPI` only with the importer that
  consumes them.
- Resolve vehicle and wheel prim relationships into the Runtime World and bind
  the chassis to its physics body.
- Convert named actions into `VehicleIntent` at the fixed-step boundary.
- Synchronize changed runtime transforms through the existing dirty queue and
  discardable runtime layer.

### Representative scenario

- Add `tests/fixtures/four_wheel_vehicle.usda`.
- Verify deterministic steering, forward/reverse drive, service braking, and
  handbraking through the complete Stage-to-runtime-to-Stage path.
- Keep at least one core test with a non-four-wheel layout so the public
  contract cannot accidentally narrow to a conventional car.

## Recommended PR sequence

1. `vehicleCore` intent, composition, validation, packaging, and unit tests
   (implemented on the current feature branch).
2. Backend-neutral physics vehicle capability and deterministic test double.
3. Jolt implementation and focused adapter tests.
4. Runner vehicle/wheel schemas and Stage importer.
5. Input mapping, four-wheel fixture, full vertical-slice tests, and docs.

## Completion criteria

- A USD-composed four-wheel vehicle is drivable with normalized input.
- Chassis, wheels, suspension, steering, powertrain, and braking remain
  independently configurable.
- The runtime contract supports other wheel counts and layouts.
- Vehicle motion uses physics and incremental runtime-layer synchronization.
- Core, adapter, integration, and representative fixture tests are
  deterministic.
- Plain CMake and OpenStrata build paths remain valid in supported
  environments.

The relevant contracts are documented in the
[runtime model](../design/runtime-model.md),
[input design](../design/input.md),
[module boundaries](../design/modules.md),
[USD integration](../design/usd-integration.md), and
[testing strategy](../design/testing.md).
