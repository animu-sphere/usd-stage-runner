#include "usd_stage_runner/input_sdl/sdl_input_source.h"

#include "usd_stage_runner/input_sdl/physical_input.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>

#if USD_STAGE_RUNNER_HAS_SDL3
#include <SDL3/SDL.h>
#elif USD_STAGE_RUNNER_HAS_SDL2
#include <SDL.h>
#endif

namespace usd_stage_runner::input_sdl {
namespace {

constexpr double gamepadDeadZone = 0.15;

double normalizeAxis(std::int16_t value) {
  const double normalized = value < 0 ? static_cast<double>(value) / 32768.0
                                      : static_cast<double>(value) / 32767.0;
  if (std::abs(normalized) <= gamepadDeadZone) {
    return 0.0;
  }
  const double magnitude = (std::abs(normalized) - gamepadDeadZone) / (1.0 - gamepadDeadZone);
  return std::copysign(std::clamp(magnitude, 0.0, 1.0), normalized);
}

} // namespace

struct SdlInputSource::Impl {
  bool initialized{false};
  std::string errorMessage;

#if USD_STAGE_RUNNER_HAS_SDL3
  SDL_Window* window{nullptr};
  SDL_Gamepad* gamepad{nullptr};

  Impl() {
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
      errorMessage = SDL_GetError();
      return;
    }
    initialized = true;
    window = SDL_CreateWindow("usd-stage-runner", 640, 360, 0);
    if (window == nullptr) {
      errorMessage = SDL_GetError();
      return;
    }
  }

  ~Impl() {
    if (gamepad != nullptr) {
      SDL_CloseGamepad(gamepad);
    }
    if (window != nullptr) {
      SDL_DestroyWindow(window);
    }
    if (initialized) {
      SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD);
    }
  }

  void openFirstGamepad() {
    if (gamepad != nullptr) {
      return;
    }
    int count = 0;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&count);
    if (gamepads != nullptr && count > 0) {
      gamepad = SDL_OpenGamepad(gamepads[0]);
    }
    SDL_free(gamepads);
  }

  bool poll(input::ActionState& actions) {
    if (!initialized || window == nullptr) {
      actions.clear();
      return true;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
        return false;
      }
    }
    openFirstGamepad();

    const bool* keys = SDL_GetKeyboardState(nullptr);
    PhysicalInputState physical;
    physical.moveLeft = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    physical.moveRight = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
    physical.moveForward = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    physical.moveBackward = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];
    physical.keyboardJump = keys[SDL_SCANCODE_SPACE];
    if (gamepad != nullptr) {
      physical.gamepadMoveX = normalizeAxis(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX));
      physical.gamepadMoveY = -normalizeAxis(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY));
      physical.gamepadJump = SDL_GetGamepadButton(gamepad, SDL_GAMEPAD_BUTTON_SOUTH);
    }
    mapPhysicalInput(physical, actions);
    return true;
  }
#elif USD_STAGE_RUNNER_HAS_SDL2
  SDL_Window* window{nullptr};
  SDL_GameController* gamepad{nullptr};

  Impl() {
    if (SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0) {
      errorMessage = SDL_GetError();
      return;
    }
    initialized = true;
    window = SDL_CreateWindow("usd-stage-runner", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                              640, 360, SDL_WINDOW_SHOWN);
    if (window == nullptr) {
      errorMessage = SDL_GetError();
    }
  }

  ~Impl() {
    if (gamepad != nullptr) {
      SDL_GameControllerClose(gamepad);
    }
    if (window != nullptr) {
      SDL_DestroyWindow(window);
    }
    if (initialized) {
      SDL_QuitSubSystem(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    }
  }

  void openFirstGamepad() {
    if (gamepad != nullptr) {
      return;
    }
    const int count = SDL_NumJoysticks();
    for (int index = 0; index < count && gamepad == nullptr; ++index) {
      if (SDL_IsGameController(index)) {
        gamepad = SDL_GameControllerOpen(index);
      }
    }
  }

  bool poll(input::ActionState& actions) {
    if (!initialized || window == nullptr) {
      actions.clear();
      return true;
    }

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_QUIT) {
        return false;
      }
    }
    openFirstGamepad();

    const Uint8* keys = SDL_GetKeyboardState(nullptr);
    PhysicalInputState physical;
    physical.moveLeft = keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
    physical.moveRight = keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
    physical.moveForward = keys[SDL_SCANCODE_W] || keys[SDL_SCANCODE_UP];
    physical.moveBackward = keys[SDL_SCANCODE_S] || keys[SDL_SCANCODE_DOWN];
    physical.keyboardJump = keys[SDL_SCANCODE_SPACE] != 0;
    if (gamepad != nullptr) {
      physical.gamepadMoveX =
          normalizeAxis(SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTX));
      physical.gamepadMoveY =
          -normalizeAxis(SDL_GameControllerGetAxis(gamepad, SDL_CONTROLLER_AXIS_LEFTY));
      physical.gamepadJump =
          SDL_GameControllerGetButton(gamepad, SDL_CONTROLLER_BUTTON_A) != 0;
    }
    mapPhysicalInput(physical, actions);
    return true;
  }
#else
  Impl() : errorMessage("inputSdl was built without SDL2 or SDL3") {}
  bool poll(input::ActionState& actions) {
    actions.clear();
    return true;
  }
#endif
};

SdlInputSource::SdlInputSource() : impl_(std::make_unique<Impl>()) {}
SdlInputSource::~SdlInputSource() = default;
SdlInputSource::SdlInputSource(SdlInputSource&&) noexcept = default;
SdlInputSource& SdlInputSource::operator=(SdlInputSource&&) noexcept = default;

bool SdlInputSource::available() const noexcept {
  return impl_->initialized && impl_->errorMessage.empty();
}

std::string_view SdlInputSource::error() const noexcept {
  return impl_->errorMessage;
}

bool SdlInputSource::poll(input::ActionState& actions) {
  return impl_->poll(actions);
}

} // namespace usd_stage_runner::input_sdl
