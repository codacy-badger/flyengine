// NO #pragma once in this file !

template <typename R PL_COMMA_IF(ARG_COUNT) PL_LIST(typename P, ARG_COUNT)>
class plConsoleFunction<R(PL_LIST(P, ARG_COUNT))> : public plConsoleFunctionBase
{
public:
  using FUNC = plDelegate<R(PL_LIST(P, ARG_COUNT))>;

  FUNC m_Func;

  plConsoleFunction(plStringView sFunctionName, plStringView sDescription, FUNC f)
    : plConsoleFunctionBase(sFunctionName, sDescription)
  {
    m_Func = f;
  }

  plUInt32 GetNumParameters() const override { return ARG_COUNT; }

  virtual plVariant::Type::Enum GetParameterType(plUInt32 uiParam) const override
  {
    PL_ASSERT_DEV(uiParam < GetNumParameters(), "Invalid Parameter Index {0}", uiParam);

#if (ARG_COUNT > 0)

    switch (uiParam)
    {
      case 0:
        return static_cast<plVariant::Type::Enum>(plVariant::TypeDeduction<P0>::value);

#  if (ARG_COUNT > 1)
      case 1:
        return static_cast<plVariant::Type::Enum>(plVariant::TypeDeduction<P1>::value);
#  endif
#  if (ARG_COUNT > 2)
      case 2:
        return static_cast<plVariant::Type::Enum>(plVariant::TypeDeduction<P2>::value);
#  endif
#  if (ARG_COUNT > 3)
      case 3:
        return static_cast<plVariant::Type::Enum>(plVariant::TypeDeduction<P3>::value);
#  endif
#  if (ARG_COUNT > 4)
      case 4:
        return static_cast<plVariant::Type::Enum>(plVariant::TypeDeduction<P4>::value);
#  endif
#  if (ARG_COUNT > 5)
      case 5:
        return static_cast<plVariant::Type::Enum>(plVariant::TypeDeduction<P5>::value);
#  endif
    }

#endif
    return plVariant::Type::Invalid;
  }

  virtual plResult Call(plArrayPtr<plVariant> params) override
  {
    plResult r = PL_FAILURE;
    PL_IGNORE_UNUSED(r);

#if (ARG_COUNT > 0)
    P0 param0 = params[0].ConvertTo<P0>(&r);

    if (r.Failed())
      return PL_FAILURE;
#endif

#if (ARG_COUNT > 1)
    P1 param1 = params[1].ConvertTo<P1>(&r);

    if (r.Failed())
      return PL_FAILURE;
#endif

#if (ARG_COUNT > 2)
    P2 param2 = params[2].ConvertTo<P2>(&r);

    if (r.Failed())
      return PL_FAILURE;
#endif

#if (ARG_COUNT > 3)
    P3 param3 = params[3].ConvertTo<P3>(&r);

    if (r.Failed())
      return PL_FAILURE;
#endif

#if (ARG_COUNT > 4)
    P4 param4 = params[4].ConvertTo<P4>(&r);

    if (r.Failed())
      return PL_FAILURE;
#endif

#if (ARG_COUNT > 5)
    P5 param5 = params[5].ConvertTo<P5>(&r);

    if (r.Failed())
      return PL_FAILURE;
#endif

    m_Func(PL_LIST(param, ARG_COUNT));
    return PL_SUCCESS;
  }
};
