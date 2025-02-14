#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeBoxComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plJoltShapeBoxComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("HalfExtents", GetHalfExtents, SetHalfExtents)->AddAttributes(new plDefaultValueAttribute(plVec3(0.5f)), new plClampValueAttribute(plVec3(0), plVariant())),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plBoxManipulatorAttribute("HalfExtents", 2.0f, true),
    new plBoxVisualizerAttribute("HalfExtents", 2.0f),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltShapeBoxComponent::plJoltShapeBoxComponent() = default;
plJoltShapeBoxComponent::~plJoltShapeBoxComponent() = default;

void plJoltShapeBoxComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_vHalfExtents;
}

void plJoltShapeBoxComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();
  s >> m_vHalfExtents;
}

void plJoltShapeBoxComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(plBoundingBoxSphere::MakeFromBox(plBoundingBox::MakeFromMinMax(-m_vHalfExtents, m_vHalfExtents)), plInvalidSpatialDataCategory);
}

void plJoltShapeBoxComponent::ExtractGeometry(plMsgExtractGeometry& ref_msg) const
{
  ref_msg.AddBox(GetOwner()->GetGlobalTransform(), m_vHalfExtents * 2.0f);
}

void plJoltShapeBoxComponent::SetHalfExtents(const plVec3& value)
{
  m_vHalfExtents = value.CompMax(plVec3::MakeZero());

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plJoltShapeBoxComponent::CreateShapes(plDynamicArray<plJoltSubShape>& out_Shapes, const plTransform& rootTransform, float fDensity, const plJoltMaterial* pMaterial)
{
  // can't create boxes smaller than this
  plVec3 size = m_vHalfExtents;
  size.x = plMath::Max(size.x, JPH::cDefaultConvexRadius);
  size.y = plMath::Max(size.y, JPH::cDefaultConvexRadius);
  size.z = plMath::Max(size.z, JPH::cDefaultConvexRadius);

  auto pNewShape = new JPH::BoxShape(plJoltConversionUtils::ToVec3(size));
  pNewShape->AddRef();
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<plUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  plJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape = pNewShape;
  sub.m_Transform = plTransform::MakeLocalTransform(rootTransform, GetOwner()->GetGlobalTransform());
}


PL_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeBoxComponent);

