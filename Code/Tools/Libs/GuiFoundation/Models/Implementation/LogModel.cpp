#include <GuiFoundation/GuiFoundationPCH.h>

#include <GuiFoundation/Models/LogModel.moc.h>
#include <QColor>
#include <QThread>


plQtLogModel::plQtLogModel(QObject* pParent)
  : QAbstractItemModel(pParent)
{
  m_bIsValid = true;
  m_LogLevel = plLogMsgType::InfoMsg;
}

void plQtLogModel::Invalidate()
{
  if (!m_bIsValid)
    return;

  beginResetModel();
  m_bIsValid = false;
  endResetModel();
}

void plQtLogModel::Clear()
{
  if (m_AllMessages.IsEmpty())
    return;

  {
    PL_LOCK(m_NewMessagesMutex);

    m_uiNumErrors = 0;
    m_uiNumSeriousWarnings = 0;
    m_uiNumWarnings = 0;

    m_AllMessages.Clear();
    m_VisibleMessages.Clear();
    m_BlockQueue.Clear();
    Invalidate();
    m_bIsValid = true;
  }

  Q_EMIT NewErrorsOrWarnings(nullptr, false);
}

void plQtLogModel::SetLogLevel(plLogMsgType::Enum logLevel)
{
  if (m_LogLevel == logLevel)
    return;

  m_LogLevel = logLevel;
  Invalidate();
}

void plQtLogModel::SetSearchText(const char* szText)
{
  if (m_sSearchText == szText)
    return;

  m_sSearchText = szText;
  Invalidate();
}

void plQtLogModel::AddLogMsg(const plLogEntry& msg)
{
  {
    PL_LOCK(m_NewMessagesMutex);
    m_NewMessages.PushBack(msg);
  }

  // always queue the message processing, otherwise it can happen that an error during this
  // triggers recursive logging, which is forbidden
  QMetaObject::invokeMethod(this, "ProcessNewMessages", Qt::ConnectionType::QueuedConnection);

  return;
}

bool plQtLogModel::IsFiltered(const plLogEntry& lm) const
{
  if (lm.m_Type < plLogMsgType::None)
    return false;

  if (lm.m_Type > m_LogLevel)
    return true;

  if (m_sSearchText.IsEmpty())
    return false;

  if (lm.m_sMsg.FindSubString_NoCase(m_sSearchText.GetData()))
    return false;

  return true;
}

////////////////////////////////////////////////////////////////////////
// plQtLogModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

QVariant plQtLogModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  UpdateVisibleEntries();

  const plInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (plInt32)m_VisibleMessages.GetCount())
    return QVariant();

  const plLogEntry& msg = *m_VisibleMessages[iRow];

  switch (iRole)
  {
    case Qt::DisplayRole:
    {
      if (msg.m_sMsg.FindSubString("\n") != nullptr)
      {
        plStringBuilder sTemp = msg.m_sMsg;
        sTemp.ReplaceAll("\n", " ");
        return plMakeQString(sTemp);
      }
      return plMakeQString(msg.m_sMsg);
    }
    case Qt::ToolTipRole:
    {
      return plMakeQString(msg.m_sMsg);
    }
    case Qt::ForegroundRole:
    {
      switch (msg.m_Type)
      {
        case plLogMsgType::BeginGroup:
          return plToQtColor(plColorScheme::LightUI(plColorScheme::Gray));
        case plLogMsgType::EndGroup:
          return plToQtColor(plColorScheme::DarkUI(plColorScheme::Gray));
        case plLogMsgType::ErrorMsg:
          return plToQtColor(plColorScheme::LightUI(plColorScheme::Red));
        case plLogMsgType::SeriousWarningMsg:
          return plToQtColor(plColorScheme::LightUI(plColorScheme::Orange));
        case plLogMsgType::WarningMsg:
          return plToQtColor(plColorScheme::LightUI(plColorScheme::Yellow));
        case plLogMsgType::SuccessMsg:
          return plToQtColor(plColorScheme::LightUI(plColorScheme::Green));
        case plLogMsgType::DevMsg:
          return plToQtColor(plColorScheme::LightUI(plColorScheme::Blue));
        case plLogMsgType::DebugMsg:
          return plToQtColor(plColorScheme::LightUI(plColorScheme::Cyan));
        default:
          return QVariant();
      }
    }

    default:
      return QVariant();
  }
}

