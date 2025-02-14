#include <EditorPluginFileserve/EditorPluginFileservePCH.h>

#include <EditorPluginFileserve/FileserveUI/AllFilesModel.moc.h>

plQtFileserveAllFilesModel::plQtFileserveAllFilesModel(QWidget* pParent)
  : QAbstractListModel(pParent)
{
}

int plQtFileserveAllFilesModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return m_IndexedFiles.GetCount() - m_uiAddedItems;
}

int plQtFileserveAllFilesModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return 2;
}

QVariant plQtFileserveAllFilesModel::data(const QModelIndex& index, int iRole /*= Qt::DisplayRole*/) const
{
  if (!index.isValid())
    return QVariant();

  if (index.column() == 0)
  {
    if (iRole == Qt::DisplayRole)
    {
      return QString::number(m_IndexedFiles[index.row()].Value());
    }
  }

  if (index.column() == 1)
  {
    if (iRole == Qt::DisplayRole)
    {
      return m_IndexedFiles[index.row()].Key().GetData();
    }
  }

  return QVariant();
}


void plQtFileserveAllFilesModel::AddAccessedFile(const char* szFile)
{
  bool bExisted = false;
  auto it = m_AllFiles.FindOrAdd(szFile, &bExisted);

  // we count the accesses, but for performance reasons the TableView is not updated when the counter changes
  it.Value()++;

  if (!bExisted)
  {
    m_IndexedFiles.PushBack(it);
    ++m_uiAddedItems;

    if (!m_bTimerRunning)
    {
      m_bTimerRunning = true;
      QTimer::singleShot(500, this, &plQtFileserveAllFilesModel::UpdateViewSlot);
    }
  }
}

void plQtFileserveAllFilesModel::UpdateView()
{
  if (m_uiAddedItems == 0)
    return;

  beginInsertRows(QModelIndex(), m_IndexedFiles.GetCount() - m_uiAddedItems, m_IndexedFiles.GetCount() - 1);
  insertRows(m_IndexedFiles.GetCount(), m_uiAddedItems, QModelIndex());
  m_uiAddedItems = 0;
  endInsertRows();
}

void plQtFileserveAllFilesModel::Clear()
{
  m_AllFiles.Clear();
  m_IndexedFiles.Clear();
  m_uiAddedItems = 0;

  beginResetModel();
  endResetModel();
}

void plQtFileserveAllFilesModel::UpdateViewSlot()
{
  m_bTimerRunning = false;

  UpdateView();
}

QVariant plQtFileserveAllFilesModel::headerData(int iSection, Qt::Orientation orientation, int iRole /*= Qt::DisplayRole*/) const
{
  if (iRole == Qt::DisplayRole)
  {
    if (iSection == 0)
    {
      return "Accesses";
    }

    if (iSection == 1)
    {
      return "File";
    }
  }

  return QVariant();
}
