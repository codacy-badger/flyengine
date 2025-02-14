#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Strings/StringView.h>

using namespace plTokenParseUtils;

plResult plPreprocessor::Expand(const TokenStream& Tokens, TokenStream& Output)
{
  TokenStream Temp[2] = {TokenStream(&m_ClassAllocator), TokenStream(&m_ClassAllocator)};
  plInt32 iCur0 = 0;
  plInt32 iCur1 = 1;

  // first expansion
  if (ExpandOnce(Tokens, Temp[iCur0]).Failed())
    return PL_FAILURE;

  // second expansion
  if (ExpandOnce(Temp[iCur0], Temp[iCur1]).Failed())
    return PL_FAILURE;

  plInt32 iIterations = 1;

  // if they are not equal, at least two expansions are necessary
  while (Temp[iCur0] != Temp[iCur1])
  {
    if (iIterations > 10)
    {
      PP_LOG(Error, "Macro expansion reached {0} iterations", Tokens[0], iIterations);
      return PL_FAILURE;
    }

    iIterations++;

    plMath::Swap(iCur0, iCur1);

    // third and up
    Temp[iCur1].Clear();
    if (ExpandOnce(Temp[iCur0], Temp[iCur1]).Failed())
      return PL_FAILURE;
  }

  // Generally 2 iterations should be sufficient to handle most (all?) cases
  // the limit is currently very strict to detect whether there are macros that could require more expansions
  // if we can construct a macro that needs more iterations, this limit can easily be raised
  if (iIterations > 2)
  {
    PP_LOG(Warning, "Macro expansion reached {0} iterations", Tokens[0], iIterations);
  }

  Output.PushBackRange(Temp[iCur1]);

  return PL_SUCCESS;
}

plResult plPreprocessor::ExpandOnce(const TokenStream& Tokens, TokenStream& Output)
{
  const bool bIsOutermost = m_CurrentFileStack.PeekBack().m_iExpandDepth == 0;

  ++m_CurrentFileStack.PeekBack().m_iExpandDepth;

  for (plUInt32 uiCurToken = 0; uiCurToken < Tokens.GetCount();)
  {
    PL_ASSERT_DEV(Tokens[uiCurToken]->m_iType < s_iMacroParameter0, "Implementation error");

    // if we are not inside some macro expansion, but on the top level, adjust the line counter
    if (bIsOutermost)
    {
      m_CurrentFileStack.PeekBack().m_sVirtualFileName = Tokens[uiCurToken]->m_File;
      m_CurrentFileStack.PeekBack().m_iCurrentLine = (plInt32)Tokens[uiCurToken]->m_uiLine;
    }

    // if it is no identifier, it cannot be a macro -> just pass it through
    if (Tokens[uiCurToken]->m_iType != plTokenType::Identifier)
    {
      Output.PushBack(Tokens[uiCurToken]);
      ++uiCurToken;
      continue;
    }

    const plUInt32 uiIdentifierToken = uiCurToken;

    auto itMacro = m_Macros.Find(Tokens[uiIdentifierToken]->m_DataView);

    // no known macro name, or flagged as not to be expanded further -> pass through
    if (!itMacro.IsValid() || ((Tokens[uiCurToken]->m_uiCustomFlags & TokenFlags::NoFurtherExpansion) != 0))
    {
      Output.PushBack(Tokens[uiCurToken]);
      ++uiCurToken;
      continue;
    }

    // it is a valid macro name !

    if (!itMacro.Value().m_bIsFunction)
    {
      if (ExpandObjectMacro(itMacro.Value(), Output, Tokens[uiIdentifierToken]).Failed())
        return PL_FAILURE;

      ++uiCurToken; // move uiCurToken after the object macro token
      continue;
    }

    ++uiCurToken;

    SkipWhitespaceAndNewline(Tokens, uiCurToken);

    if (Accept(Tokens, uiCurToken, "("))
    {
      --uiCurToken;

      // we have a function macro -> extract all parameters, pre-expand them, then replace the macro body parameters and expand the macro itself

      MacroParameters AllParameters(&m_ClassAllocator);
      if (ExtractAllMacroParameters(Tokens, uiCurToken, AllParameters).Failed())
        return PL_FAILURE;

      // uiCurToken is now after the )

      if (ExpandFunctionMacro(itMacro.Value(), AllParameters, Output, Tokens[uiIdentifierToken]).Failed())
        return PL_FAILURE;

      continue;
    }
    else
    {
      // although the identifier is a function macro name, it is not used as a function macro -> just pass it through

      for (plUInt32 i = uiIdentifierToken; i < uiCurToken; ++i)
        Output.PushBack(Tokens[i]);

      continue;
    }
  }

  --m_CurrentFileStack.PeekBack().m_iExpandDepth;
  return PL_SUCCESS;
}

