#include "Player.h"
#include "Logger.h"
#include <assert.h>
#include <boost/atomic.hpp>				// boost::atomic_bool

#pragma comment(lib, "mfuuid.lib")		// MF_EVENT_TOPOLOGY_STATUS MFMediaType_Audio MFMediaType_Video
#pragma comment(lib, "strmiids.lib")	// MR_VIDEO_RENDER_SERVICE
#pragma comment(lib, "Shlwapi.lib")		// SHCreateMemStream

HINSTANCE hLibMFPlat = NULL;
_MFCreateMFByteStreamOnStream lpfMFCreateMFByteStreamOnStream = NULL;
_MFCreateSourceResolver lpfMFCreateSourceResolver = NULL;
_MFShutdown lpfMFShutdown = NULL;
_MFStartup lpfMFStartup = NULL;

HINSTANCE hLibMF = NULL;
_MFCreateAudioRendererActivate lpfMFCreateAudioRendererActivate = NULL;
_MFCreateMediaSession lpfMFCreateMediaSession = NULL;
_MFCreateTopology lpfMFCreateTopology = NULL;
_MFCreateTopologyNode lpfMFCreateTopologyNode = NULL;
_MFCreateVideoRendererActivate lpfMFCreateVideoRendererActivate = NULL;
_MFGetService lpfMFGetService = NULL;


const UINT WM_APP_PLAYER_EVENT = WM_APP + 1000;

BOOST_LOG_GLOBAL_LOGGER_CTOR_ARGS(player_logger,
								src::severity_channel_logger_mt<SeverityLevel>,
								(keywords::channel = "player"))
src::severity_channel_logger_mt<SeverityLevel>& logger = player_logger::get();

#define CHECK_HR(hr)			\
	if (FAILED(hr))				\
	{							\
		BOOST_LOG_FUNCTION();	\
		LOG_ERROR(logger) << std::showbase << std::uppercase << std::hex << hr << std::noshowbase << std::nouppercase << std::dec; \
		goto done;				\
	}

template <class Q>
HRESULT GetEventObject(IMFMediaEvent *pEvent, Q **ppObject)
{
    *ppObject = NULL;   // zero output

    PROPVARIANT var;
    HRESULT hr = pEvent->GetValue(&var);
    if (SUCCEEDED(hr))
    {
        if (var.vt == VT_UNKNOWN)
        {
            hr = var.punkVal->QueryInterface(ppObject);
        }
        else
        {
            hr = MF_E_INVALIDTYPE;
        }
        PropVariantClear(&var);
    }
    return hr;
}

HRESULT CreateMediaSource(LPCWSTR pwszFileName, IMFByteStream *pByteStream, IMFMediaSource **ppSource);

HRESULT CreateMediaSource(PCWSTR pszURL, IMFMediaSource **ppSource);

HRESULT CreatePlaybackTopology(IMFMediaSource *pSource, 
    IMFPresentationDescriptor *pPD, HWND hVideoWnd,IMFTopology **ppTopology);

//  Static class method to create the CPlayer object.

