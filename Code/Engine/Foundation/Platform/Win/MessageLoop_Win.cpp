#include <Foundation/FoundationPCH.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)

#  include <Foundation/Communication/IpcChannel.h>
#  include <Foundation/Platform/Win/MessageLoop_Win.h>
#  include <Foundation/Platform/Win/PipeChannel_Win.h>

plMessageLoop_win::plMessageLoop_win()
{
  m_hPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
  PL_ASSERT_DEBUG(m_hPort != INVALID_HANDLE_VALUE, "Failed to create IO completion port!");
}

plMessageLoop_win::~plMessageLoop_win()
{
  StopUpdateThread();
  CloseHandle(m_hPort);
}


bool plMessageLoop_win::WaitForMessages(plInt32 iTimeout, plIpcChannel* pFilter)
{
  if (iTimeout < 0)
    iTimeout = INFINITE;

  IOItem item;
  if (!MatchCompletedIOItem(pFilter, &item))
  {
    if (!GetIOItem(iTimeout, &item))
      return false;

    if (ProcessInternalIOItem(item))
      return true;
  }

  if (item.pContext->pChannel != NULL)
  {
    if (pFilter != NULL && item.pChannel != pFilter)
    {
      m_CompletedIO.PushBack(item);
    }
    else
    {
      PL_ASSERT_DEBUG(item.pContext->pChannel == item.pChannel, "");
      static_cast<plPipeChannel_win*>(item.pChannel)->OnIOCompleted(item.pContext, item.uiBytesTransfered, item.uiError);
    }
  }
  return true;
}

bool plMessageLoop_win::GetIOItem(plInt32 iTimeout, IOItem* pItem)
{
  memset(pItem, 0, sizeof(*pItem));
  ULONG_PTR key = 0;
  OVERLAPPED* overlapped = NULL;
  if (!GetQueuedCompletionStatus(m_hPort, &pItem->uiBytesTransfered, &key, &overlapped, iTimeout))
  {
    // nothing queued
    if (overlapped == NULL)
      return false;

    pItem->uiError = GetLastError();
    pItem->uiBytesTransfered = 0;
  }

  pItem->pChannel = reinterpret_cast<plIpcChannel*>(key);
  pItem->pContext = reinterpret_cast<IOContext*>(overlapped);
  return true;
}

bool plMessageLoop_win::ProcessInternalIOItem(const IOItem& item)
{
  if (reinterpret_cast<plMessageLoop_win*>(item.pContext) == this && reinterpret_cast<plMessageLoop_win*>(item.pChannel) == this)
  {
    // internal notification
    PL_ASSERT_DEBUG(item.uiBytesTransfered == 0, "");
    InterlockedExchange(&m_iHaveWork, 0);
    return true;
  }
  return false;
}

bool plMessageLoop_win::MatchCompletedIOItem(plIpcChannel* pFilter, IOItem* pItem)
{
  for (plUInt32 i = 0; i < m_CompletedIO.GetCount(); i++)
  {
    if (pFilter == NULL || m_CompletedIO[i].pChannel == pFilter)
    {
      *pItem = m_CompletedIO[i];
      m_CompletedIO.RemoveAtAndCopy(i);
      return true;
    }
  }
  return false;
}

void plMessageLoop_win::WakeUp()
{
  if (InterlockedExchange(&m_iHaveWork, 1))
  {
    // already running
    return;
  }
  // wake up the loop
  BOOL res = PostQueuedCompletionStatus(m_hPort, 0, reinterpret_cast<ULONG_PTR>(this), reinterpret_cast<OVERLAPPED*>(this));
  PL_IGNORE_UNUSED(res);
  PL_ASSERT_DEBUG(res, "Could not PostQueuedCompletionStatus: {0}", plArgErrorCode(GetLastError()));
}

#endif


