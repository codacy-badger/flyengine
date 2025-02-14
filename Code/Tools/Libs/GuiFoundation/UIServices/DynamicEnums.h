#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <GuiFoundation/GuiFoundationDLL.h>

/// \brief Stores the valid values and names for 'dynamic' enums.
///
/// The names and valid values for dynamic enums may change due to user configuration changes.
/// The UI should show these user specified names without restarting the tool.
///
/// Call the static function GetDynamicEnum() to create or get the plDynamicEnum for a specific type.
class PL_GUIFOUNDATION_DLL plDynamicEnum
{
public:
  /// \brief Returns a plDynamicEnum under the given name. Creates a new one, if the name has not been used before.
  static plDynamicEnum& GetDynamicEnum(const char* szEnumName);

  /// \brief Returns all enum values and current names.
  const plMap<plInt32, plString>& GetAllValidValues() const { return m_ValidValues; }

  /// \brief Resets the internal data.
  void Clear();

  /// \brief Sets the name for the given enum value.
  void SetValueAndName(plInt32 iValue, plStringView sNewName);

  /// \brief Removes a certain enum value, if it exists.
  void RemoveValue(plInt32 iValue);

  /// \brief Returns whether a certain value is known.
  bool IsValueValid(plInt32 iValue) const;

  /// \brief Returns the name for the given value. Returns "<invalid value>" if the value is not in use.
  plStringView GetValueName(plInt32 iValue) const;

private:
  plMap<plInt32, plString> m_ValidValues;

  static plMap<plString, plDynamicEnum> s_DynamicEnums;
};