void plPreprocessor::OutputNotExpandableMacro(MacroDefinition& Macro, TokenStream& Output)
{
  const plStringView& sMacroName = Macro.m_MacroIdentifier->m_DataView;

  PL_ASSERT_DEV(Macro.m_bCurrentlyExpanding, "Implementation Error.");

  plToken* pNewToken = AddCustomToken(Macro.m_MacroIdentifier, sMacroName);
  pNewToken->m_uiCustomFlags = TokenFlags::NoFurtherExpansion;

  Output.PushBack(pNewToken);
}

plResult plPreprocessor::ExpandObjectMacro(MacroDefinition& Macro, TokenStream& Output, const plToken* pMacroToken)
{
  // when the macro is already being expanded, just pass the macro name through, but flag it as not to be expanded further
  if (Macro.m_bCurrentlyExpanding)
  {
    OutputNotExpandableMacro(Macro, Output);
    return PL_SUCCESS;
  }

  ProcessingEvent pe;
  pe.m_pToken = pMacroToken;
  pe.m_Type = ProcessingEvent::BeginExpansion;
  m_ProcessingEvents.Broadcast(pe);

  pe.m_Type = ProcessingEvent::EndExpansion;

  if (pMacroToken->m_DataView.IsEqual("__FILE__"))
  {
    PL_ASSERT_DEV(!m_CurrentFileStack.IsEmpty(), "Implementation error");

    plStringBuilder sName = "\"";
    sName.Append(m_CurrentFileStack.PeekBack().m_sVirtualFileName.GetView());
    sName.Append("\"");

    plToken* pNewToken = AddCustomToken(pMacroToken, sName);
    pNewToken->m_iType = plTokenType::String1;

    Output.PushBack(pNewToken);

    m_ProcessingEvents.Broadcast(pe);
    return PL_SUCCESS;
  }

  if (pMacroToken->m_DataView.IsEqual("__LINE__"))
  {
    plStringBuilder sLine;
    sLine.SetFormat("{0}", m_CurrentFileStack.PeekBack().m_iCurrentLine);

    plToken* pNewToken = AddCustomToken(pMacroToken, sLine);
    pNewToken->m_iType = plTokenType::Integer;

    Output.PushBack(pNewToken);

    m_ProcessingEvents.Broadcast(pe);
    return PL_SUCCESS;
  }

  Macro.m_bCurrentlyExpanding = true;

  if (Expand(Macro.m_Replacement, Output).Failed())
    return PL_FAILURE;

  Macro.m_bCurrentlyExpanding = false;

  m_ProcessingEvents.Broadcast(pe);
  return PL_SUCCESS;
}

void plPreprocessor::PassThroughFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, TokenStream& Output)
{
  OutputNotExpandableMacro(Macro, Output);

  Output.PushBack(m_pTokenOpenParenthesis);

  for (plUInt32 p = 0; p < Parameters.GetCount(); ++p)
  {
    /// \todo Maybe the passed through parameters need expansion

    Output.PushBackRange(Parameters[p]);

    if (p + 1 < Parameters.GetCount())
      Output.PushBack(m_pTokenComma);
  }

  Output.PushBack(m_pTokenClosedParenthesis);
}