HRESULT CPlayer::CreateInstance(
    HWND hVideo,                  // Video window.
    HWND hEvent,                  // Window to receive notifications.
    CPlayer **ppPlayer)           // Receives a pointer to the CPlayer object.
{
	BOOST_LOG_FUNCTION();
	FUNC_TRACER("CPlayer::CreateInstance");
	if (ppPlayer == NULL)
    {
		LOG_ERROR(logger) << "E_POINTER";
        return E_POINTER;
    }

    CPlayer *pPlayer = new (std::nothrow) CPlayer(hVideo, hEvent);
    if (pPlayer == NULL)
    {
		LOG_ERROR(logger) << "E_OUTOFMEMORY";
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pPlayer->Initialize();
    if (SUCCEEDED(hr))
    {
        *ppPlayer = pPlayer;
    }
    else
    {
		LOG_ERROR(logger) << "Player->Initialize";
		pPlayer->Release();
		*ppPlayer = NULL;
    }
    return hr;
}

HRESULT CPlayer::Initialize()
{
	if (!GetProcAddresses(&hLibMFPlat, "mfplat.dll", 4,
		&lpfMFCreateMFByteStreamOnStream,	"MFCreateMFByteStreamOnStream",
		&lpfMFCreateSourceResolver,			"MFCreateSourceResolver",
		&lpfMFShutdown,						"MFShutdown",
		&lpfMFStartup,						"MFStartup"))
	{
		LOG_ERROR(logger) << "GetProcAddresses from mfplat failed";
		return -1;
	}

	if (!GetProcAddresses(&hLibMF, "mf.dll", 6,
		&lpfMFCreateAudioRendererActivate,	"MFCreateAudioRendererActivate",
		&lpfMFCreateMediaSession,			"MFCreateMediaSession",
		&lpfMFCreateTopology,				"MFCreateTopology",
		&lpfMFCreateTopologyNode,			"MFCreateTopologyNode",
		&lpfMFCreateVideoRendererActivate,	"MFCreateVideoRendererActivate",
		&lpfMFGetService,					"MFGetService"))
	{
		LOG_ERROR(logger) << "GetProcAddresses from mf failed";
		return -1;
	}

	return S_OK;
}

CPlayer::CPlayer(HWND hVideo, HWND hEvent) : 
    m_pSession(NULL),
    m_pSource(NULL),
    m_pVideoDisplay(NULL),
    m_hwndVideo(hVideo),
    m_hwndEvent(hEvent),
    m_state(Closed),
    m_hCloseEvent(NULL),
    m_nRefCount(1)
{
}

CPlayer::~CPlayer()
{
    assert(m_pSession == NULL);  
    // If FALSE, the app did not call Shutdown().

    // When CPlayer calls IMediaEventGenerator::BeginGetEvent on the
    // media session, it causes the media session to hold a reference 
    // count on the CPlayer. 
    
    // This creates a circular reference count between CPlayer and the 
    // media session. Calling Shutdown breaks the circular reference 
    // count.

    // If CreateInstance fails, the application will not call 
    // Shutdown. To handle that case, call Shutdown in the destructor. 

    Shutdown();
}

// IUnknown methods

HRESULT CPlayer::QueryInterface(REFIID riid, void** ppv)
{
    static const QITAB qit[] = 
    {
        QITABENT(CPlayer, IMFAsyncCallback),
        { 0 }
    };
    return QISearch(this, qit, riid, ppv);
}

ULONG CPlayer::AddRef()
{
    return InterlockedIncrement(&m_nRefCount);
}

ULONG CPlayer::Release()
{
    ULONG uCount = InterlockedDecrement(&m_nRefCount);
    if (uCount == 0)
    {
        delete this;
    }
    return uCount;
}

HRESULT CPlayer::OpenMem(std::string name, const BYTE *pBuf, const int len)
{
	FUNC_TRACER("CPlayer::OpenMem");

	IStream* pMemStream = NULL;
	IMFByteStream *pByteStream = NULL;
	IMFTopology *pTopology = NULL;
	IMFPresentationDescriptor* pSourcePD = NULL;
	IMFMediaSource *pSourceTmp = NULL;

	if (m_pSource)
		pSourceTmp = m_pSource;

	size_t size = name.length();
	wchar_t pwszFileName[2046] = { 0 };
	MultiByteToWideChar(CP_ACP, 0, name.c_str(), size, pwszFileName, size * sizeof(wchar_t));
	pwszFileName[size] = 0;

	HRESULT hr = S_OK;
	pMemStream = SHCreateMemStream(pBuf, len);
	if (!pMemStream)
		hr = -1;
	CHECK_HR(hr);

	hr = lpfMFCreateMFByteStreamOnStream(pMemStream, &pByteStream);
	CHECK_HR(hr);

	hr = CreateMediaSource(pwszFileName, pByteStream, &m_pSource);
	CHECK_HR(hr);

	hr = m_pSource->CreatePresentationDescriptor(&pSourcePD);
	CHECK_HR(hr);

	hr = CreatePlaybackTopology(m_pSource, pSourcePD, m_hwndVideo, &pTopology);
	CHECK_HR(hr);

	hr = m_pSession->SetTopology(0, pTopology);
	CHECK_HR(hr);

	// 释放之前播放过的资源
	if (pSourceTmp)
	{
		hr = pSourceTmp->Shutdown();
		CHECK_HR(hr);
	}
	m_state = OpenPending;

done:
	if (FAILED(hr))
		m_state = Closed;

	SafeRelease(&pMemStream);
	SafeRelease(&pByteStream);
	SafeRelease(&pSourcePD);
	SafeRelease(&pTopology);
	SafeRelease(&pSourceTmp);
	return hr;
}

//  Pause playback.
HRESULT CPlayer::Pause()    
{
    if (m_state != Started)
    {
        return MF_E_INVALIDREQUEST;
    }
    if (m_pSession == NULL || m_pSource == NULL)
    {
        return E_UNEXPECTED;
    }

    HRESULT hr = m_pSession->Pause();
    if (SUCCEEDED(hr))
    {
        m_state = Paused;
    }

    return hr;
}

// Stop playback.
HRESULT CPlayer::Stop()
{
    if (m_state != Started && m_state != Paused)
    {
        return MF_E_INVALIDREQUEST;
    }
    if (m_pSession == NULL)
    {
        return E_UNEXPECTED;
    }

    HRESULT hr = m_pSession->Stop();
    if (SUCCEEDED(hr))
    {
        m_state = Stopped;
    }
    return hr;
}

//  Repaint the video window. Call this method on WM_PAINT.
HRESULT CPlayer::Repaint()
{
	LOG_TRACE(logger) << "CPlayer::Repaint	(" << m_pVideoDisplay << ")";
	if (m_pVideoDisplay)
    {
        return m_pVideoDisplay->RepaintVideo();
    }
    else
    {
        return S_OK;
    }
}

//  Resize the video rectangle.
//
//  Call this method if the size of the video window changes.
HRESULT CPlayer::ResizeVideo(WORD width, WORD height)
{
	LOG_TRACE(logger) << "CPlayer::ResizeVideo	(" << m_pVideoDisplay << ")";
	if (m_pVideoDisplay)
    {
        // Set the destination rectangle.
        // Leave the default source rectangle (0,0,1,1).

        RECT rcDest = { 0, 0, width, height };

        return m_pVideoDisplay->SetVideoPosition(NULL, &rcDest);
    }
    else
    {
        return S_OK;
    }
}

//  Callback for the asynchronous BeginGetEvent method.
HRESULT CPlayer::Invoke(IMFAsyncResult *pResult)
{
	MediaEventType meType = MEUnknown;  // Event type

    IMFMediaEvent *pEvent = NULL;

    HRESULT hr = m_pSession->EndGetEvent(pResult, &pEvent);
	CHECK_HR(hr);

    hr = pEvent->GetType(&meType);
	CHECK_HR(hr);

    if (meType == MESessionClosed)
    {
        SetEvent(m_hCloseEvent);
    }
    else
    {
        hr = m_pSession->BeginGetEvent(this, NULL);
		CHECK_HR(hr);
	}

    if (m_state != Closing)
    {
        pEvent->AddRef();

        PostMessage(m_hwndEvent, WM_APP_PLAYER_EVENT, 
            (WPARAM)pEvent, (LPARAM)meType);
    }

done:
    SafeRelease(&pEvent);
    return S_OK;
}

HRESULT CPlayer::HandleEvent(UINT_PTR pEventPtr)
{
	BOOST_LOG_FUNCTION();
	HRESULT hrStatus = S_OK;
    MediaEventType meType = MEUnknown;  

    IMFMediaEvent *pEvent = (IMFMediaEvent*)pEventPtr;

    if (pEvent == NULL)
    {
		LOG_ERROR(logger) << "E_POINTER";
        return E_POINTER;
    }

    // Get the event type.
    HRESULT hr = pEvent->GetType(&meType);
	CHECK_HR(hr);

    // Get the event status. If the operation that triggered the event 
    // did not succeed, the status is a failure code.
    hr = pEvent->GetStatus(&hrStatus);

    // Check if the async operation succeeded.
    if (SUCCEEDED(hr) && FAILED(hrStatus)) 
    {
		LOG_ERROR(logger) << "async operation failed, event:" << meType;
		hr = hrStatus;
    }
	CHECK_HR(hr);

    switch(meType)
    {
    case MESessionTopologyStatus:
        hr = OnTopologyStatus(pEvent);
        break;

    case MEEndOfPresentation:
        hr = OnPresentationEnded(pEvent);
        break;

    case MENewPresentation:
        hr = OnNewPresentation(pEvent);
        break;

	case MESessionEnded:
		LOG_DEBUG(logger) << "MediaEvent: MESessionEnded";
		::Sleep(1000);
		NotifyPlay();
		break;

	default:
        hr = OnSessionEvent(pEvent, meType);
        break;
    }

done:
    SafeRelease(&pEvent);
    return hr;
}

//  Release all resources held by this object.
HRESULT CPlayer::Shutdown()
{
    // Close the session
    HRESULT hr = CloseSession();

    // Shutdown the Media Foundation platform
	if (lpfMFShutdown)
		lpfMFShutdown();

    if (m_hCloseEvent)
    {
        CloseHandle(m_hCloseEvent);
        m_hCloseEvent = NULL;
    }

    return hr;
}

HRESULT CPlayer::OnTopologyStatus(IMFMediaEvent *pEvent)
{
	HRESULT hr = S_OK;
	TOPOID TopoID = 0;
	IMFTopology *pTopology = NULL;
	IMFTopology *pFullTopo = NULL;

    UINT32 status = 0;
    hr = pEvent->GetUINT32(MF_EVENT_TOPOLOGY_STATUS, &status);
	CHECK_HR(hr);

	switch (status)
	{
	case MF_TOPOSTATUS_READY:
		LOG_DEBUG(logger) << "MediaEvent: MF_TOPOSTATUS_READY";
		hr = GetEventObject(pEvent, &pTopology);	// 防止开始的闪烁
		CHECK_HR(hr);
		hr = pTopology->GetTopologyID(&TopoID);
		CHECK_HR(hr);
		hr = m_pSession->GetFullTopology(0, TopoID, &pFullTopo);
		CHECK_HR(hr);

		SafeRelease(&m_pVideoDisplay);
		lpfMFGetService(m_pSession, MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&m_pVideoDisplay));
		hr = StartPlayback();
		CHECK_HR(hr);
		break;

	case MF_TOPOSTATUS_STARTED_SOURCE:
		break;

	case MF_TOPOSTATUS_ENDED:
		LOG_DEBUG(logger) << "MediaEvent: MF_TOPOSTATUS_ENDED";
		hr = m_pSession->SetTopology(MFSESSION_SETTOPOLOGY_CLEAR_CURRENT, NULL);
		CHECK_HR(hr);
		break;
	}

done:
	if (FAILED(hr))
		LOG_DEBUG(logger) << "MediaEvent: " << status;
	SafeRelease(&pTopology);
	SafeRelease(&pFullTopo);
	return hr;
}


