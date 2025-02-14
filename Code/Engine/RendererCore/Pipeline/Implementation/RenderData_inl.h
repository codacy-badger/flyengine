#include <Core/World/GameObject.h>

PL_ALWAYS_INLINE plRenderData::Category::Category() = default;

PL_ALWAYS_INLINE plRenderData::Category::Category(plUInt16 uiValue)
  : m_uiValue(uiValue)
{
}

PL_ALWAYS_INLINE bool plRenderData::Category::operator==(const Category& other) const
{
  return m_uiValue == other.m_uiValue;
}

PL_ALWAYS_INLINE bool plRenderData::Category::operator!=(const Category& other) const
{
  return m_uiValue != other.m_uiValue;
}

//////////////////////////////////////////////////////////////////////////

// static
PL_FORCE_INLINE const plRenderer* plRenderData::GetCategoryRenderer(Category category, const plRTTI* pRenderDataType)
{
  if (s_bRendererInstancesDirty)
  {
    CreateRendererInstances();
  }

  auto& categoryData = s_CategoryData[category.m_uiValue];

  plUInt32 uiIndex = 0;
  if (categoryData.m_TypeToRendererIndex.TryGetValue(pRenderDataType, uiIndex))
  {
    return s_RendererInstances[uiIndex].Borrow();
  }

  return nullptr;
}

// static
PL_FORCE_INLINE plHashedString plRenderData::GetCategoryName(Category category)
{
  if (category.m_uiValue < s_CategoryData.GetCount())
  {
    return s_CategoryData[category.m_uiValue].m_sName;
  }

  return plHashedString();
}

PL_FORCE_INLINE plUInt64 plRenderData::GetCategorySortingKey(Category category, const plCamera& camera) const
{
  return s_CategoryData[category.m_uiValue].m_sortingKeyFunc(this, camera);
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
static T* plCreateRenderDataForThisFrame(const plGameObject* pOwner)
{
  PL_CHECK_AT_COMPILETIME(PL_IS_DERIVED_FROM_STATIC(plRenderData, T));

  T* pRenderData = PL_NEW(plFrameAllocator::GetCurrentAllocator(), T);

  if (pOwner != nullptr)
  {
    pRenderData->m_hOwner = pOwner->GetHandle();
  }

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  pRenderData->m_pOwner = pOwner;
#endif

  return pRenderData;
}
