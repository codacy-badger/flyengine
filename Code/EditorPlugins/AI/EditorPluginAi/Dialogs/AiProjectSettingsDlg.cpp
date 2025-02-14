#include <EditorPluginAi/EditorPluginAiPCH.h>

#include <EditorPluginAi/Dialogs/AiProjectSettingsDlg.moc.h>
#include <GuiFoundation/UIServices/DynamicEnums.h>
#include <GuiFoundation/Widgets/DoubleSpinBox.moc.h>
#include <QCheckBox>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QPushButton>

void UpdateGroundTypeDynamicEnumValues();

plQtAiProjectSettingsDlg::plQtAiProjectSettingsDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  EnsureConfigFileExists();

  plDynamicEnum& col = plDynamicEnum::GetDynamicEnum("PhysicsCollisionLayer");
  for (auto it : col.GetAllValidValues())
  {
    const int iLayer = it.Key();
    CollisionLayer->addItem(it.Value().GetData(), QVariant(iLayer));
  }

  ResetState();
}

void plQtAiProjectSettingsDlg::on_DefaultButtons_clicked(QAbstractButton* pButton)
{
  if (pButton == DefaultButtons->button(QDialogButtonBox::Reset))
  {
    ResetState();
    return;
  }

  if (pButton == DefaultButtons->button(QDialogButtonBox::Apply))
  {
    SaveState();
    return;
  }

  if (pButton == DefaultButtons->button(QDialogButtonBox::Ok))
  {
    SaveState();
    accept();
    return;
  }

  if (pButton == DefaultButtons->button(QDialogButtonBox::Cancel))
  {
    reject();
    return;
  }
}

void plQtAiProjectSettingsDlg::EnsureConfigFileExists()
{
  plAiNavigationConfig cfg;

  if (cfg.Load().Failed())
  {
    cfg.Save().IgnoreResult();
  }
}

void plQtAiProjectSettingsDlg::ResetState()
{
  m_Config.Load().IgnoreResult();

  UpdateGroundTypeTable();
  FillPathSearchTypeComboBox();
  FillNavmeshTypeComboBox();

  if (SelectedPathCfg->count() > 0)
  {
    SelectedPathCfg->setCurrentIndex(0);
  }

  if (SelectedMeshCfg->count() > 0)
  {
    SelectedMeshCfg->setCurrentIndex(0);
  }

  ApplyPathConfig(SelectedPathCfg->currentIndex());
  ApplyNavmeshConfig(SelectedMeshCfg->currentIndex());
}

void plQtAiProjectSettingsDlg::SaveState()
{
  RetrieveGroundTypeTable();
  RetrievePathConfig(SelectedPathCfg->currentIndex());
  RetrieveNavmeshConfig(SelectedMeshCfg->currentIndex());
  m_Config.Save().IgnoreResult();
  UpdateGroundTypeDynamicEnumValues();
}

void plQtAiProjectSettingsDlg::UpdateGroundTypeTable()
{
  GroundTypes->clear();

  for (plUInt32 i = 0; i < plAiNumGroundTypes; ++i)
  {
    const plAiNavigationConfig::GroundType gt = m_Config.m_GroundTypes[i];

    QListWidgetItem* pItem = new QListWidgetItem(GroundTypes);
    pItem->setText(gt.m_sName.GetData());

    if (i <= 1)
    {
      pItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
    }
    else
    {
      pItem->setCheckState(gt.m_bUsed ? Qt::CheckState::Checked : Qt::CheckState::Unchecked);
      pItem->setFlags(Qt::ItemFlag::ItemIsEditable | Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable | Qt::ItemFlag::ItemIsUserCheckable);
    }
  }
}

void plQtAiProjectSettingsDlg::RetrieveGroundTypeTable()
{
  for (plUInt32 i = 2; i < plAiNumGroundTypes; ++i)
  {
    auto& gt = m_Config.m_GroundTypes[i];

    const QListWidgetItem* pItem = GroundTypes->item(i);

    gt.m_bUsed = (pItem->checkState() == Qt::CheckState::Checked);
    gt.m_sName = pItem->text().toUtf8().data();
  }
}

void plQtAiProjectSettingsDlg::on_AddPathCfg_clicked()
{
  QString name = "NewPathSearch";

retry:

  bool ok = false;
  name = QInputDialog::getText(this, "Add Path Search Config", "Name:", QLineEdit::Normal, name, &ok);
  if (!ok)
    return;

  if (name.isEmpty())
  {
    plQtUiServices::MessageBoxInformation("The name can't be empty.");
    goto retry;
  }

  for (const auto& cfg : m_Config.m_PathSearchConfigs)
  {
    if (cfg.m_sName == name.toUtf8().data())
    {
      plQtUiServices::MessageBoxInformation("A Path Search Config with this name already exists.");
      goto retry;
    }
  }

  {
    auto& newCfg = m_Config.m_PathSearchConfigs.ExpandAndGetRef();
    newCfg.m_sName = name.toUtf8().data();
  }

  m_Config.m_PathSearchConfigs.Sort([](const auto& lhs, const auto& rhs) -> bool { return lhs.m_sName < rhs.m_sName; });

  FillPathSearchTypeComboBox();
  SelectedPathCfg->setCurrentText(name);
}

