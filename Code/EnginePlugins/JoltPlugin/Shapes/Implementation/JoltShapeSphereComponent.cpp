#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeSphereComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plJoltShapeSphereComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new plDefaultValueAttribute(0.5f), new plClampValueAttribute(0.0f, plVariant())),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plSphereManipulatorAttribute("Radius"),
    new plSphereVisualizerAttribute("Radius"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltShapeSphereComponent::plJoltShapeSphereComponent() = default;
plJoltShapeSphereComponent::~plJoltShapeSphereComponent() = default;

void plJoltShapeSphereComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_fRadius;
}

void plJoltShapeSphereComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = inout_stream.GetStream();
  s >> m_fRadius;
}

void plJoltShapeSphereComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), m_fRadius), plInvalidSpatialDataCategory);
}

void plJoltShapeSphereComponent::SetRadius(float f)
{
  m_fRadius = plMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plJoltShapeSphereComponent::CreateShapes(plDynamicArray<plJoltSubShape>& out_Shapes, const plTransform& rootTransform, float fDensity, const plJoltMaterial* pMaterial)
{
  auto pNewShape = new JPH::SphereShape(m_fRadius);
  pNewShape->AddRef();
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<plUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  plJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape = pNewShape;
  sub.m_Transform = plTransform::MakeLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
}


PL_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeSphereComponent);