//  Handler for MEEndOfPresentation event.
HRESULT CPlayer::OnPresentationEnded(IMFMediaEvent *pEvent)
{
    // The session puts itself into the stopped state automatically.
    m_state = Stopped;
    return S_OK;
}

//  Handler for MENewPresentation event.
//
//  This event is sent if the media source has a new presentation, which 
//  requires a new topology. 
HRESULT CPlayer::OnNewPresentation(IMFMediaEvent *pEvent)
{
    IMFPresentationDescriptor *pPD = NULL;
    IMFTopology *pTopology = NULL;

    // Get the presentation descriptor from the event.
    HRESULT hr = GetEventObject(pEvent, &pPD);
	CHECK_HR(hr);

    // Create a partial topology.
    hr = CreatePlaybackTopology(m_pSource, pPD,  m_hwndVideo,&pTopology);
	CHECK_HR(hr);

    // Set the topology on the media session.
    hr = m_pSession->SetTopology(0, pTopology);
	CHECK_HR(hr);

    m_state = OpenPending;

done:
    SafeRelease(&pTopology);
    SafeRelease(&pPD);
    return S_OK;
}

HRESULT CPlayer::CreateSession()
{
    HRESULT hr = CloseSession();
	CHECK_HR(hr);

    assert(m_state == Closed);

    hr = lpfMFCreateMediaSession(NULL, &m_pSession);
	CHECK_HR(hr);

    hr = m_pSession->BeginGetEvent((IMFAsyncCallback*)this, NULL);
	CHECK_HR(hr);

    m_state = Ready;

done:
    return hr;
}

