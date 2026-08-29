#include "usd_stage_runner/runtime/fixed_step_accumulator.h"
#include "usd_stage_runner/runtime/frame_clock.h"
#include "usd_stage_runner/runtime/runtime_world.h"

#if USD_STAGE_RUNNER_HAS_OPENUSD
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usd/stage.h>
#endif

#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>

namespace {

using usd_stage_runner::runtime::FixedStepAccumulator;
using usd_stage_runner::runtime::FrameClock;
using usd_stage_runner::runtime::RuntimeWorld;

struct Options {
  std::filesystem::path stagePath;
  std::size_t frameCount{300};
  double fixedDt{1.0 / 60.0};
  std::size_t maxFixedSteps{8};
  bool deterministic{false};
};

[[noreturn]] void usageError(const std::string& message) {
  throw std::invalid_argument(
      message + "\nusage: stage_runner <scene.usd[a|c]> [--frames N] [--fixed-dt SECONDS]"
                " [--max-fixed-steps N] [--deterministic]");
}

std::size_t parsePositiveSize(const std::string& value, const std::string& option) {
  std::size_t parsed = 0;
  const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
  if (result.ec != std::errc{} || result.ptr != value.data() + value.size() || parsed == 0) {
    usageError(option + " requires a positive integer");
  }
  return static_cast<std::size_t>(parsed);
}

Options parseOptions(int argc, char** argv) {
  if (argc < 2) {
    usageError("a Stage path is required");
  }

  Options options;
  options.stagePath = argv[1];
  for (int index = 2; index < argc; ++index) {
    const std::string argument = argv[index];
    if (argument == "--deterministic") {
      options.deterministic = true;
    } else if (argument == "--frames" || argument == "--max-fixed-steps" ||
               argument == "--fixed-dt") {
      if (++index >= argc) {
        usageError(argument + " requires a value");
      }
      const std::string value = argv[index];
      if (argument == "--frames") {
        options.frameCount = parsePositiveSize(value, argument);
      } else if (argument == "--max-fixed-steps") {
        options.maxFixedSteps = parsePositiveSize(value, argument);
      } else {
        std::size_t consumed = 0;
        options.fixedDt = std::stod(value, &consumed);
        if (consumed != value.size() || !std::isfinite(options.fixedDt) || options.fixedDt <= 0.0) {
          usageError(argument + " requires a positive number");
        }
      }
    } else {
      usageError("unknown option: " + argument);
    }
  }
  return options;
}

#if USD_STAGE_RUNNER_HAS_OPENUSD
RuntimeWorld openWorld(const std::filesystem::path& stagePath) {
  const auto stage = pxr::UsdStage::Open(stagePath.string());
  if (!stage) {
    throw std::runtime_error("could not open USD Stage: " + stagePath.string());
  }

  RuntimeWorld world;
  for (const auto& prim : stage->Traverse()) {
    world.addPrim(prim.GetPath().GetString());
  }
  return world;
}
#endif

int run(const Options& options) {
#if !USD_STAGE_RUNNER_HAS_OPENUSD
  (void)options;
  throw std::runtime_error(
      "stage_runner was built without OpenUSD; configure with an OpenUSD SDK or an "
      "OpenStrata usd runtime");
#else
  RuntimeWorld world = openWorld(options.stagePath);
  const FixedStepAccumulator::Duration fixedStep{options.fixedDt};
  FixedStepAccumulator accumulator(fixedStep, options.maxFixedSteps);
  FrameClock clock;
  std::size_t fixedSteps = 0;
  std::size_t droppedSteps = 0;

  auto nextFrame = FrameClock::Clock::now();
  (void)clock.tick();
  for (std::size_t frame = 0; frame < options.frameCount; ++frame) {
    FrameClock::Duration frameTime;
    if (options.deterministic) {
      frameTime = fixedStep;
    } else {
      nextFrame += std::chrono::duration_cast<FrameClock::Clock::duration>(fixedStep);
      std::this_thread::sleep_until(nextFrame);
      frameTime = clock.tick();
    }

    const auto result = accumulator.advance(frameTime, [&](const auto) { ++fixedSteps; });
    droppedSteps += result.droppedSteps;
  }

  std::cout << "opened " << options.stagePath.string() << " with " << world.primCount()
            << " prims; frames=" << options.frameCount << ", fixed_steps=" << fixedSteps
            << ", dropped_steps=" << droppedSteps << '\n';
  return 0;
#endif
}

} // namespace

int main(int argc, char** argv) {
  try {
    return run(parseOptions(argc, argv));
  } catch (const std::exception& error) {
    std::cerr << "stage_runner: " << error.what() << '\n';
    return 2;
  }
}
