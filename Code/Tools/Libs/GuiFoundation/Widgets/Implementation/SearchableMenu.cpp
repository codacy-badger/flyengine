#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Models/TreeSearchFilterModel.moc.h>
#include <GuiFoundation/Widgets/SearchWidget.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <QBoxLayout>
#include <QMouseEvent>
#include <QStandardItemModel>
#include <QTreeWidget>

namespace
{
  enum CustomRoles
  {
    InternalPathRole = Qt::UserRole + 1,
    VariantRole = Qt::UserRole + 2
  };
}


class QNullWidget : public QWidget
{
public:
  QNullWidget()
    : QWidget(nullptr)
  {
  }

protected:
  virtual void mousePressEvent(QMouseEvent* e) override { e->accept(); }
  virtual void mouseReleaseEvent(QMouseEvent* e) override { e->accept(); }
  virtual void mouseDoubleClickEvent(QMouseEvent* e) override { e->accept(); }
};


plQtSearchableMenu::plQtSearchableMenu(QObject* pParent)
  : QWidgetAction(pParent)
{
  m_pGroup = new QNullWidget();
  m_pGroup->setLayout(new QVBoxLayout(m_pGroup));
  m_pGroup->setContentsMargins(0, 0, 0, 0);

  m_pSearch = new plQtSearchWidget(m_pGroup);
  connect(m_pSearch, &plQtSearchWidget::enterPressed, this, &plQtSearchableMenu::OnEnterPressed);
  connect(m_pSearch, &plQtSearchWidget::specialKeyPressed, this, &plQtSearchableMenu::OnSpecialKeyPressed);
  connect(m_pSearch, &plQtSearchWidget::textChanged, this, &plQtSearchableMenu::OnSearchChanged);
  connect(m_pSearch, &plQtSearchWidget::visibleEvent, this, &plQtSearchableMenu::OnShow);

  m_pGroup->layout()->addWidget(m_pSearch);
  m_pGroup->layout()->setContentsMargins(1, 1, 1, 1);

  m_pItemModel = new QStandardItemModel(m_pGroup);

  m_pFilterModel = new plQtTreeSearchFilterModel(m_pGroup);
  m_pFilterModel->SetIncludeChildren(true);
  m_pFilterModel->setSourceModel(m_pItemModel);

  m_pTreeView = new QTreeView(m_pGroup);
  m_pTreeView->installEventFilter(this);
  m_pTreeView->setModel(m_pFilterModel);
  m_pTreeView->setHeaderHidden(true);
  m_pTreeView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
  m_pTreeView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
  connect(m_pTreeView, &QTreeView::activated, this, &plQtSearchableMenu::OnItemActivated);


  m_pGroup->layout()->addWidget(m_pTreeView);

  setDefaultWidget(m_pGroup);
}

QStandardItem* plQtSearchableMenu::CreateCategoryMenu(plStringView sCategory)
{
  if (sCategory.IsEmpty())
    return m_pItemModel->invisibleRootItem();

  auto it = m_Hierarchy.Find(sCategory);
  if (it.IsValid())
    return it.Value();

  plStringBuilder sPath = sCategory;
  sPath.PathParentDirectory();
  sPath.Trim("/");

  QStandardItem* pParentMenu = m_pItemModel->invisibleRootItem();

  if (!sPath.IsEmpty())
  {
    pParentMenu = CreateCategoryMenu(sPath);
  }

  sPath = sCategory;
  sPath = sPath.GetFileName();

  QStandardItem* pThisItem = new QStandardItem(sPath.GetData());
  pThisItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);

  pParentMenu->appendRow(pThisItem);

  m_Hierarchy[sCategory] = pThisItem;

  return pThisItem;
}

bool plQtSearchableMenu::SelectFirstLeaf(QModelIndex parent)
{
  const int iRows = m_pFilterModel->rowCount(parent);

  for (int i = 0; i < iRows; ++i)
  {
    QModelIndex child = m_pFilterModel->index(i, 0, parent);

    if (!m_pFilterModel->hasChildren(child))
    {
      if (m_pFilterModel->data(child, VariantRole).isValid())
      {
        // set this one item as the new selection
        m_pTreeView->selectionModel()->setCurrentIndex(
          child, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::SelectionFlag::SelectCurrent);
        m_pTreeView->scrollTo(child, QAbstractItemView::EnsureVisible);
        return true;
      }
    }
    else
    {
      if (SelectFirstLeaf(child))
        return true;
    }
  }

  return false;
}

bool plQtSearchableMenu::eventFilter(QObject* pObject, QEvent* event)
{
  if (pObject == m_pTreeView)
  {
    if (event->type() == QEvent::KeyPress)
    {
      QKeyEvent* pKeyEvent = (QKeyEvent*)(event);

      if (pKeyEvent->key() == Qt::Key_Tab || pKeyEvent->key() == Qt::Key_Backtab)
      {
        m_pSearch->setFocus();
        return true;
      }
    }
  }

  return false;
}

void plQtSearchableMenu::AddItem(plStringView sDisplayName, const char* szInternalPath, const QVariant& variant, QIcon icon)
{
  QStandardItem* pParent = m_pItemModel->invisibleRootItem();

  const char* szLastCat = plStringUtils::FindLastSubString(szInternalPath, "/");
  if (szLastCat != nullptr)
  {
    plStringView sCategory(szInternalPath, szLastCat);

    pParent = CreateCategoryMenu(sCategory);
  }

  QStandardItem* pThisItem = new QStandardItem(plMakeQString(sDisplayName));
  pThisItem->setFlags(Qt::ItemFlag::ItemIsEnabled | Qt::ItemFlag::ItemIsSelectable);
  pThisItem->setData(szInternalPath, InternalPathRole);
  pThisItem->setData(variant, VariantRole);
  pThisItem->setIcon(icon);

  pParent->appendRow(pThisItem);
}

QString plQtSearchableMenu::GetSearchText() const
{
  return m_pSearch->text();
}

void plQtSearchableMenu::Finalize(const QString& sSearchText)
{
  m_pItemModel->sort(0);

  m_pSearch->setText(sSearchText);
  m_pSearch->setFocus();
  m_pSearch->selectAll();
}

void plQtSearchableMenu::OnItemActivated(const QModelIndex& index)
{
  if (!index.isValid())
    return;

  QModelIndex realIndex = m_pFilterModel->mapToSource(index);

  QString sName = m_pItemModel->data(realIndex, Qt::DisplayRole).toString();
  QVariant variant = m_pItemModel->data(realIndex, VariantRole);

  // potentially only a folder item
  if (!variant.isValid())
    return;

  Q_EMIT MenuItemTriggered(sName, variant);
}

void plQtSearchableMenu::OnEnterPressed()
{
  auto selection = m_pTreeView->selectionModel()->selection();

  if (selection.size() != 1)
    return;

  OnItemActivated(selection.indexes()[0]);
}

void plQtSearchableMenu::OnSpecialKeyPressed(Qt::Key key)
{
  if (key == Qt::Key_Down || key == Qt::Key_Up || key == Qt::Key_Tab || key == Qt::Key_Backtab)
  {
    m_pTreeView->setFocus();
  }
}

void plQtSearchableMenu::OnSearchChanged(const QString& text)
{
  m_pFilterModel->SetFilterText(text);

  if (!text.isEmpty())
  {
    SelectFirstLeaf(QModelIndex());
    m_pTreeView->expandAll();
  }

  Q_EMIT SearchTextChanged(text);
}

void plQtSearchableMenu::OnShow()
{
  m_pSearch->setFocus();
  m_pSearch->selectAll();
}
