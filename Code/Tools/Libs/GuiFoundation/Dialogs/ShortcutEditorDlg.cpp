#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/Action/ActionManager.h>
#include <GuiFoundation/Dialogs/ShortcutEditorDlg.moc.h>
#include <QKeySequenceEdit>
#include <QTableWidget>
#include <QTreeWidget>

plQtShortcutEditorDlg::plQtShortcutEditorDlg(QWidget* pParent)
  : QDialog(pParent)
{
  setupUi(this);

  PL_VERIFY(connect(Shortcuts, SIGNAL(itemSelectionChanged()), this, SLOT(SlotSelectionChanged())) != nullptr, "signal/slot connection failed");

  m_iSelectedAction = -1;
  KeyEditor->setEnabled(false);

  plMap<plString, plMap<plString, plInt32>> SortedItems;

  {
    auto itActions = plActionManager::GetActionIterator();

    while (itActions.IsValid())
    {
      if (itActions.Value()->m_Type == plActionType::Action)
      {
        SortedItems[itActions.Value()->m_sCategoryPath][itActions.Value()->m_sActionName] = m_ActionDescs.GetCount();
        m_ActionDescs.PushBack(itActions.Value());
      }

      itActions.Next();
    }
  }

  {
    plQtScopedBlockSignals bs(Shortcuts);
    plQtScopedUpdatesDisabled ud(Shortcuts);

    Shortcuts->setAlternatingRowColors(true);
    Shortcuts->setEditTriggers(QAbstractItemView::EditTrigger::NoEditTriggers);
    Shortcuts->setExpandsOnDoubleClick(true);

    plStringBuilder sTemp;

    for (auto it = SortedItems.GetIterator(); it.IsValid(); ++it)
    {
      auto pParent = new QTreeWidgetItem();
      pParent->setData(0, Qt::DisplayRole, it.Key().GetData());
      Shortcuts->addTopLevelItem(pParent);

      pParent->setExpanded(true);
      pParent->setFirstColumnSpanned(true);
      pParent->setFlags(Qt::ItemFlag::ItemIsEnabled);

      QFont font = pParent->font(0);
      font.setBold(true);

      pParent->setFont(0, font);

      for (auto it2 : it.Value())
      {
        const auto& item = m_ActionDescs[it2.Value()];
        auto pItem = new QTreeWidgetItem(pParent);

        /// \todo Instead of removing &, replace it by underlined text (requires formatted text output)
        sTemp = plTranslate(item->m_sActionName);
        sTemp.ReplaceAll("&", "");

        pItem->setData(0, Qt::UserRole, it2.Value());
        pItem->setData(0, Qt::DisplayRole, item->m_sActionName.GetData());
        pItem->setData(1, Qt::DisplayRole, sTemp.GetData());
        pItem->setData(2, Qt::DisplayRole, item->m_sShortcut.GetData());
        pItem->setData(3, Qt::DisplayRole, plMakeQString(plTranslateTooltip(item->m_sActionName)));

        if (item->m_sShortcut == item->m_sDefaultShortcut)
          pItem->setBackground(2, QBrush());
        else
          pItem->setBackground(2, Qt::darkYellow);

        sTemp.Set("Default: ", item->m_sDefaultShortcut.IsEmpty() ? "<none>" : item->m_sDefaultShortcut.GetData());

        pItem->setToolTip(2, QString::fromUtf8(sTemp.GetData()));
      }
    }

    Shortcuts->resizeColumnToContents(0);
    Shortcuts->resizeColumnToContents(2);
  }

  ButtonAssign->setEnabled(false);
  ButtonRemove->setEnabled(false);
  ButtonReset->setEnabled(false);
}

plQtShortcutEditorDlg::~plQtShortcutEditorDlg()
{
  plActionManager::SaveShortcutAssignment();
}