plToken* plPreprocessor::CreateStringifiedParameter(plUInt32 uiParam, const plToken* pParamToken, const MacroDefinition& Macro)
{
  plStringBuilder sStringifiedParam;

  // if there were fewer parameters passed than the macro uses, replace the non-existing parameter by an empty string
  if (uiParam >= m_MacroParamStack.PeekBack()->GetCount())
    sStringifiedParam = "\"\"";
  else
  {
    // if we want to stringify the var-args parameters
    if (Macro.m_bHasVarArgs && uiParam + 1 == Macro.m_iNumParameters)
    {
      plStringBuilder sOneParam;

      sStringifiedParam = "\"";

      // stringify each parameter at the end, attach it to the output
      for (plUInt32 i = uiParam; i < m_MacroParamStack.PeekBack()->GetCount(); ++i)
      {
        StringifyTokens((*m_MacroParamStack.PeekBack())[i], sOneParam, false);

        if (i > uiParam) // second, third, etc.
          sStringifiedParam.Append(", ");

        sStringifiedParam.Append(sOneParam.GetView());
      }

      // remove redundant whitespace at end (can happen when having empty parameters
      while (sStringifiedParam.EndsWith(" "))
        sStringifiedParam.Shrink(0, 1);

      sStringifiedParam.Append("\"");
    }
    else
      StringifyTokens((*m_MacroParamStack.PeekBack())[uiParam], sStringifiedParam, true);
  }

  plToken* pStringifiedToken = AddCustomToken(pParamToken, sStringifiedParam);
  pStringifiedToken->m_iType = plTokenType::String1;
  return pStringifiedToken;
}

plResult plPreprocessor::InsertStringifiedParameters(const TokenStream& Tokens, TokenStream& Output, const MacroDefinition& Macro)
{
  plInt32 iConsecutiveHashes = 0;
  bool bLastTokenWasHash = false;
  bool bStringifyParameter = false;

  for (plUInt32 i = 0; i < Tokens.GetCount(); ++i)
  {
    const bool bTokenIsHash = (Tokens[i]->m_iType == plTokenType::NonIdentifier && plString(Tokens[i]->m_DataView) == "#");

    if (bTokenIsHash)
    {
      if (!bLastTokenWasHash)
        iConsecutiveHashes = 0;

      iConsecutiveHashes++;

      bLastTokenWasHash = true;
    }
    else
    {
      bLastTokenWasHash = false;

      // if there is an odd number of hashes, the last hash means 'stringify' the parameter
      bStringifyParameter = plMath::IsOdd(iConsecutiveHashes);
    }

    if (bStringifyParameter && Tokens[i]->m_iType < s_iMacroParameter0 && Tokens[i]->m_iType != plTokenType::Whitespace)
    {
      PP_LOG0(Error, "Expected a macro parameter name", Tokens[i]);
      return PL_FAILURE;
    }

    if (Tokens[i]->m_iType >= s_iMacroParameter0 && bStringifyParameter)
    {
      const plUInt32 uiParam = Tokens[i]->m_iType - s_iMacroParameter0;

      bStringifyParameter = false;

      plToken* pStringifiedToken = CreateStringifiedParameter(uiParam, Tokens[i], Macro);

      // remove all whitespace and the last # at the end of the output
      while (true)
      {
        if (Output.PeekBack()->m_iType == plTokenType::Whitespace)
          Output.PopBack();
        else
        {
          Output.PopBack();
          break;
        }
      }

      Output.PushBack(pStringifiedToken);
    }
    else
    {
      // we are also appending # signs as we go, they will be removed later again, if necessary
      Output.PushBack(Tokens[i]);
    }

    if (!bTokenIsHash && (Tokens[i]->m_iType != plTokenType::BlockComment) && (Tokens[i]->m_iType != plTokenType::LineComment) && (Tokens[i]->m_iType != plTokenType::Whitespace))
      iConsecutiveHashes = 0;
  }

  // hash at  the end of a macro is already forbidden as 'invalid character at end of macro'
  // so this case does not need to be handled here

  return PL_SUCCESS;
}

