#pragma once

#include <GuiFoundation/GuiFoundationDLL.h>

#include <Foundation/Communication/Event.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/Variant.h>
#include <GuiFoundation/PropertyGrid/PropertyBaseWidget.moc.h>

#include <QFrame>
#include <QLabel>

class QCheckBox;
class QDoubleSpinBox;
class QSpinBox;
class QLabel;
class QHBoxLayout;
class QLineEdit;
class QPushButton;
class QComboBox;
class QStandardItemModel;
class QStandardItem;
class QToolButton;
class QMenu;
class plDocumentObject;
class plQtDoubleSpinBox;
class QSlider;

/// *** CHECKBOX ***

class PL_GUIFOUNDATION_DLL plQtPropertyEditorCheckboxWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtPropertyEditorCheckboxWidget();

  virtual void mousePressEvent(QMouseEvent* pEv) override;

private Q_SLOTS:
  void on_StateChanged_triggered(int state);

protected:
  virtual void OnInit() override {}
  virtual void InternalSetValue(const plVariant& value) override;

  QHBoxLayout* m_pLayout;
  QCheckBox* m_pWidget;
};



/// *** DOUBLE SPINBOX ***

class PL_GUIFOUNDATION_DLL plQtPropertyEditorDoubleSpinboxWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtPropertyEditorDoubleSpinboxWidget(plInt8 iNumComponents);

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;

  bool m_bUseTemporaryTransaction = false;
  bool m_bTemporaryCommand = false;
  plInt8 m_iNumComponents = 0;
  plEnum<plVariantType> m_OriginalType;
  QHBoxLayout* m_pLayout = nullptr;
  plQtDoubleSpinBox* m_pWidget[4] = {};
  QLabel* m_pWidgetLabel[4] = {};
  const char m_cAxisNames[4] = {'X', 'Y', 'Z', 'W'};
  const char* m_pWidgetLabelColors[4] = {"#63000D", "#006317", "#000863", "transparent"};
};

/// *** TIME SPINBOX ***

class PL_GUIFOUNDATION_DLL plQtPropertyEditorTimeWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtPropertyEditorTimeWidget();

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;

  bool m_bTemporaryCommand;
  QHBoxLayout* m_pLayout;
  plQtDoubleSpinBox* m_pWidget;
};

/// *** ANGLE SPINBOX ***

class PL_GUIFOUNDATION_DLL plQtPropertyEditorAngleWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtPropertyEditorAngleWidget();

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;

  bool m_bTemporaryCommand;
  QHBoxLayout* m_pLayout;
  plQtDoubleSpinBox* m_pWidget;
};

/// *** INT SPINBOX ***

class PL_GUIFOUNDATION_DLL plQtPropertyEditorIntSpinboxWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtPropertyEditorIntSpinboxWidget(plInt8 iNumComponents, plInt32 iMinValue, plInt32 iMaxValue);
  ~plQtPropertyEditorIntSpinboxWidget();

private Q_SLOTS:
  void SlotValueChanged();
  void SlotSliderValueChanged(int value);
  void on_EditingFinished_triggered();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;

  bool m_bUseTemporaryTransaction = false;
  bool m_bTemporaryCommand = false;
  plInt8 m_iNumComponents = 0;
  plEnum<plVariantType> m_OriginalType;
  QHBoxLayout* m_pLayout = nullptr;
  plQtDoubleSpinBox* m_pWidget[4] = {};
  QSlider* m_pSlider = nullptr;
};

/// *** SLIDER ***

class PL_GUIFOUNDATION_DLL plQtImageSliderWidget : public QWidget
{
  Q_OBJECT
public:
  using ImageGeneratorFunc = QImage (*)(plUInt32 uiWidth, plUInt32 uiHeight, double fMinValue, double fMaxValue);

  plQtImageSliderWidget(ImageGeneratorFunc generator, double fMinValue, double fMaxValue, QWidget* pParent);

  static plMap<plString, ImageGeneratorFunc> s_ImageGenerators;

  double GetValue() const { return m_fValue; }
  void SetValue(double fValue);

Q_SIGNALS:
  void valueChanged(double x);
  void sliderReleased();

protected:
  virtual void paintEvent(QPaintEvent*) override;
  virtual void mouseMoveEvent(QMouseEvent*) override;
  virtual void mousePressEvent(QMouseEvent*) override;
  virtual void mouseReleaseEvent(QMouseEvent*) override;

  void UpdateImage();

  ImageGeneratorFunc m_Generator = nullptr;
  QImage m_Image;
  double m_fValue = 0;
  double m_fMinValue = 0;
  double m_fMaxValue = 0;
};

class PL_GUIFOUNDATION_DLL plQtPropertyEditorSliderWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtPropertyEditorSliderWidget();
  ~plQtPropertyEditorSliderWidget();

private Q_SLOTS:
  void SlotSliderValueChanged(double fValue);
  void on_EditingFinished_triggered();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;

  bool m_bTemporaryCommand = false;
  plEnum<plVariantType> m_OriginalType;
  QHBoxLayout* m_pLayout = nullptr;
  plQtImageSliderWidget* m_pSlider = nullptr;

  double m_fMinValue = 0;
  double m_fMaxValue = 0;
};