void plQtShortcutEditorDlg::UpdateTable()
{
  for (plInt32 iTop = 0; iTop < Shortcuts->topLevelItemCount(); ++iTop)
  {
    auto pTopItem = Shortcuts->topLevelItem(iTop);

    for (plInt32 iChild = 0; iChild < pTopItem->childCount(); ++iChild)
    {
      auto pChild = pTopItem->child(iChild);

      plInt32 idx = pChild->data(0, Qt::UserRole).toInt();

      const auto& item = m_ActionDescs[idx];

      pChild->setData(2, Qt::DisplayRole, QVariant(item->m_sShortcut.GetData()));

      if (item->m_sShortcut == item->m_sDefaultShortcut)
        pChild->setBackground(2, QBrush());
      else
        pChild->setBackground(2, Qt::darkYellow);
    }
  }

  if (m_iSelectedAction >= 0)
  {
    ButtonRemove->setEnabled(!m_ActionDescs[m_iSelectedAction]->m_sShortcut.IsEmpty());
    ButtonReset->setEnabled(m_ActionDescs[m_iSelectedAction]->m_sShortcut != m_ActionDescs[m_iSelectedAction]->m_sDefaultShortcut);
  }

  QString sText = KeyEditor->keySequence().toString(QKeySequence::SequenceFormat::NativeText);
  ButtonAssign->setEnabled(!sText.isEmpty());
}

void plQtShortcutEditorDlg::SlotSelectionChanged()
{
  auto selection = Shortcuts->selectedItems();
  if (selection.size() == 1)
  {
    m_iSelectedAction = selection[0]->data(0, Qt::UserRole).toInt();
    KeyEditor->clear();
    KeyEditor->setEnabled(true);
    ButtonAssign->setEnabled(false);
    ButtonRemove->setEnabled(!m_ActionDescs[m_iSelectedAction]->m_sShortcut.IsEmpty());
    ButtonReset->setEnabled(m_ActionDescs[m_iSelectedAction]->m_sShortcut != m_ActionDescs[m_iSelectedAction]->m_sDefaultShortcut);
  }
  else
  {
    m_iSelectedAction = -1;
    KeyEditor->clear();
    KeyEditor->setEnabled(false);
    ButtonAssign->setEnabled(false);
    ButtonRemove->setEnabled(false);
    ButtonReset->setEnabled(false);
  }
}

void plQtShortcutEditorDlg::on_KeyEditor_editingFinished()
{
  UpdateKeyEdit();
}

void plQtShortcutEditorDlg::UpdateKeyEdit()
{
  if (m_iSelectedAction < 0)
    return;

  QString sText = KeyEditor->keySequence().toString(QKeySequence::SequenceFormat::NativeText);
  ButtonAssign->setEnabled(!sText.isEmpty());
}

void plQtShortcutEditorDlg::on_KeyEditor_keySequenceChanged(const QKeySequence& keySequence)
{
  UpdateKeyEdit();
}

void plQtShortcutEditorDlg::on_ButtonAssign_clicked()
{
  QString sText = KeyEditor->keySequence().toString(QKeySequence::SequenceFormat::NativeText);
  KeyEditor->clear();

  m_ActionDescs[m_iSelectedAction]->m_sShortcut = sText.toUtf8().data();
  m_ActionDescs[m_iSelectedAction]->UpdateExistingActions();

  UpdateTable();
}

void plQtShortcutEditorDlg::on_ButtonRemove_clicked()
{
  KeyEditor->clear();

  m_ActionDescs[m_iSelectedAction]->m_sShortcut.Clear();
  m_ActionDescs[m_iSelectedAction]->UpdateExistingActions();

  UpdateTable();
}

void plQtShortcutEditorDlg::on_ButtonReset_clicked()
{
  KeyEditor->clear();

  m_ActionDescs[m_iSelectedAction]->m_sShortcut = m_ActionDescs[m_iSelectedAction]->m_sDefaultShortcut;
  m_ActionDescs[m_iSelectedAction]->UpdateExistingActions();

  UpdateTable();
}