//  Close the media session. 
HRESULT CPlayer::CloseSession()
{
	FUNC_TRACER("CPlayer::CloseSession");
	BOOST_LOG_FUNCTION();
	HRESULT hr = S_OK;

    SafeRelease(&m_pVideoDisplay);

    // First close the media session.
    if (m_pSession)
    {
        DWORD dwWaitResult = 0;

        m_state = Closing;
           
        hr = m_pSession->Close();
        // Wait for the close operation to complete
        if (SUCCEEDED(hr))
        {
            dwWaitResult = WaitForSingleObject(m_hCloseEvent, 5000);
            if (dwWaitResult == WAIT_TIMEOUT)
            {
				LOG_ERROR(logger) << "timeout while waiting for Session->Close";
                assert(FALSE);
            }
            // Now there will be no more events from this session.
        }
    }

    // Complete shutdown operations.
    if (SUCCEEDED(hr))
    {
        // Shut down the media source. (Synchronous operation, no events.)
        if (m_pSource)
        {
            (void)m_pSource->Shutdown();
        }
        // Shut down the media session. (Synchronous operation, no events.)
        if (m_pSession)
        {
            (void)m_pSession->Shutdown();
        }
    }

    SafeRelease(&m_pSource);
    SafeRelease(&m_pSession);
    m_state = Closed;
    return hr;
}