void plQtAiProjectSettingsDlg::on_RemovePathCfg_clicked()
{
  int cur = SelectedPathCfg->currentIndex();

  if (cur < 0 || cur >= m_Config.m_PathSearchConfigs.GetCount())
    return;

  if (plQtUiServices::MessageBoxQuestion("Remove the current Path Search Config?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
  {
    return;
  }

  m_iSelectedPathSearchConfig = -1;
  m_Config.m_PathSearchConfigs.RemoveAtAndCopy(cur);

  FillPathSearchTypeComboBox();
  SelectedPathCfg->setCurrentIndex(plMath::Min(cur, SelectedPathCfg->count() - 1));
}

void plQtAiProjectSettingsDlg::on_SelectedPathCfg_currentIndexChanged(int index)
{
  RetrievePathConfig(m_iSelectedPathSearchConfig);

  ApplyPathConfig(index);
}

void plQtAiProjectSettingsDlg::ApplyPathConfig(int index)
{
  m_iSelectedPathSearchConfig = index;
  PathConfig->setRowCount(0);

  if (index < 0 || index >= m_Config.m_PathSearchConfigs.GetCount())
    return;

  const auto& ps = m_Config.m_PathSearchConfigs[index];

  PathConfig->setColumnCount(3);
  PathConfig->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Stretch);
  PathConfig->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeMode::ResizeToContents);
  PathConfig->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeMode::ResizeToContents);

  for (plUInt32 i = 1; i < plAiNumGroundTypes; ++i)
  {
    const auto& gt = m_Config.m_GroundTypes[i];
    if (!gt.m_bUsed)
      continue;

    const int row = PathConfig->rowCount();
    PathConfig->setRowCount(row + 1);

    QCheckBox* pCB = new QCheckBox();
    pCB->setCheckState(ps.m_bGroundTypeAllowed[i] ? Qt::Checked : Qt::Unchecked);

    plQtDoubleSpinBox* pVal = new plQtDoubleSpinBox(PathConfig);
    pVal->setValue(ps.m_fGroundTypeCost[i]);

    PathConfig->setItem(row, 0, new QTableWidgetItem(gt.m_sName.GetData()));
    PathConfig->setCellWidget(row, 1, pCB);
    PathConfig->setCellWidget(row, 2, pVal);
  }
}
void plQtAiProjectSettingsDlg::FillPathSearchTypeComboBox()
{
  RetrievePathConfig(m_iSelectedPathSearchConfig);
  m_iSelectedPathSearchConfig = -1;

  plQtScopedBlockSignals _1(SelectedPathCfg);

  SelectedPathCfg->clear();

  for (const auto& cfg : m_Config.m_PathSearchConfigs)
  {
    SelectedPathCfg->addItem(cfg.m_sName.GetData());
  }

  RemovePathCfg->setEnabled(!m_Config.m_PathSearchConfigs.IsEmpty());
  SelectedPathCfg->setEnabled(!m_Config.m_PathSearchConfigs.IsEmpty());
  SelectedPathCfg->setCurrentIndex(-1);
}

void plQtAiProjectSettingsDlg::RetrievePathConfig(int index)
{
  if (index < 0 || index >= m_Config.m_PathSearchConfigs.GetCount())
    return;

  auto& cfg = m_Config.m_PathSearchConfigs[index];

  for (int row = 0; row < PathConfig->rowCount(); ++row)
  {
    const QTableWidgetItem* pText = PathConfig->item(row, 0);
    const QCheckBox* pCB = qobject_cast<QCheckBox*>(PathConfig->cellWidget(row, 1));
    const plQtDoubleSpinBox* pSB = qobject_cast<plQtDoubleSpinBox*>(PathConfig->cellWidget(row, 2));

    const plString sGroundType = pText->text().toUtf8().data();

    for (plUInt32 gt = 0; gt < plAiNumGroundTypes; ++gt)
    {
      if (m_Config.m_GroundTypes[gt].m_sName == sGroundType)
      {
        cfg.m_bGroundTypeAllowed[gt] = pCB->isChecked();
        cfg.m_fGroundTypeCost[gt] = (float)pSB->value();
        break;
      }
    }
  }
}

void plQtAiProjectSettingsDlg::on_AddMeshCfg_clicked()
{
  QString name = "New Navmesh";

retry:

  bool ok = false;
  name = QInputDialog::getText(this, "Add Navmesh Config", "Name:", QLineEdit::Normal, name, &ok);
  if (!ok)
    return;

  if (name.isEmpty())
  {
    plQtUiServices::MessageBoxInformation("The name can't be empty.");
    goto retry;
  }

  for (const auto& cfg : m_Config.m_NavmeshConfigs)
  {
    if (cfg.m_sName == name.toUtf8().data())
    {
      plQtUiServices::MessageBoxInformation("A Navmesh Config with this name already exists.");
      goto retry;
    }
  }

  {
    auto& newCfg = m_Config.m_NavmeshConfigs.ExpandAndGetRef();
    newCfg.m_sName = name.toUtf8().data();
  }

  m_Config.m_NavmeshConfigs.Sort([](const auto& lhs, const auto& rhs) -> bool { return lhs.m_sName < rhs.m_sName; });

  FillNavmeshTypeComboBox();
  SelectedMeshCfg->setCurrentText(name);
}

