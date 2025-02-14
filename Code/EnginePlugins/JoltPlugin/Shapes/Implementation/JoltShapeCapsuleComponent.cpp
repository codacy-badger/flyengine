#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeCapsuleComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plJoltShapeCapsuleComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new plDefaultValueAttribute(0.25f), new plClampValueAttribute(0.0f, plVariant())),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCapsuleManipulatorAttribute("Height", "Radius"),
    new plCapsuleVisualizerAttribute("Height", "Radius"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltShapeCapsuleComponent::plJoltShapeCapsuleComponent() = default;
plJoltShapeCapsuleComponent::~plJoltShapeCapsuleComponent() = default;

void plJoltShapeCapsuleComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_fRadius;
  s << m_fHeight;
}

void plJoltShapeCapsuleComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = inout_stream.GetStream();
  s >> m_fRadius;
  s >> m_fHeight;
}

void plJoltShapeCapsuleComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(plBoundingSphere::MakeFromCenterAndRadius(plVec3(0, 0, -m_fHeight * 0.5f), m_fRadius), plInvalidSpatialDataCategory);
  msg.AddBounds(plBoundingSphere::MakeFromCenterAndRadius(plVec3(0, 0, +m_fHeight * 0.5f), m_fRadius), plInvalidSpatialDataCategory);
}

void plJoltShapeCapsuleComponent::SetRadius(float f)
{
  m_fRadius = plMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plJoltShapeCapsuleComponent::SetHeight(float f)
{
  m_fHeight = plMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plJoltShapeCapsuleComponent::CreateShapes(plDynamicArray<plJoltSubShape>& out_Shapes, const plTransform& rootTransform, float fDensity, const plJoltMaterial* pMaterial)
{
  JPH::Ref<JPH::CapsuleShape> pNewShape = new JPH::CapsuleShape(m_fHeight * 0.5f, m_fRadius);
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<plUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  JPH::Ref<JPH::RotatedTranslatedShapeSettings> pRotShapeSet = new JPH::RotatedTranslatedShapeSettings(JPH::Vec3::sZero(), JPH::Quat::sRotation(JPH::Vec3::sAxisX(), plAngle::MakeFromDegree(90).GetRadian()), pNewShape);

  JPH::Shape* pRotShape = pRotShapeSet->Create().Get().GetPtr();
  pRotShape->SetUserData(reinterpret_cast<plUInt64>(GetUserData()));

  plJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape = pRotShape;
  sub.m_pShape->AddRef();
  sub.m_Transform = plTransform::MakeLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
}


PL_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeCapsuleComponent);
