#pragma once

#include "usd_stage_runner/physics/handles.h"
#include "usd_stage_runner/runtime/runtime_transform.h"

#include <optional>

namespace usd_stage_runner::physics {

struct SegmentHit {
  BodyHandle body;
  // Normalized distance from origin (0) to target (1).
  double fraction{0.0};
};

// Optional backend capability for finding the first body along a world-space
// segment. The ignored body is useful for probes that start at a controlled
// character or other followed physics object.
class CollisionQuery {
public:
  virtual ~CollisionQuery();

  [[nodiscard]] virtual std::optional<SegmentHit>
  segmentHit(runtime::Vec3d origin, runtime::Vec3d target,
             BodyHandle ignoredBody = {}) const = 0;
};

void validateCollisionSegment(runtime::Vec3d origin, runtime::Vec3d target);
void validateSegmentHit(const SegmentHit& hit);

} // namespace usd_stage_runner::physics