//  Start playback from the current position. 
HRESULT CPlayer::StartPlayback()
{
	FUNC_TRACER("CPlayer::StartPlayback");
	assert(m_pSession != NULL);

    PROPVARIANT varStart;
    PropVariantInit(&varStart);

    HRESULT hr = m_pSession->Start(&GUID_NULL, &varStart);
    if (SUCCEEDED(hr))
    {
        m_state = Started;
    }
    PropVariantClear(&varStart);
    return hr;
}

//  Start playback from paused or stopped.
HRESULT CPlayer::Play()
{
    if (m_state != Paused && m_state != Stopped)
    {
        return MF_E_INVALIDREQUEST;
    }
    if (m_pSession == NULL || m_pSource == NULL)
    {
        return E_UNEXPECTED;
    }
    return StartPlayback();
}


//  Create a media source from a URL.
HRESULT CreateMediaSource(PCWSTR sURL, IMFMediaSource **ppSource)
{
    MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;

    IMFSourceResolver* pSourceResolver = NULL;
    IUnknown* pSource = NULL;

    HRESULT hr = lpfMFCreateSourceResolver(&pSourceResolver);
	CHECK_HR(hr);

    hr = pSourceResolver->CreateObjectFromURL(
        sURL,                       // URL of the source.
        MF_RESOLUTION_MEDIASOURCE,  // Create a source object.
        NULL,                       // Optional property store.
        &ObjectType,        // Receives the created object type. 
        &pSource            // Receives a pointer to the media source.
        );
	CHECK_HR(hr);

    // Get the IMFMediaSource interface from the media source.
    hr = pSource->QueryInterface(IID_PPV_ARGS(ppSource));

done:
    SafeRelease(&pSourceResolver);
    SafeRelease(&pSource);
    return hr;
}

