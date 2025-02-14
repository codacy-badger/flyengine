#include <Foundation/FoundationPCH.h>

#include <Foundation/Communication/Implementation/IpcChannelEnet.h>
#include <Foundation/Communication/Implementation/MessageLoop.h>
#include <Foundation/Communication/IpcChannel.h>
#include <Foundation/Communication/RemoteMessage.h>
#include <Foundation/Logging/Log.h>

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
#  include <Foundation/Platform/Win/PipeChannel_Win.h>
#elif PL_ENABLED(PL_PLATFORM_LINUX)
#  include <Foundation/Platform/Linux/PipeChannel_Linux.h>
#endif

PL_CHECK_AT_COMPILETIME((plInt32)plIpcChannel::ConnectionState::Disconnected == (plInt32)plIpcChannelEvent::Disconnected);
PL_CHECK_AT_COMPILETIME((plInt32)plIpcChannel::ConnectionState::Connecting == (plInt32)plIpcChannelEvent::Connecting);
PL_CHECK_AT_COMPILETIME((plInt32)plIpcChannel::ConnectionState::Connected == (plInt32)plIpcChannelEvent::Connected);

plIpcChannel::plIpcChannel(plStringView sAddress, Mode::Enum mode)
  : m_sAddress(sAddress)
  , m_Mode(mode)
  , m_pOwner(plMessageLoop::GetSingleton())
{
}

plIpcChannel::~plIpcChannel()
{


  m_pOwner->RemoveChannel(this);
}

plInternal::NewInstance<plIpcChannel> plIpcChannel::CreatePipeChannel(plStringView sAddress, Mode::Enum mode)
{
  if (sAddress.IsEmpty() || sAddress.GetElementCount() > 200)
  {
    plLog::Error("Failed co create pipe '{0}', name is not valid", sAddress);
    return nullptr;
  }

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
  return PL_DEFAULT_NEW(plPipeChannel_win, sAddress, mode);
#elif PL_ENABLED(PL_PLATFORM_LINUX)
  return PL_DEFAULT_NEW(plPipeChannel_linux, sAddress, mode);
#else
  PL_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
#endif
}


plInternal::NewInstance<plIpcChannel> plIpcChannel::CreateNetworkChannel(plStringView sAddress, Mode::Enum mode)
{
#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT
  return PL_DEFAULT_NEW(plIpcChannelEnet, sAddress, mode);
#else
  PL_ASSERT_NOT_IMPLEMENTED;
  return nullptr;
#endif
}

void plIpcChannel::Connect()
{
  PL_LOCK(m_pOwner->m_TasksMutex);
  m_pOwner->m_ConnectQueue.PushBack(this);
  m_pOwner->WakeUp();
}


void plIpcChannel::Disconnect()
{
  PL_LOCK(m_pOwner->m_TasksMutex);
  m_pOwner->m_DisconnectQueue.PushBack(this);
  m_pOwner->WakeUp();
}


bool plIpcChannel::Send(plArrayPtr<const plUInt8> data)
{
  {
    PL_LOCK(m_OutputQueueMutex);
    plMemoryStreamStorageInterface& storage = m_OutputQueue.ExpandAndGetRef();
    plMemoryStreamWriter writer(&storage);
    plUInt32 uiSize = data.GetCount() + HEADER_SIZE;
    plUInt32 uiMagic = MAGIC_VALUE;
    writer << uiMagic;
    writer << uiSize;
    PL_ASSERT_DEBUG(storage.GetStorageSize32() == HEADER_SIZE, "Magic value and size should have written HEADER_SIZE bytes.");
    writer.WriteBytes(data.GetPtr(), data.GetCount()).AssertSuccess("Failed to write to in-memory buffer, out of memory?");
  }
  if (IsConnected())
  {
    PL_LOCK(m_pOwner->m_TasksMutex);
    if (!m_pOwner->m_SendQueue.Contains(this))
      m_pOwner->m_SendQueue.PushBack(this);
    if (NeedWakeup())
    {
      m_pOwner->WakeUp();
    }
    return true;
  }
  return false;
}

void plIpcChannel::SetReceiveCallback(ReceiveCallback callback)
{
  PL_LOCK(m_ReceiveCallbackMutex);
  m_ReceiveCallback = callback;
}

