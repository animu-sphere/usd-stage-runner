#include "usd_stage_runner/physics_jolt/jolt_physics_world.h"

#include "usd_stage_runner/physics/collision_query.h"
#include "usd_stage_runner/physics/ground_query.h"

#include <Jolt/Jolt.h>

#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyFilter.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/NarrowPhaseQuery.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Constraints/FixedConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace usd_stage_runner::physics_jolt {
namespace {

namespace ObjectLayers {
constexpr JPH::ObjectLayer nonMoving = nonMovingCollisionLayer;
constexpr JPH::ObjectLayer moving = movingCollisionLayer;
constexpr JPH::ObjectLayer count = 2;
} // namespace ObjectLayers

namespace BroadPhaseLayers {
const JPH::BroadPhaseLayer nonMoving{0};
const JPH::BroadPhaseLayer moving{1};
constexpr JPH::uint count = 2;
} // namespace BroadPhaseLayers

class ObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter {
public:
  bool ShouldCollide(JPH::ObjectLayer first, JPH::ObjectLayer second) const override {
    if (first == ObjectLayers::nonMoving) {
      return second == ObjectLayers::moving;
    }
    return first == ObjectLayers::moving;
  }
};

class BroadPhaseLayerInterface final : public JPH::BroadPhaseLayerInterface {
public:
  JPH::uint GetNumBroadPhaseLayers() const override {
    return BroadPhaseLayers::count;
  }

  JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
    return layer == ObjectLayers::nonMoving ? BroadPhaseLayers::nonMoving
                                            : BroadPhaseLayers::moving;
  }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override {
    return layer == BroadPhaseLayers::nonMoving ? "non-moving" : "moving";
  }
#endif
};

class ObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
  bool ShouldCollide(JPH::ObjectLayer objectLayer,
                     JPH::BroadPhaseLayer broadPhaseLayer) const override {
    if (objectLayer == ObjectLayers::nonMoving) {
      return broadPhaseLayer == BroadPhaseLayers::moving;
    }
    return objectLayer == ObjectLayers::moving;
  }
};

std::mutex joltRuntimeMutex;
std::size_t joltRuntimeUsers = 0;

class JoltRuntime final {
public:
  JoltRuntime() {
    std::lock_guard<std::mutex> lock{joltRuntimeMutex};
    if (joltRuntimeUsers == 0) {
      if (JPH::Factory::sInstance != nullptr) {
        throw std::runtime_error("Jolt Physics was initialized outside physicsJolt");
      }
      JPH::RegisterDefaultAllocator();
      JPH::Factory::sInstance = new JPH::Factory();
      JPH::RegisterTypes();
    }
    ++joltRuntimeUsers;
  }

  ~JoltRuntime() {
    std::lock_guard<std::mutex> lock{joltRuntimeMutex};
    --joltRuntimeUsers;
    if (joltRuntimeUsers == 0) {
      JPH::UnregisterTypes();
      delete JPH::Factory::sInstance;
      JPH::Factory::sInstance = nullptr;
    }
  }

  JoltRuntime(const JoltRuntime&) = delete;
  JoltRuntime& operator=(const JoltRuntime&) = delete;
};

template <typename Handle, typename Value>
using HandleMap =
    std::unordered_map<Handle, Value, physics::PhysicsHandleHash<Handle>>;

runtime::Vec3d toRuntimeVector(JPH::Vec3Arg value) {
  return {static_cast<double>(value.GetX()), static_cast<double>(value.GetY()),
          static_cast<double>(value.GetZ())};
}

runtime::Vec3d toRuntimePosition(JPH::RVec3Arg value) {
  return {static_cast<double>(value.GetX()), static_cast<double>(value.GetY()),
          static_cast<double>(value.GetZ())};
}

JPH::Vec3 toJoltVector(runtime::Vec3d value) {
  return {static_cast<float>(value.x), static_cast<float>(value.y),
          static_cast<float>(value.z)};
}

JPH::RVec3 toJoltPosition(runtime::Vec3d value) {
  return {static_cast<JPH::Real>(value.x), static_cast<JPH::Real>(value.y),
          static_cast<JPH::Real>(value.z)};
}

bool stateChanged(const physics::BodyState& previous, const physics::BodyState& current) {
  return previous.transform.translation.x != current.transform.translation.x ||
         previous.transform.translation.y != current.transform.translation.y ||
         previous.transform.translation.z != current.transform.translation.z ||
         previous.linearVelocity.x != current.linearVelocity.x ||
         previous.linearVelocity.y != current.linearVelocity.y ||
         previous.linearVelocity.z != current.linearVelocity.z;
}