/// *** QUATERNION ***

class PL_GUIFOUNDATION_DLL plQtPropertyEditorQuaternionWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtPropertyEditorQuaternionWidget();

private Q_SLOTS:
  void on_EditingFinished_triggered();
  void SlotValueChanged();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;

protected:
  bool m_bTemporaryCommand;
  QHBoxLayout* m_pLayout;
  plQtDoubleSpinBox* m_pWidget[3];
  QLabel* m_pWidgetLabel[3];
  const char m_cAxisNames[3] = {'X', 'Y', 'Z'};
  const char* m_pWidgetLabelColors[3] = {"#63000D", "#006317", "#000863"};
};


/// *** LINEEDIT ***

class PL_GUIFOUNDATION_DLL plQtPropertyEditorLineEditWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtPropertyEditorLineEditWidget();

protected Q_SLOTS:
  void on_TextChanged_triggered(const QString& value);
  void on_TextFinished_triggered();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;

protected:
  QHBoxLayout* m_pLayout;
  QLineEdit* m_pWidget;
  plEnum<plVariantType> m_OriginalType;
};


/// *** COLOR ***

class PL_GUIFOUNDATION_DLL plQtColorButtonWidget : public QFrame
{
  Q_OBJECT

public:
  explicit plQtColorButtonWidget(QWidget* pParent);
  void SetColor(const plVariant& color);

Q_SIGNALS:
  void clicked();

protected:
  virtual void showEvent(QShowEvent* event) override;
  virtual void mouseReleaseEvent(QMouseEvent* event) override;

  virtual QSize sizeHint() const override;
  virtual QSize minimumSizeHint() const override;

private:
  QPalette m_Pal;
};

class PL_GUIFOUNDATION_DLL plQtPropertyEditorColorWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtPropertyEditorColorWidget();

private Q_SLOTS:
  void on_Button_triggered();
  void on_CurrentColor_changed(const plColor& color);
  void on_Color_reset();
  void on_Color_accepted();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;

protected:
  bool m_bExposeAlpha;
  QHBoxLayout* m_pLayout;
  plQtColorButtonWidget* m_pWidget;
  plVariant m_OriginalValue;
};


/// *** ENUM COMBOBOX ***

class PL_GUIFOUNDATION_DLL plQtPropertyEditorEnumWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtPropertyEditorEnumWidget();

private Q_SLOTS:
  void on_CurrentEnum_changed(int iEnum);

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;

protected:
  QHBoxLayout* m_pLayout;
  QComboBox* m_pWidget;
  plInt64 m_iCurrentEnum;
};


/// *** BITFLAGS COMBOBOX ***

class PL_GUIFOUNDATION_DLL plQtPropertyEditorBitflagsWidget : public plQtStandardPropertyWidget
{
  Q_OBJECT

public:
  plQtPropertyEditorBitflagsWidget();
  virtual ~plQtPropertyEditorBitflagsWidget();

private Q_SLOTS:
  void on_Menu_aboutToShow();
  void on_Menu_aboutToHide();

protected:
  virtual void OnInit() override;
  virtual void InternalSetValue(const plVariant& value) override;
  void SetAllChecked(bool bChecked);

protected:
  plMap<plInt64, QCheckBox*> m_Constants;
  QHBoxLayout* m_pLayout = nullptr;
  QPushButton* m_pWidget = nullptr;
  QPushButton* m_pAllButton = nullptr;
  QPushButton* m_pClearButton = nullptr;
  QMenu* m_pMenu = nullptr;
  plInt64 m_iCurrentBitflags = 0;
};


/// *** CURVE1D ***

class PL_GUIFOUNDATION_DLL plQtCurve1DButtonWidget : public QLabel
{
  Q_OBJECT

public:
  explicit plQtCurve1DButtonWidget(QWidget* pParent);

  void UpdatePreview(plObjectAccessorBase* pObjectAccessor, const plDocumentObject* pCurveObject, QColor color, double fLowerExtents, bool bLowerFixed, double fUpperExtents, bool bUpperFixed, double fDefaultValue, double fLowerRange, double fUpperRange);

Q_SIGNALS:
  void clicked();

protected:
  virtual void mouseReleaseEvent(QMouseEvent* event) override;
};

class PL_GUIFOUNDATION_DLL plQtPropertyEditorCurve1DWidget : public plQtPropertyWidget
{
  Q_OBJECT

public:
  plQtPropertyEditorCurve1DWidget();

private Q_SLOTS:
  void on_Button_triggered();

protected:
  virtual void SetSelection(const plHybridArray<plPropertySelection, 8>& items) override;
  virtual void OnInit() override;
  virtual void DoPrepareToDie() override;
  void UpdatePreview();

protected:
  QHBoxLayout* m_pLayout = nullptr;
  plQtCurve1DButtonWidget* m_pButton = nullptr;
};