//  Create an activation object for a renderer, based on the stream media type.

HRESULT CreateMediaSinkActivate(
    IMFStreamDescriptor *pSourceSD,     // Pointer to the stream descriptor.
    HWND hVideoWindow,                  // Handle to the video clipping window.
    IMFActivate **ppActivate
)
{
    IMFMediaTypeHandler *pHandler = NULL;
    IMFActivate *pActivate = NULL;

    // Get the media type handler for the stream.
    HRESULT hr = pSourceSD->GetMediaTypeHandler(&pHandler);
	CHECK_HR(hr);

    // Get the major media type.
    GUID guidMajorType;
    hr = pHandler->GetMajorType(&guidMajorType);
	CHECK_HR(hr);

    // Create an IMFActivate object for the renderer, based on the media type.
    if (MFMediaType_Audio == guidMajorType)
    {
        // Create the audio renderer.
        hr = lpfMFCreateAudioRendererActivate(&pActivate);
    }
    else if (MFMediaType_Video == guidMajorType)
    {
        // Create the video renderer.
        hr = lpfMFCreateVideoRendererActivate(hVideoWindow, &pActivate);
    }
    else
    {
        // Unknown stream type. 
        hr = E_FAIL;
        // Optionally, you could deselect this stream instead of failing.
    }
	CHECK_HR(hr);

    // Return IMFActivate pointer to caller.
    *ppActivate = pActivate;
    (*ppActivate)->AddRef();

done:
    SafeRelease(&pHandler);
    SafeRelease(&pActivate);
    return hr;
}

// Add a source node to a topology.
HRESULT AddSourceNode(
    IMFTopology *pTopology,           // Topology.
    IMFMediaSource *pSource,          // Media source.
    IMFPresentationDescriptor *pPD,   // Presentation descriptor.
    IMFStreamDescriptor *pSD,         // Stream descriptor.
    IMFTopologyNode **ppNode)         // Receives the node pointer.
{
    IMFTopologyNode *pNode = NULL;

    // Create the node.
    HRESULT hr = lpfMFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &pNode);
	CHECK_HR(hr);

    // Set the attributes.
    hr = pNode->SetUnknown(MF_TOPONODE_SOURCE, pSource);
	CHECK_HR(hr);

    hr = pNode->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, pPD);
	CHECK_HR(hr);

    hr = pNode->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, pSD);
	CHECK_HR(hr);

    // Add the node to the topology.
    hr = pTopology->AddNode(pNode);
	CHECK_HR(hr);

    // Return the pointer to the caller.
    *ppNode = pNode;
    (*ppNode)->AddRef();

done:
    SafeRelease(&pNode);
    return hr;
}

// Add an output node to a topology.
HRESULT AddOutputNode(
    IMFTopology *pTopology,     // Topology.
    IMFActivate *pActivate,     // Media sink activation object.
    DWORD dwId,                 // Identifier of the stream sink.
    IMFTopologyNode **ppNode)   // Receives the node pointer.
{
    IMFTopologyNode *pNode = NULL;

    // Create the node.
    HRESULT hr = lpfMFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &pNode);
	CHECK_HR(hr);

    // Set the object pointer.
    hr = pNode->SetObject(pActivate);
	CHECK_HR(hr);

    // Set the stream sink ID attribute.
    hr = pNode->SetUINT32(MF_TOPONODE_STREAMID, dwId);
	CHECK_HR(hr);

    hr = pNode->SetUINT32(MF_TOPONODE_NOSHUTDOWN_ON_REMOVE, FALSE);
	CHECK_HR(hr);

    // Add the node to the topology.
    hr = pTopology->AddNode(pNode);
	CHECK_HR(hr);

    // Return the pointer to the caller.
    *ppNode = pNode;
    (*ppNode)->AddRef();