void plPreprocessor::MergeTokens(const plToken* pFirst, const plToken* pSecond, TokenStream& Output, const MacroDefinition& Macro)
{
  if (pFirst != nullptr && pFirst->m_iType >= s_iMacroParameter0)
  {
    plUInt32 uiParam = pFirst->m_iType - s_iMacroParameter0;

    if (uiParam < m_MacroParamStack.PeekBack()->GetCount())
    {
      // lovely var-args
      if (Macro.m_bHasVarArgs && uiParam + 1 == Macro.m_iNumParameters)
      {
        for (plUInt32 i = uiParam; i < m_MacroParamStack.PeekBack()->GetCount() - 1; ++i)
        {
          Output.PushBackRange((*m_MacroParamStack.PeekBack())[i]);
          Output.PushBack(m_pTokenComma);
        }

        uiParam = m_MacroParamStack.PeekBack()->GetCount() - 1;
      }

      for (plUInt32 i = 1; i < (*m_MacroParamStack.PeekBack())[uiParam].GetCount(); ++i)
        Output.PushBack((*m_MacroParamStack.PeekBack())[uiParam][i - 1]);

      if (!(*m_MacroParamStack.PeekBack())[uiParam].IsEmpty())
        pFirst = (*m_MacroParamStack.PeekBack())[uiParam].PeekBack();
      else
        pFirst = nullptr;
    }
    else
      pFirst = nullptr;
  }

  if (pSecond != nullptr && pSecond->m_iType >= s_iMacroParameter0)
  {
    const plUInt32 uiParam = pSecond->m_iType - s_iMacroParameter0;

    if (uiParam < m_MacroParamStack.PeekBack()->GetCount() && !(*m_MacroParamStack.PeekBack())[uiParam].IsEmpty())
    {
      MergeTokens(pFirst, (*m_MacroParamStack.PeekBack())[uiParam][0], Output, Macro);

      for (plUInt32 i = 1; i < (*m_MacroParamStack.PeekBack())[uiParam].GetCount(); ++i)
        Output.PushBack((*m_MacroParamStack.PeekBack())[uiParam][i]);

      // lovely var-args
      if (Macro.m_bHasVarArgs && uiParam + 1 == Macro.m_iNumParameters)
      {
        for (plUInt32 i = uiParam + 1; i < m_MacroParamStack.PeekBack()->GetCount(); ++i)
        {
          Output.PushBack(m_pTokenComma);
          Output.PushBackRange((*m_MacroParamStack.PeekBack())[i]);
        }
      }
    }
    else
    {
      MergeTokens(pFirst, nullptr, Output, Macro);
    }

    return;
  }

  if (pFirst == nullptr || pSecond == nullptr || (pFirst->m_iType != plTokenType::Identifier && pFirst->m_iType != plTokenType::Integer) || (pSecond->m_iType != plTokenType::Identifier && pSecond->m_iType != plTokenType::Integer))
  {
    if (pFirst != nullptr)
      Output.PushBack(pFirst);

    if (pSecond != nullptr)
      Output.PushBack(pSecond);

    return;
  }

  plStringBuilder sMerged;
  sMerged.Append(pFirst->m_DataView);
  sMerged.Append(pSecond->m_DataView);

  const plToken* pMergedToken = AddCustomToken(pFirst, sMerged);

  Output.PushBack(pMergedToken);
}

plResult plPreprocessor::ConcatenateParameters(const TokenStream& Tokens, TokenStream& Output, const MacroDefinition& Macro)
{
  plUInt32 uiCurToken = 0;


  while (uiCurToken < Tokens.GetCount())
  {
    plUInt32 uiConcatToken = uiCurToken;

    // do this extra check for whitespace here, because 'Accept' would just skip it, but we want the whitespace in our output
    if (Tokens[uiCurToken]->m_iType != plTokenType::Whitespace && Tokens[uiCurToken]->m_iType != plTokenType::Newline && Accept(Tokens, uiCurToken, "#", "#", &uiConcatToken))
    {
      // we have already removed all single hashes during the stringification, so here we will only encounter double hashes
      // (and quadruple, etc.)

      while (Accept(Tokens, uiCurToken, "#", "#"))
      { /* remove all double hashes ##, also skip whitespace in between */
      }

      // remove whitespace at end of current output
      while (!Output.IsEmpty() && (Output.PeekBack()->m_iType == plTokenType::Whitespace || Output.PeekBack()->m_iType == plTokenType::Newline))
        Output.PopBack();

      if (Output.IsEmpty())
      {
        PP_LOG0(Error, "## cannot occur at the beginning of a macro definition", Tokens[uiConcatToken]);
        return PL_FAILURE;
      }

      const plToken* pFirstToken = Output.PeekBack();
      Output.PopBack();

      if (uiCurToken >= Tokens.GetCount())
      {
        PP_LOG0(Error, "## cannot occur at the end of a macro definition", Tokens[uiConcatToken]);
        return PL_FAILURE;
      }

      const plToken* pSecondToken = Tokens[uiCurToken];

      MergeTokens(pFirstToken, pSecondToken, Output, Macro);
    }
    else
    {
      // output will not contain whitespace
      Output.PushBack(Tokens[uiCurToken]);
    }

    ++uiCurToken;
  }

  return PL_SUCCESS;
}

