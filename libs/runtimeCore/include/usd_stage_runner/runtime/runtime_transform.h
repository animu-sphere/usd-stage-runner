#pragma once

namespace usd_stage_runner::runtime {

struct Vec3d {
  double x{0.0};
  double y{0.0};
  double z{0.0};
};

struct RuntimeTransform {
  Vec3d translation;
};

} // namespace usd_stage_runner::runtime