done:
    SafeRelease(&pNode);
    return hr;
}
//</SnippetPlayer.cpp>

//  Add a topology branch for one stream.
//
//  For each stream, this function does the following:
//
//    1. Creates a source node associated with the stream. 
//    2. Creates an output node for the renderer. 
//    3. Connects the two nodes.
//
//  The media session will add any decoders that are needed.

HRESULT AddBranchToPartialTopology(
    IMFTopology *pTopology,         // Topology.
    IMFMediaSource *pSource,        // Media source.
    IMFPresentationDescriptor *pPD, // Presentation descriptor.
    DWORD iStream,                  // Stream index.
    HWND hVideoWnd)                 // Window for video playback.
{
    IMFStreamDescriptor *pSD = NULL;
    IMFActivate         *pSinkActivate = NULL;
    IMFTopologyNode     *pSourceNode = NULL;
    IMFTopologyNode     *pOutputNode = NULL;

    BOOL fSelected = FALSE;

    HRESULT hr = pPD->GetStreamDescriptorByIndex(iStream, &fSelected, &pSD);
	CHECK_HR(hr);

    if (fSelected)
    {
        // Create the media sink activation object.
        hr = CreateMediaSinkActivate(pSD, hVideoWnd, &pSinkActivate);
		CHECK_HR(hr);

        // Add a source node for this stream.
        hr = AddSourceNode(pTopology, pSource, pPD, pSD, &pSourceNode);
		CHECK_HR(hr);

        // Create the output node for the renderer.
        hr = AddOutputNode(pTopology, pSinkActivate, 0, &pOutputNode);
		CHECK_HR(hr);

        // Connect the source node to the output node.
        hr = pSourceNode->ConnectOutput(0, pOutputNode, 0);
    }
    // else: If not selected, don't add the branch. 

done:
    SafeRelease(&pSD);
    SafeRelease(&pSinkActivate);
    SafeRelease(&pSourceNode);
    SafeRelease(&pOutputNode);
    return hr;
}

//  Create a playback topology from a media source.
HRESULT CreatePlaybackTopology(
    IMFMediaSource *pSource,          // Media source.
    IMFPresentationDescriptor *pPD,   // Presentation descriptor.
    HWND hVideoWnd,                   // Video window.
    IMFTopology **ppTopology)         // Receives a pointer to the topology.
{
    IMFTopology *pTopology = NULL;
    DWORD cSourceStreams = 0;

    // Create a new topology.
    HRESULT hr = lpfMFCreateTopology(&pTopology);
	CHECK_HR(hr);

    // Get the number of streams in the media source.
    hr = pPD->GetStreamDescriptorCount(&cSourceStreams);
	CHECK_HR(hr);

    // For each stream, create the topology nodes and add them to the topology.
    for (DWORD i = 0; i < cSourceStreams; i++)
    {
        hr = AddBranchToPartialTopology(pTopology, pSource, pPD, i, hVideoWnd);
		CHECK_HR(hr);
	}

    // Return the IMFTopology pointer to the caller.
    *ppTopology = pTopology;
    (*ppTopology)->AddRef();

done:
    SafeRelease(&pTopology);
    return hr;
}