void plQtAiProjectSettingsDlg::on_RemoveMeshCfg_clicked()
{
  int cur = SelectedMeshCfg->currentIndex();

  if (cur < 0 || cur >= m_Config.m_NavmeshConfigs.GetCount())
    return;

  if (plQtUiServices::MessageBoxQuestion("Remove the current Navmesh Config?", QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::No)
  {
    return;
  }

  m_iSelectedNavmeshConfig = -1;
  m_Config.m_NavmeshConfigs.RemoveAtAndCopy(cur);

  FillNavmeshTypeComboBox();
  SelectedMeshCfg->setCurrentIndex(plMath::Min(cur, SelectedMeshCfg->count() - 1));
}

void plQtAiProjectSettingsDlg::on_SelectedMeshCfg_currentIndexChanged(int index)
{
  RetrieveNavmeshConfig(m_iSelectedNavmeshConfig);

  ApplyNavmeshConfig(index);
}

void plQtAiProjectSettingsDlg::ApplyNavmeshConfig(int index)
{
  m_iSelectedNavmeshConfig = index;

  if (index < 0 || index >= m_Config.m_NavmeshConfigs.GetCount())
  {
    NavmeshCfg->setEnabled(false);
    return;
  }

  NavmeshCfg->setEnabled(true);

  const auto& ps = m_Config.m_NavmeshConfigs[index];

  NavCellSize->setValue(ps.m_fCellSize);
  NavCellHeight->setValue(ps.m_fCellHeight);
  NavAgentRadius->setValue(ps.m_fAgentRadius);
  NavAgentHeight->setValue(ps.m_fAgentHeight);
  NavAgentStep->setValue(ps.m_fAgentStepHeight);
  NavAgentSlope->setValue(ps.m_WalkableSlope.GetDegree());

  NavMaxEdgeLength->setValue(ps.m_fMaxEdgeLength);
  NavSimplificationError->setValue(ps.m_fMaxSimplificationError);
  NavMinRegionSize->setValue(ps.m_fMinRegionSize);
  NavRegionMergeSize->setValue(ps.m_fRegionMergeSize);
  NavDetailSampleDistance->setValue(ps.m_fDetailMeshSampleDistanceFactor);
  NavDetailSampleError->setValue(ps.m_fDetailMeshSampleErrorFactor);

  for (plInt32 i = 0; i < CollisionLayer->count(); ++i)
  {
    if (CollisionLayer->itemData(i, Qt::UserRole).toInt() == (int)ps.m_uiCollisionLayer)
    {
      CollisionLayer->setCurrentIndex(i);
      break;
    }
  }
}
void plQtAiProjectSettingsDlg::FillNavmeshTypeComboBox()
{
  RetrieveNavmeshConfig(m_iSelectedNavmeshConfig);
  m_iSelectedNavmeshConfig = -1;

  plQtScopedBlockSignals _1(SelectedMeshCfg);

  SelectedMeshCfg->clear();

  for (const auto& cfg : m_Config.m_NavmeshConfigs)
  {
    SelectedMeshCfg->addItem(cfg.m_sName.GetData());
  }

  RemoveMeshCfg->setEnabled(!m_Config.m_NavmeshConfigs.IsEmpty());
  SelectedMeshCfg->setEnabled(!m_Config.m_NavmeshConfigs.IsEmpty());
  SelectedMeshCfg->setCurrentIndex(-1);
}

void plQtAiProjectSettingsDlg::RetrieveNavmeshConfig(int index)
{
  if (index < 0 || index >= m_Config.m_NavmeshConfigs.GetCount())
    return;

  auto& cfg = m_Config.m_NavmeshConfigs[index];

  cfg.m_uiCollisionLayer = (plUInt32)CollisionLayer->currentData(Qt::UserRole).toInt();

  cfg.m_fCellSize = NavCellSize->value();
  cfg.m_fCellHeight = NavCellHeight->value();
  cfg.m_fAgentRadius = NavAgentRadius->value();
  cfg.m_fAgentHeight = NavAgentHeight->value();
  cfg.m_fAgentStepHeight = NavAgentStep->value();
  cfg.m_WalkableSlope = plAngle::MakeFromDegree(NavAgentSlope->value());

  cfg.m_fMaxEdgeLength = NavMaxEdgeLength->value();
  cfg.m_fMaxSimplificationError = NavSimplificationError->value();
  cfg.m_fMinRegionSize = NavMinRegionSize->value();
  cfg.m_fRegionMergeSize = NavRegionMergeSize->value();
  cfg.m_fDetailMeshSampleDistanceFactor = NavDetailSampleDistance->value();
  cfg.m_fDetailMeshSampleErrorFactor = NavDetailSampleError->value();
}
