#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Manipulators/CapsuleManipulatorAdapter.h>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plCapsuleManipulatorAdapter::plCapsuleManipulatorAdapter() = default;

plCapsuleManipulatorAdapter::~plCapsuleManipulatorAdapter() = default;

void plCapsuleManipulatorAdapter::Finalize()
{
  auto* pDoc = m_pObject->GetDocumentObjectManager()->GetDocument()->GetMainDocument();

  auto* pWindow = plQtDocumentWindow::FindWindowByDocument(pDoc);

  plQtEngineDocumentWindow* pEngineWindow = qobject_cast<plQtEngineDocumentWindow*>(pWindow);
  PL_ASSERT_DEV(pEngineWindow != nullptr, "Manipulators are only supported in engine document windows");

  m_Gizmo.SetTransformation(GetObjectTransform());
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);

  m_Gizmo.SetOwner(pEngineWindow, nullptr);

  m_Gizmo.m_GizmoEvents.AddEventHandler(plMakeDelegate(&plCapsuleManipulatorAdapter::GizmoEventHandler, this));
}

void plCapsuleManipulatorAdapter::Update()
{
  m_Gizmo.SetVisible(m_bManipulatorIsVisible);
  plObjectAccessorBase* pObjectAccessor = GetObjectAccessor();
  const plCapsuleManipulatorAttribute* pAttr = static_cast<const plCapsuleManipulatorAttribute*>(m_pManipulatorAttr);

  if (!pAttr->GetLengthProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetLengthProperty()));
    m_Gizmo.SetLength(fValue);
  }

  if (!pAttr->GetRadiusProperty().IsEmpty())
  {
    float fValue = pObjectAccessor->Get<float>(m_pObject, GetProperty(pAttr->GetRadiusProperty()));
    m_Gizmo.SetRadius(fValue);
  }

  m_Gizmo.SetTransformation(GetObjectTransform());
}

void plCapsuleManipulatorAdapter::GizmoEventHandler(const plGizmoEvent& e)
{
  switch (e.m_Type)
  {
    case plGizmoEvent::Type::BeginInteractions:
      BeginTemporaryInteraction();
      break;

    case plGizmoEvent::Type::CancelInteractions:
      CancelTemporayInteraction();
      break;

    case plGizmoEvent::Type::EndInteractions:
      EndTemporaryInteraction();
      break;

    case plGizmoEvent::Type::Interaction:
    {
      const plCapsuleManipulatorAttribute* pAttr = static_cast<const plCapsuleManipulatorAttribute*>(m_pManipulatorAttr);

      ChangeProperties(pAttr->GetLengthProperty(), m_Gizmo.GetLength(), pAttr->GetRadiusProperty(), m_Gizmo.GetRadius());
    }
    break;
  }
}

void plCapsuleManipulatorAdapter::UpdateGizmoTransform()
{
  m_Gizmo.SetTransformation(GetObjectTransform());
}
