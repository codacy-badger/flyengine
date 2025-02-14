#include <RmlUiPlugin/RmlUiPluginPCH.h>

#include <Core/Utils/Blackboard.h>
#include <RmlUiPlugin/Implementation/BlackboardDataBinding.h>
#include <RmlUiPlugin/RmlUiContext.h>
#include <RmlUiPlugin/RmlUiConversionUtils.h>

namespace plRmlUiInternal
{
  BlackboardDataBinding::BlackboardDataBinding(const plSharedPtr<plBlackboard>& pBlackboard)
    : m_pBlackboard(pBlackboard)
  {
  }

  BlackboardDataBinding::~BlackboardDataBinding() = default;

  plResult BlackboardDataBinding::Initialize(Rml::Context& ref_context)
  {
    if (m_pBlackboard == nullptr)
      return PL_FAILURE;

    const char* szModelName = m_pBlackboard->GetName();
    if (plStringUtils::IsNullOrEmpty(szModelName))
    {
      plLog::Error("Can't bind a blackboard without a valid name");
      return PL_FAILURE;
    }

    Rml::DataModelConstructor constructor = ref_context.CreateDataModel(szModelName);
    if (!constructor)
    {
      return PL_FAILURE;
    }

    for (auto it : m_pBlackboard->GetAllEntries())
    {
      m_EntryWrappers.emplace_back(*m_pBlackboard, it.Key(), it.Value().m_uiChangeCounter);
    }

    for (auto& wrapper : m_EntryWrappers)
    {
      constructor.BindFunc(
        wrapper.m_sName.GetData(),
        [&](Rml::Variant& out_value) { wrapper.GetValue(out_value); },
        [&](const Rml::Variant& value) { wrapper.SetValue(value); });
    }

    m_hDataModel = constructor.GetModelHandle();

    m_uiBlackboardChangeCounter = m_pBlackboard->GetBlackboardChangeCounter();
    m_uiBlackboardEntryChangeCounter = m_pBlackboard->GetBlackboardEntryChangeCounter();

    return PL_SUCCESS;
  }

  void BlackboardDataBinding::Deinitialize(Rml::Context& ref_context)
  {
    if (m_pBlackboard != nullptr)
    {
      ref_context.RemoveDataModel(m_pBlackboard->GetName());
    }
  }

  void BlackboardDataBinding::Update()
  {
    if (m_uiBlackboardChangeCounter != m_pBlackboard->GetBlackboardChangeCounter())
    {
      plLog::Warning("Data Binding doesn't work with values that are registered or unregistered after setup");
      m_uiBlackboardChangeCounter = m_pBlackboard->GetBlackboardChangeCounter();
    }

    if (m_uiBlackboardEntryChangeCounter != m_pBlackboard->GetBlackboardEntryChangeCounter())
    {
      for (auto& wrapper : m_EntryWrappers)
      {
        auto pEntry = m_pBlackboard->GetEntry(wrapper.m_sName);

        if (pEntry != nullptr && wrapper.m_uiChangeCounter != pEntry->m_uiChangeCounter)
        {
          m_hDataModel.DirtyVariable(wrapper.m_sName.GetData());
          wrapper.m_uiChangeCounter = pEntry->m_uiChangeCounter;
        }
      }

      m_uiBlackboardEntryChangeCounter = m_pBlackboard->GetBlackboardEntryChangeCounter();
    }
  }

  //////////////////////////////////////////////////////////////////////////

  void BlackboardDataBinding::EntryWrapper::SetValue(const Rml::Variant& value)
  {
    plVariant::Type::Enum targetType = plVariant::Type::Invalid;
    if (auto pEntry = m_Blackboard.GetEntry(m_sName))
    {
      targetType = pEntry->m_Value.GetType();
    }

    m_Blackboard.SetEntryValue(m_sName, plRmlUiConversionUtils::ToVariant(value, targetType));
  }

  void BlackboardDataBinding::EntryWrapper::GetValue(Rml::Variant& out_value) const
  {
    out_value = plRmlUiConversionUtils::ToVariant(m_Blackboard.GetEntryValue(m_sName));
  }

} // namespace plRmlUiInternal