HRESULT CreateMediaSource(LPCWSTR pwszFileName, IMFByteStream *pByteStream, IMFMediaSource **ppSource)
{
	MF_OBJECT_TYPE ObjectType = MF_OBJECT_INVALID;

	IMFSourceResolver* pSourceResolver = NULL;
	IUnknown* pSource = NULL;

	HRESULT hr = lpfMFCreateSourceResolver(&pSourceResolver);
	CHECK_HR(hr);

	hr = pSourceResolver->CreateObjectFromByteStream(
		pByteStream,
		pwszFileName,
		MF_RESOLUTION_MEDIASOURCE,
		NULL,
		&ObjectType,
		&pSource
	);
	CHECK_HR(hr);
	MF_E_UNSUPPORTED_BYTESTREAM_TYPE;	// 0xC00D36C4
	
	// Get the IMFMediaSource interface from the media source.
	hr = pSource->QueryInterface(IID_PPV_ARGS(ppSource));

done:
	SafeRelease(&pSourceResolver);
	SafeRelease(&pSource);
	return hr;
}

void CPlayer::UpdatePlayList(uint32_t id, std::shared_ptr<std::string> adfile)
{
	for (auto it = _playList.begin(); it != _playList.end(); it++)
	{
		if (it->id == id)
			it->adfile = adfile;
	}
}

void CPlayer::NotifyPlay()
{
	int count = _playList.size();
	for (int i = 0; i < count; i++)
	{
		auto item = _playList.front();
		_playList.pop_front();
		_playList.push_back(item);
		if (item.type == 1 && item.adfile)
		{
			LOG_DEBUG(logger) << "开始播放" << item.filename;
			OpenMem(item.filename, (BYTE*)item.adfile->c_str(), item.adfile->length());
			break;
		}
	}
}

static boost::atomic_bool ready(false);
void CPlayer::SetVideoWindow(HWND hVideo)
{
	BOOST_LOG_FUNCTION();
	m_hwndVideo = hVideo;
	m_hwndEvent = hVideo;

	HRESULT hr = lpfMFStartup(MF_VERSION, MFSTARTUP_FULL);	// MFSTARTUP_FULL	MFSTARTUP_NOSOCKET
	if (FAILED(hr))
	{
		LOG_ERROR(logger) << std::showbase << std::uppercase << std::hex << "MFStartup:" << hr << std::noshowbase << std::nouppercase << std::dec; \
		return;
	}

	m_hCloseEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (m_hCloseEvent == NULL)
	{
		LOG_ERROR(logger) << "CreateEvent:" << GetLastError();
		return;
	}

	CreateSession();
	for (int i = 0; i < 30; i++)
	{
		if (ready)
		{
			NotifyPlay();
			break;
		}
		else
		{
			::Sleep(100);
		}
	}
}

void CPlayer::SetMediaSourceReady(bool isReady)
{
	ready = isReady;
}

BOOL GetProcAddresses(HINSTANCE *hLibrary, LPCSTR lpszLibrary, INT nCount, ...)
{
	va_list va;
	va_start(va, nCount);

	if ((*hLibrary = LoadLibrary(lpszLibrary)) != NULL)
	{
		FARPROC * lpfProcFunction = NULL;
		LPSTR lpszFuncName = NULL;
		INT nIdxCount = 0;
		while (nIdxCount < nCount)
		{
			lpfProcFunction = va_arg(va, FARPROC*);
			lpszFuncName = va_arg(va, LPSTR);
			if ((*lpfProcFunction =
				GetProcAddress(*hLibrary,
					lpszFuncName)) == NULL)
			{
				LOG_ERROR(logger) << "GetProcAddress " << lpszLibrary << lpszFuncName << ":" << GetLastError();
				lpfProcFunction = NULL;
				return FALSE;
			}
			nIdxCount++;
		}
	}
	else
	{
		LOG_ERROR(logger) << "LoadLibrary " << lpszLibrary << ":" << GetLastError();
		va_end(va);
		return FALSE;
	}
	va_end(va);
	return TRUE;
}