class JoltPhysicsWorld final : public physics::PhysicsWorld,
                               public physics::GroundQuery,
                               public physics::CollisionQuery {
public:
  JoltPhysicsWorld()
      : jobSystem_(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, workerThreadCount()) {
    physicsSystem_.Init(1024, 0, 1024, 1024, broadPhaseLayerInterface_,
                        objectVsBroadPhaseLayerFilter_, objectLayerPairFilter_);
  }

  ~JoltPhysicsWorld() override {
    auto& bodyInterface = physicsSystem_.GetBodyInterface();
    for (auto& entry : constraints_) {
      physicsSystem_.RemoveConstraint(entry.second.constraint.GetPtr());
    }
    constraints_.clear();
    for (auto& entry : bodies_) {
      bodyInterface.RemoveBody(entry.second.id);
      bodyInterface.DestroyBody(entry.second.id);
    }
  }

  physics::ShapeHandle createShape(const physics::ShapeDescriptor& descriptor) override {
    physics::validateShapeDescriptor(descriptor);
    JPH::ShapeRefC shape = new JPH::BoxShape(toJoltVector(descriptor.halfExtents));
    const physics::ShapeHandle handle{nextShape_++};
    shapes_.emplace(handle, ShapeRecord{std::move(shape)});
    return handle;
  }

  bool destroyShape(physics::ShapeHandle shape) noexcept override {
    for (const auto& entry : bodies_) {
      if (entry.second.shape == shape) {
        return false;
      }
    }
    return shapes_.erase(shape) != 0;
  }

  physics::BodyHandle createBody(const physics::BodyDescriptor& descriptor) override {
    physics::validateBodyDescriptor(descriptor);
    const auto shape = shapes_.find(descriptor.shape);
    if (shape == shapes_.end()) {
      throw std::invalid_argument("body references an unknown Jolt shape");
    }

    const bool dynamic = descriptor.motionType == physics::MotionType::dynamicBody;
    const physics::CollisionLayer expectedLayer =
        dynamic ? movingCollisionLayer : nonMovingCollisionLayer;
    if (descriptor.collisionLayer != expectedLayer) {
      throw std::invalid_argument(dynamic
                                      ? "dynamic Jolt bodies require the moving collision layer"
                                      : "static Jolt bodies require the non-moving collision layer");
    }
    JPH::BodyCreationSettings settings(
        shape->second.shape, toJoltPosition(descriptor.initialTransform.translation),
        JPH::Quat::sIdentity(), dynamic ? JPH::EMotionType::Dynamic : JPH::EMotionType::Static,
        static_cast<JPH::ObjectLayer>(descriptor.collisionLayer));
    if (dynamic) {
      settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
      settings.mMassPropertiesOverride.mMass = static_cast<float>(descriptor.mass);
    }

    JPH::Body* body = physicsSystem_.GetBodyInterface().CreateBody(settings);
    if (body == nullptr) {
      throw std::runtime_error("Jolt body capacity was exhausted");
    }
    physicsSystem_.GetBodyInterface().AddBody(
        body->GetID(), dynamic ? JPH::EActivation::Activate : JPH::EActivation::DontActivate);

    const physics::BodyHandle handle{nextBody_++};
    physics::BodyState initial{handle, descriptor.initialTransform, {}};
    bodies_.emplace(handle,
                    BodyRecord{body->GetID(), body, descriptor.shape, descriptor.motionType,
                               initial});
    return handle;
  }

  bool destroyBody(physics::BodyHandle body) noexcept override {
    const auto found = bodies_.find(body);
    if (found == bodies_.end()) {
      return false;
    }
    for (auto iterator = constraints_.begin(); iterator != constraints_.end();) {
      if (iterator->second.firstBody == body || iterator->second.secondBody == body) {
        physicsSystem_.RemoveConstraint(iterator->second.constraint.GetPtr());
        iterator = constraints_.erase(iterator);
      } else {
        ++iterator;
      }
    }
    auto& bodyInterface = physicsSystem_.GetBodyInterface();
    bodyInterface.RemoveBody(found->second.id);
    bodyInterface.DestroyBody(found->second.id);
    bodies_.erase(found);
    changed_.erase(body);
    return true;
  }

  physics::ConstraintHandle
  createConstraint(const physics::ConstraintDescriptor& descriptor) override {
    physics::validateConstraintDescriptor(descriptor);
    const auto first = bodies_.find(descriptor.firstBody);
    const auto second = bodies_.find(descriptor.secondBody);
    if (first == bodies_.end() || second == bodies_.end()) {
      throw std::invalid_argument("constraint references an unknown Jolt body");
    }

    JPH::FixedConstraintSettings settings;
    settings.mAutoDetectPoint = true;
    JPH::Ref<JPH::Constraint> constraint =
        settings.Create(*first->second.body, *second->second.body);
    physicsSystem_.AddConstraint(constraint.GetPtr());
    const physics::ConstraintHandle handle{nextConstraint_++};
    constraints_.emplace(handle, ConstraintRecord{descriptor.firstBody, descriptor.secondBody,
                                                   std::move(constraint)});
    return handle;
  }

  bool destroyConstraint(physics::ConstraintHandle constraint) noexcept override {
    const auto found = constraints_.find(constraint);
    if (found == constraints_.end()) {
      return false;
    }
    physicsSystem_.RemoveConstraint(found->second.constraint.GetPtr());
    constraints_.erase(found);
    return true;
  }

  bool applyForce(physics::BodyHandle body, runtime::Vec3d force) override {
    physics::validatePhysicsVector(force, "force");
    const auto found = bodies_.find(body);
    if (found == bodies_.end() || found->second.motionType != physics::MotionType::dynamicBody) {
      return false;
    }
    physicsSystem_.GetBodyInterface().AddForce(found->second.id, toJoltVector(force),
                                               JPH::EActivation::Activate);
    return true;
  }

  bool setLinearVelocity(physics::BodyHandle body, runtime::Vec3d velocity) override {
    physics::validatePhysicsVector(velocity, "linear velocity");
    const auto found = bodies_.find(body);
    if (found == bodies_.end() || found->second.motionType != physics::MotionType::dynamicBody) {
      return false;
    }
    physicsSystem_.GetBodyInterface().SetLinearVelocity(found->second.id,
                                                        toJoltVector(velocity));
    return true;
  }

  physics::BodyState bodyState(physics::BodyHandle body) const override {
    const auto found = bodies_.find(body);
    if (found == bodies_.end()) {
      throw std::out_of_range("unknown Jolt body handle");
    }
    return readBodyState(body, found->second);
  }

  std::optional<physics::GroundContact>
  groundContact(physics::BodyHandle body, double maxDistance) const override {
    if (!std::isfinite(maxDistance) || maxDistance < 0.0 ||
        maxDistance > static_cast<double>(std::numeric_limits<float>::max())) {
      throw std::invalid_argument(
          "Jolt ground probe distance must be finite and non-negative");
    }
    const auto found = bodies_.find(body);
    if (found == bodies_.end()) {
      throw std::out_of_range("unknown Jolt body handle");
    }

    const auto& bodyInterface = physicsSystem_.GetBodyInterface();
    const auto shape = shapes_.find(found->second.shape);
    if (shape == shapes_.end()) {
      throw std::logic_error("Jolt body references a missing shape");
    }

    const JPH::RShapeCast cast{
        shape->second.shape, JPH::Vec3::sOne(),
        bodyInterface.GetCenterOfMassTransform(found->second.id),
        JPH::Vec3{0.0f, -static_cast<float>(maxDistance), 0.0f}};
    JPH::ShapeCastSettings settings;
    settings.mReturnDeepestPoint = true;
    JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;
    const JPH::IgnoreSingleBodyFilter ignoreCharacter{found->second.id};
    physicsSystem_.GetNarrowPhaseQuery().CastShape(
        cast, settings, cast.mCenterOfMassStart.GetTranslation(), collector, {}, {},
        ignoreCharacter);
    if (!collector.HadHit()) {
      return std::nullopt;
    }

    const JPH::ShapeCastResult* groundHit = nullptr;
    JPH::Vec3 groundNormal;
    double groundDistance = 0.0;
    for (const auto& hit : collector.mHits) {
      const JPH::Vec3 normal =
          -hit.mPenetrationAxis.NormalizedOr(-JPH::Vec3::sAxisY());
      if (normal.GetY() <= 0.0f) {
        continue;
      }
      const double distance =
          std::max(0.0, static_cast<double>(hit.mFraction) * maxDistance);
      if (groundHit == nullptr || normal.GetY() > groundNormal.GetY() ||
          (normal.GetY() == groundNormal.GetY() && distance < groundDistance)) {
        groundHit = &hit;
        groundNormal = normal;
        groundDistance = distance;
      }
    }
    if (groundHit == nullptr) {
      return std::nullopt;
    }

    const auto support = bodyHandle(groundHit->mBodyID2);
    if (!support) {
      throw std::logic_error("Jolt ground query returned an untracked body");
    }
    physics::GroundContact contact{support, toRuntimeVector(groundNormal),
                                   groundDistance};
    physics::validateGroundContact(contact);
    return contact;
  }

  std::optional<physics::SegmentHit>
  segmentHit(runtime::Vec3d origin, runtime::Vec3d target,
             physics::BodyHandle ignoredBody) const override {
    physics::validateCollisionSegment(origin, target);
    const runtime::Vec3d displacement{target.x - origin.x, target.y - origin.y,
                                     target.z - origin.z};
    const JPH::RRayCast ray{toJoltPosition(origin), toJoltVector(displacement)};
    JPH::RayCastResult hit;
    bool hadHit = false;
    if (ignoredBody) {
      const auto ignored = bodies_.find(ignoredBody);
      if (ignored == bodies_.end()) {
        throw std::out_of_range("unknown ignored Jolt body handle");
      }
      const JPH::IgnoreSingleBodyFilter filter{ignored->second.id};
      hadHit = physicsSystem_.GetNarrowPhaseQuery().CastRay(ray, hit, {}, {}, filter);
    } else {
      hadHit = physicsSystem_.GetNarrowPhaseQuery().CastRay(ray, hit);
    }
    if (!hadHit) {
      return std::nullopt;
    }
    const auto body = bodyHandle(hit.mBodyID);
    if (!body) {
      throw std::logic_error("Jolt collision query returned an untracked body");
    }
    physics::SegmentHit result{body, static_cast<double>(hit.mFraction)};
    physics::validateSegmentHit(result);
    return result;
  }

  void step(Duration fixedStep) override {
    physics::validatePhysicsStep(fixedStep);
    const JPH::EPhysicsUpdateError updateError = physicsSystem_.Update(
        static_cast<float>(fixedStep.count()), 1, &tempAllocator_, &jobSystem_);
    if (updateError != JPH::EPhysicsUpdateError::None) {
      throw std::runtime_error("Jolt physics update exhausted collision capacity");
    }
    for (auto& entry : bodies_) {
      if (entry.second.motionType != physics::MotionType::dynamicBody) {
        continue;
      }
      auto current = readBodyState(entry.first, entry.second);
      if (stateChanged(entry.second.lastState, current)) {
        entry.second.lastState = current;
        changed_[entry.first] = std::move(current);
      }
    }
  }

  std::vector<physics::BodyState> takeChangedBodyStates() override {
    std::vector<physics::BodyState> result;
    result.reserve(changed_.size());
    for (auto& entry : changed_) {
      result.push_back(std::move(entry.second));
    }
    std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
      return left.body.value() < right.body.value();
    });
    changed_.clear();
    return result;
  }