plResult plIpcChannel::WaitForMessages(plTime timeout)
{
  if (IsConnected())
  {
    if (timeout == plTime::MakeZero())
    {
      m_IncomingMessages.WaitForSignal();
    }
    else if (m_IncomingMessages.WaitForSignal(timeout) == plThreadSignal::WaitResult::Timeout)
    {
      return PL_FAILURE;
    }
  }
  return PL_SUCCESS;
}

void plIpcChannel::SetConnectionState(plEnum<plIpcChannel::ConnectionState> state)
{
  const plEnum<plIpcChannel::ConnectionState> oldValue = m_iConnectionState.Set(state);

  if (state != oldValue)
  {
    m_Events.Broadcast(plIpcChannelEvent((plIpcChannelEvent::Type)state.GetValue(), this));
  }
}

void plIpcChannel::ReceiveData(plArrayPtr<const plUInt8> data)
{
  PL_LOCK(m_ReceiveCallbackMutex);

  if (!m_ReceiveCallback.IsValid())
  {
    m_MessageAccumulator.PushBackRange(data);
    return;
  }

  plArrayPtr<const plUInt8> remainingData = data;
  while (true)
  {
    if (m_MessageAccumulator.GetCount() < HEADER_SIZE)
    {
      if (remainingData.GetCount() + m_MessageAccumulator.GetCount() < HEADER_SIZE)
      {
        m_MessageAccumulator.PushBackRange(remainingData);
        return;
      }
      else
      {
        plUInt32 uiRemainingHeaderData = HEADER_SIZE - m_MessageAccumulator.GetCount();
        plArrayPtr<const plUInt8> headerData = remainingData.GetSubArray(0, uiRemainingHeaderData);
        m_MessageAccumulator.PushBackRange(headerData);
        PL_ASSERT_DEBUG(m_MessageAccumulator.GetCount() == HEADER_SIZE, "We should have a full header now.");
        remainingData = remainingData.GetSubArray(uiRemainingHeaderData);
      }
    }

    PL_ASSERT_DEBUG(m_MessageAccumulator.GetCount() >= HEADER_SIZE, "Header must be complete at this point.");
    if (remainingData.IsEmpty())
      return;

    // Read and verify header
    plUInt32 uiMagic = *reinterpret_cast<const plUInt32*>(m_MessageAccumulator.GetData());
    PL_IGNORE_UNUSED(uiMagic);
    PL_ASSERT_DEBUG(uiMagic == MAGIC_VALUE, "Message received with wrong magic value.");
    plUInt32 uiMessageSize = *reinterpret_cast<const plUInt32*>(m_MessageAccumulator.GetData() + 4);
    PL_ASSERT_DEBUG(uiMessageSize < MAX_MESSAGE_SIZE, "Message too big: {0}! Either the stream got corrupted or you need to increase MAX_MESSAGE_SIZE.", uiMessageSize);
    if (uiMessageSize > remainingData.GetCount() + m_MessageAccumulator.GetCount())
    {
      m_MessageAccumulator.PushBackRange(remainingData);
      return;
    }

    // Write missing data into message accumulator
    plUInt32 remainingMessageData = uiMessageSize - m_MessageAccumulator.GetCount();
    plArrayPtr<const plUInt8> messageData = remainingData.GetSubArray(0, remainingMessageData);
    m_MessageAccumulator.PushBackRange(messageData);
    PL_ASSERT_DEBUG(m_MessageAccumulator.GetCount() == uiMessageSize, "");
    remainingData = remainingData.GetSubArray(remainingMessageData);

    {
      m_ReceiveCallback(plArrayPtr<const plUInt8>(m_MessageAccumulator.GetData() + HEADER_SIZE, uiMessageSize - HEADER_SIZE));
      m_IncomingMessages.RaiseSignal();
      m_Events.Broadcast(plIpcChannelEvent(plIpcChannelEvent::NewMessages, this));
      m_MessageAccumulator.Clear();
    }
  }
}

void plIpcChannel::FlushPendingOperations()
{
  m_pOwner->WaitForMessages(-1, this);
}