plResult plPreprocessor::InsertParameters(const TokenStream& Tokens, TokenStream& Output, const MacroDefinition& Macro)
{
  TokenStream Stringified(&m_ClassAllocator);
  if (InsertStringifiedParameters(Tokens, Stringified, Macro).Failed())
    return PL_FAILURE;

  TokenStream Concatenated(&m_ClassAllocator);
  if (ConcatenateParameters(Stringified, Concatenated, Macro).Failed())
    return PL_FAILURE;


  for (plUInt32 i = 0; i < Concatenated.GetCount(); ++i)
  {
    if (Concatenated[i]->m_iType >= s_iMacroParameter0)
    {
      const plUInt32 uiParam = Concatenated[i]->m_iType - s_iMacroParameter0;

      if (ExpandMacroParam(*Concatenated[i], uiParam, Output, Macro).Failed())
        return PL_FAILURE;
    }
    else
    {
      Output.PushBack(Concatenated[i]);
    }
  }

  return PL_SUCCESS;
}

plResult plPreprocessor::ExpandFunctionMacro(MacroDefinition& Macro, const MacroParameters& Parameters, TokenStream& Output, const plToken* pMacroToken)
{
  // when the macro is already being expanded, just pass the macro name through, but flag it as not to be expanded further
  if (Macro.m_bCurrentlyExpanding)
  {
    // for the function macro, also output the parameter list
    PassThroughFunctionMacro(Macro, Parameters, Output);
    return PL_SUCCESS;
  }

  ProcessingEvent pe;
  pe.m_pToken = pMacroToken;
  pe.m_Type = ProcessingEvent::BeginExpansion;
  m_ProcessingEvents.Broadcast(pe);

  pe.m_Type = ProcessingEvent::EndExpansion;

  MacroParameters ExpandedParameters(&m_ClassAllocator);
  ExpandedParameters.SetCount(Parameters.GetCount());

  for (plUInt32 i = 0; i < Parameters.GetCount(); ++i)
  {
    if (Expand(Parameters[i], ExpandedParameters[i]).Failed())
      return PL_FAILURE;
  }

  m_MacroParamStackExpanded.PushBack(&ExpandedParameters);
  m_MacroParamStack.PushBack(&Parameters);

  Macro.m_bCurrentlyExpanding = true;

  TokenStream MacroOutput(&m_ClassAllocator);
  if (InsertParameters(Macro.m_Replacement, MacroOutput, Macro).Failed())
    return PL_FAILURE;

  if (Expand(MacroOutput, Output).Failed())
    return PL_FAILURE;

  Macro.m_bCurrentlyExpanding = false;

  m_MacroParamStack.PopBack();
  m_MacroParamStackExpanded.PopBack();

  m_ProcessingEvents.Broadcast(pe);

  return PL_SUCCESS;
}

plResult plPreprocessor::ExpandMacroParam(const plToken& MacroToken, plUInt32 uiParam, TokenStream& Output, const MacroDefinition& Macro)
{
  PL_ASSERT_DEV(!m_MacroParamStack.IsEmpty(), "Implementation error.");

  const MacroParameters& ParamsExpanded = *m_MacroParamStackExpanded.PeekBack();

  if (uiParam >= ParamsExpanded.GetCount())
  {
    plToken* pWhitespace = AddCustomToken(&MacroToken, "");
    pWhitespace->m_iType = plTokenType::Whitespace;

    Output.PushBack(pWhitespace);

    PP_LOG(Warning, "Trying to access parameter {0}, but only {1} parameters were passed along", (&MacroToken), uiParam, ParamsExpanded.GetCount());
    return PL_SUCCESS;
  }
  else if (uiParam + 1 == Macro.m_iNumParameters && Macro.m_bHasVarArgs)
  {
    // insert all vararg parameters here

    for (plUInt32 i = uiParam; i < ParamsExpanded.GetCount(); ++i)
    {
      Output.PushBackRange(ParamsExpanded[i]);

      if (i + 1 < ParamsExpanded.GetCount())
        Output.PushBack(m_pTokenComma);
    }

    return PL_SUCCESS;
  }

  Output.PushBackRange(ParamsExpanded[uiParam]);

  return PL_SUCCESS;
}


