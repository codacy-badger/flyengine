
#pragma once

#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Resources/Resource.h>

class PL_RENDERERFOUNDATION_DLL plGALUnorderedAccessView : public plGALObject<plGALUnorderedAccessViewCreationDescription>
{
public:
  PL_ALWAYS_INLINE plGALResourceBase* GetResource() const { return m_pResource; }

protected:
  friend class plGALDevice;

  plGALUnorderedAccessView(plGALResourceBase* pResource, const plGALUnorderedAccessViewCreationDescription& description);

  virtual ~plGALUnorderedAccessView();

  virtual plResult InitPlatform(plGALDevice* pDevice) = 0;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) = 0;

  plGALResourceBase* m_pResource;
};