private:
  struct ShapeRecord {
    JPH::ShapeRefC shape;
  };

  struct BodyRecord {
    JPH::BodyID id;
    JPH::Body* body;
    physics::ShapeHandle shape;
    physics::MotionType motionType;
    physics::BodyState lastState;
  };

  struct ConstraintRecord {
    physics::BodyHandle firstBody;
    physics::BodyHandle secondBody;
    JPH::Ref<JPH::Constraint> constraint;
  };

  static int workerThreadCount() {
    const unsigned int concurrency = std::thread::hardware_concurrency();
    return concurrency > 1 ? static_cast<int>(concurrency - 1) : 0;
  }

  physics::BodyState readBodyState(physics::BodyHandle handle,
                                   const BodyRecord& record) const {
    const auto& bodyInterface = physicsSystem_.GetBodyInterface();
    return {handle,
            runtime::RuntimeTransform{
                toRuntimePosition(bodyInterface.GetPosition(record.id))},
            toRuntimeVector(bodyInterface.GetLinearVelocity(record.id))};
  }

  physics::BodyHandle bodyHandle(JPH::BodyID id) const noexcept {
    for (const auto& entry : bodies_) {
      if (entry.second.id == id) {
        return entry.first;
      }
    }
    return {};
  }

  JoltRuntime runtime_;
  BroadPhaseLayerInterface broadPhaseLayerInterface_;
  ObjectVsBroadPhaseLayerFilter objectVsBroadPhaseLayerFilter_;
  ObjectLayerPairFilter objectLayerPairFilter_;
  JPH::TempAllocatorImpl tempAllocator_{10 * 1024 * 1024};
  JPH::JobSystemThreadPool jobSystem_;
  JPH::PhysicsSystem physicsSystem_;
  physics::ShapeHandle::ValueType nextShape_{1};
  physics::BodyHandle::ValueType nextBody_{1};
  physics::ConstraintHandle::ValueType nextConstraint_{1};
  HandleMap<physics::ShapeHandle, ShapeRecord> shapes_;
  HandleMap<physics::BodyHandle, BodyRecord> bodies_;
  HandleMap<physics::ConstraintHandle, ConstraintRecord> constraints_;
  HandleMap<physics::BodyHandle, physics::BodyState> changed_;
};

} // namespace

bool isJoltPhysicsAvailable() noexcept {
  return true;
}

std::unique_ptr<physics::PhysicsWorld> createJoltPhysicsWorld() {
  return std::make_unique<JoltPhysicsWorld>();
}

} // namespace usd_stage_runner::physics_jolt
