#pragma once

#include "usd_stage_runner/physics/handles.h"
#include "usd_stage_runner/runtime/runtime_transform.h"

#include <optional>

namespace usd_stage_runner::physics {

struct GroundContact {
  BodyHandle supportBody;
  runtime::Vec3d normal{0.0, 1.0, 0.0};
  double distance{0.0};
};

class GroundQuery {
public:
  virtual ~GroundQuery();

  [[nodiscard]] virtual std::optional<GroundContact>
  groundContact(BodyHandle body, double maxDistance) const = 0;
};

void validateGroundContact(const GroundContact& contact);

} // namespace usd_stage_runner::physics