Qt::ItemFlags plQtLogModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant plQtLogModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
  return QVariant();
}

QModelIndex plQtLogModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  if (parent.isValid() || iColumn != 0)
    return QModelIndex();

  return createIndex(iRow, iColumn, iRow);
}

QModelIndex plQtLogModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int plQtLogModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;

  UpdateVisibleEntries();

  return (int)m_VisibleMessages.GetCount();
}

int plQtLogModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}


void plQtLogModel::ProcessNewMessages()
{
  bool bNewErrors = false;
  plStringBuilder sLatestWarning;
  plStringBuilder sLatestError;

  {
    PL_LOCK(m_NewMessagesMutex);
    plStringBuilder s;
    for (const auto& msg : m_NewMessages)
    {
      m_AllMessages.PushBack(msg);

      if (msg.m_Type == plLogMsgType::BeginGroup || msg.m_Type == plLogMsgType::EndGroup)
      {
        s.SetPrintf("%*s<<< %s", msg.m_uiIndentation, "", msg.m_sMsg.GetData());

        if (msg.m_Type == plLogMsgType::EndGroup)
        {
          s.AppendFormat(" ({0} sec) >>>", plArgF(msg.m_fSeconds, 3));
        }
        else if (!msg.m_sTag.IsEmpty())
        {
          s.Append(" (", msg.m_sTag, ") >>>");
        }
        else
        {
          s.Append(" >>>");
        }

        m_AllMessages.PeekBack().m_sMsg = s;
      }
      else
      {
        s.SetPrintf("%*s%s", 4 * msg.m_uiIndentation, "", msg.m_sMsg.GetData());
        m_AllMessages.PeekBack().m_sMsg = s;

        if (msg.m_Type == plLogMsgType::ErrorMsg)
        {
          sLatestError = msg.m_sMsg;
          bNewErrors = true;
          ++m_uiNumErrors;
        }
        else if (msg.m_Type == plLogMsgType::SeriousWarningMsg)
        {
          sLatestWarning = msg.m_sMsg;
          bNewErrors = true;
          ++m_uiNumSeriousWarnings;
        }
        else if (msg.m_Type == plLogMsgType::WarningMsg)
        {
          sLatestWarning = msg.m_sMsg;
          bNewErrors = true;
          ++m_uiNumWarnings;
        }
      }


      // if the message would not be shown anyway, don't trigger an update
      if (IsFiltered(msg))
        continue;

      if (msg.m_Type == plLogMsgType::BeginGroup)
      {
        m_BlockQueue.PushBack(&m_AllMessages.PeekBack());
        continue;
      }
      else if (msg.m_Type == plLogMsgType::EndGroup)
      {
        if (!m_BlockQueue.IsEmpty())
        {
          m_BlockQueue.PopBack();
          continue;
        }
      }

      for (auto pMsg : m_BlockQueue)
      {
        beginInsertRows(QModelIndex(), m_VisibleMessages.GetCount(), m_VisibleMessages.GetCount());
        m_VisibleMessages.PushBack(pMsg);
        endInsertRows();
      }

      m_BlockQueue.Clear();

      beginInsertRows(QModelIndex(), m_VisibleMessages.GetCount(), m_VisibleMessages.GetCount());
      m_VisibleMessages.PushBack(&m_AllMessages.PeekBack());
      endInsertRows();
    }

    m_NewMessages.Clear();
  }

  if (bNewErrors)
  {
    if (!sLatestError.IsEmpty())
    {
      Q_EMIT NewErrorsOrWarnings(sLatestError, true);
    }
    else
    {
      Q_EMIT NewErrorsOrWarnings(sLatestWarning, false);
    }
  }
}

void plQtLogModel::UpdateVisibleEntries() const
{
  if (m_bIsValid)
    return;


  m_bIsValid = true;
  m_VisibleMessages.Clear();
  for (const auto& msg : m_AllMessages)
  {
    if (IsFiltered(msg))
      continue;

    if (msg.m_Type == plLogMsgType::EndGroup)
    {
      if (!m_VisibleMessages.IsEmpty())
      {
        if (m_VisibleMessages.PeekBack()->m_Type == plLogMsgType::BeginGroup)
          m_VisibleMessages.PopBack();
        else
          m_VisibleMessages.PushBack(&msg);
      }
    }
    else
    {
      m_VisibleMessages.PushBack(&msg);
    }
  }
}
