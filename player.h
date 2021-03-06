#pragma once

#include <new>
#include <windows.h>
#include <shobjidl.h> 
#include <shlwapi.h>
#include <assert.h>
#include <strsafe.h>
#include <memory>
#include <deque>
// Media Foundation headers
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <evr.h>
#include <stdint.h>

typedef HRESULT (WINAPI *_MFCreateMFByteStreamOnStream)(IStream*, IMFByteStream**);
typedef HRESULT (WINAPI *_MFCreateSourceResolver)(IMFSourceResolver **);
typedef HRESULT (WINAPI *_MFShutdown)(void);
typedef HRESULT (WINAPI *_MFStartup)(ULONG, DWORD);

typedef HRESULT (WINAPI *_MFCreateAudioRendererActivate)(IMFActivate**);
typedef HRESULT (WINAPI *_MFCreateMediaSession)(IMFAttributes*,IMFMediaSession**);
typedef HRESULT (WINAPI *_MFCreateTopology)(IMFTopology**);
typedef HRESULT (WINAPI *_MFCreateTopologyNode)(MF_TOPOLOGY_TYPE, IMFTopologyNode**);
typedef HRESULT (WINAPI *_MFCreateVideoRendererActivate)(HWND, IMFActivate**);
typedef HRESULT (WINAPI *_MFGetService)(IUnknown *, REFGUID, REFIID, LPVOID*);

BOOL GetProcAddresses(HINSTANCE *hLibrary, LPCSTR lpszLibrary, INT nCount, ...);

template <class T> void SafeRelease(T **ppT)
{
    if (*ppT)
    {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

enum PlayerState
{
    Closed = 0,     // No session.
    Ready,          // Session was created, ready to open a file. 
    OpenPending,    // Session is opening a file.
    Started,        // Session is playing a file.
    Paused,         // Session is paused.
    Stopped,        // Session is stopped (ready to play). 
    Closing         // Application has closed the session, but is waiting for MESessionClosed.
};

struct PlayItem
{
	uint32_t id;
	uint32_t type;		// 视频-1，本地Web页面-2，超链接-3，图片-4，图标-5
	std::string filename;
	std::shared_ptr<std::string> adfile;
};

class CPlayer : public IMFAsyncCallback
{
public:
	std::deque<PlayItem> _playList;	// 需要多线程同步
	void UpdatePlayList(uint32_t id, std::shared_ptr<std::string> adfile);
	void NotifyPlay();
	void SetVideoWindow(HWND hVideo);
	void SetMediaSourceReady(bool isReady);

	static HRESULT CreateInstance(HWND hVideo, HWND hEvent, CPlayer **ppPlayer);

    // IUnknown methods
    STDMETHODIMP QueryInterface(REFIID iid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();

    // IMFAsyncCallback methods
    STDMETHODIMP  GetParameters(DWORD*, DWORD*)
    {
        // Implementation of this method is optional.
        return E_NOTIMPL;
    }
    STDMETHODIMP  Invoke(IMFAsyncResult* pAsyncResult);

    // Playback
	HRESULT       OpenMem(std::string name, const BYTE *pBuf, const int len);
	HRESULT       OpenURL(const WCHAR *sURL);
    HRESULT       Play();
    HRESULT       Pause();
    HRESULT       Stop();
    HRESULT       Shutdown();
    HRESULT       HandleEvent(UINT_PTR pUnkPtr);
    PlayerState   GetState() const { return m_state; }

    // Video functionality
    HRESULT       Repaint();
    HRESULT       ResizeVideo(WORD width, WORD height);
    
    BOOL          HasVideo() const { return (m_pVideoDisplay != NULL);  }

protected:
    
    // Constructor is private. Use static CreateInstance method to instantiate.
    CPlayer(HWND hVideo, HWND hEvent);

    // Destructor is private. Caller should call Release.
    virtual ~CPlayer(); 

    HRESULT Initialize();
    HRESULT CreateSession();
    HRESULT CloseSession();
    HRESULT StartPlayback();

    // Media event handlers
    virtual HRESULT OnTopologyStatus(IMFMediaEvent *pEvent);
    virtual HRESULT OnPresentationEnded(IMFMediaEvent *pEvent);
    virtual HRESULT OnNewPresentation(IMFMediaEvent *pEvent);

    // Override to handle additional session events.
    virtual HRESULT OnSessionEvent(IMFMediaEvent*, MediaEventType) 
    { 
        return S_OK; 
    }

protected:
    long                    m_nRefCount;        // Reference count.

    IMFMediaSession         *m_pSession;
    IMFMediaSource          *m_pSource;
    IMFVideoDisplayControl  *m_pVideoDisplay;

    HWND                    m_hwndVideo;        // Video window.
    HWND                    m_hwndEvent;        // App window to receive events.
    PlayerState             m_state;            // Current state of the media session.
    HANDLE                  m_hCloseEvent;      // Event to wait on while closing.
};
