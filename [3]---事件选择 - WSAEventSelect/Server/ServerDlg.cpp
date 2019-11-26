
// ServerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Server.h"
#include "ServerDlg.h"
#define WM_SOCKET WM_USER+100

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CServerDlg dialog




CServerDlg::CServerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CServerDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_SockListen = INVALID_SOCKET;
	m_SockClient = INVALID_SOCKET;
	m_ClientNums = 0;
}

void CServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CServerDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BTN_START, &CServerDlg::OnBnClickedBtnStart)
	ON_BN_CLICKED(IDC_BTN_SEND, &CServerDlg::OnBnClickedBtnSend)
END_MESSAGE_MAP()


// CServerDlg message handlers

BOOL CServerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CServerDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CServerDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CServerDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

BOOL CServerDlg::WinSockInit()
{
	WSADATA data = {0};
	if(WSAStartup(MAKEWORD(2, 2), &data))
		return FALSE;
	if ( LOBYTE(data.wVersion) !=2 || HIBYTE(data.wVersion) != 2 ){
		WSACleanup();
		return FALSE;
	}
	return TRUE;
}

#define MAX_BUF_SIZE 4096
UINT CServerDlg::ThreadProc(LPVOID pParam)
{
	ASSERT(pParam);
	sockaddr_in clientAddr = {0};
	///////////////////////////////////////
	TCHAR szBuf[MAX_BUF_SIZE] = {0};
	SOCKET ArrSocket[WSA_MAXIMUM_WAIT_EVENTS] = {0};
	WSAEVENT ArrEvent[WSA_MAXIMUM_WAIT_EVENTS] = {0};
	DWORD dwTotal = 0, dwIndex = 0;
	WSANETWORKEVENTS m_NetWorkEvents = {0};
	/////////////////////////////////////////////////////
	CServerDlg *pThis = (CServerDlg *)pParam;
	pThis->WinSockInit();
	pThis->m_SockListen = INVALID_SOCKET;
	pThis->m_SockListen = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
	if ( pThis->m_SockListen == INVALID_SOCKET ) {
		AfxMessageBox(_T("新建Socket失败！"));
		goto __Error_End;
	}

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = INADDR_ANY;
	service.sin_port = htons(9527);

	if ( bind(pThis->m_SockListen, (sockaddr*)&service, sizeof(sockaddr_in)) == SOCKET_ERROR ) {
		AfxMessageBox(_T("绑定端口失败！"));
		goto __Error_End;
	}

	WSAEVENT m_ListenEvent = WSACreateEvent();

    // 创建 event 负责处理 accept 事件
	WSAEventSelect(pThis->m_SockListen, m_ListenEvent, FD_ACCEPT | FD_CLOSE);

	if( listen(pThis->m_SockListen, SOMAXCONN) == SOCKET_ERROR ) {
		AfxMessageBox(_T("监听失败！"));
		goto __Error_End;
	}

    pThis->ShowMsg(_T("监听端口：9527 成功！"));
    pThis->ShowMsg(_T("等待客户端接入..."));

	ArrSocket[dwTotal] = pThis->m_SockListen;
	ArrEvent[dwTotal] = m_ListenEvent;
	++dwTotal;

	while (TRUE) {
		dwIndex = WSAWaitForMultipleEvents(dwTotal, ArrEvent, FALSE, 100, FALSE);
		if (dwIndex == WSA_WAIT_TIMEOUT) {
			continue;
		}
		WSAEnumNetworkEvents(ArrSocket[dwIndex-WSA_WAIT_EVENT_0], ArrEvent[dwIndex-WSA_WAIT_EVENT_0], &m_NetWorkEvents);

		// accept the client;
		if (m_NetWorkEvents.lNetworkEvents & FD_ACCEPT) {
			if (m_NetWorkEvents.iErrorCode[FD_ACCEPT_BIT] != 0) {
				continue;
			}
			pThis->m_SockClient = accept(ArrSocket[dwIndex-WSA_WAIT_EVENT_0], NULL, NULL);
			if (pThis->m_SockClient == INVALID_SOCKET) {
				continue;
			}

            // 创建事件负责处理 读写事件
			WSAEVENT newEvent = WSACreateEvent();
			WSAEventSelect(pThis->m_SockClient, newEvent, FD_READ | FD_WRITE | FD_CLOSE);

			ArrSocket[dwTotal] = pThis->m_SockClient;
			ArrEvent[dwTotal] = newEvent;
			++dwTotal;
		}

		// recv the data;
		if (m_NetWorkEvents.lNetworkEvents & FD_READ) {
			if (m_NetWorkEvents.iErrorCode[FD_READ_BIT] != 0) {
				continue;
			}
			recv(ArrSocket[dwIndex-WSA_WAIT_EVENT_0], (char *)szBuf, MAX_BUF_SIZE, 0);
			pThis->ShowMsg(szBuf);
		}

		// send the data;
		if (m_NetWorkEvents.lNetworkEvents & FD_WRITE) {
			if (m_NetWorkEvents.iErrorCode[FD_WRITE_BIT] != 0) {
				continue;
			}
		}

		// close the socket;
		if (m_NetWorkEvents.lNetworkEvents & FD_CLOSE) {
			if (m_NetWorkEvents.iErrorCode[FD_CLOSE_BIT] != 0) {
				continue;
			}
			closesocket(ArrSocket[dwIndex-WSA_WAIT_EVENT_0]);
			//more works;
			//remove the socket and the event form the array, decrement the dwTotal;
			//your works;
		}
	}

__Error_End:
	if (pThis->m_SockListen != INVALID_SOCKET) {
		closesocket(pThis->m_SockListen);
	}

	WSACleanup();
	return TRUE;
}

void CServerDlg::ShowMsg(CString strMsg)
{
	CEdit *pEditShow = (CEdit *)GetDlgItem(IDC_EDIT_SHOW);
	ASSERT(pEditShow);
	pEditShow->SetSel(-1, -1);
	pEditShow->ReplaceSel(_T("Client : ") + strMsg+_T("\r\n"));
}

void CServerDlg::OnBnClickedBtnStart()
{
	AfxBeginThread(ThreadProc, this);
}

void CServerDlg::OnBnClickedBtnSend()
{
	CString strText;
	ASSERT(m_SockClient != INVALID_SOCKET);
	GetDlgItemText(IDC_EDIT_WRITE, strText);
	if (!strText.IsEmpty()) {
		send(m_SockClient, (char *)strText.GetBuffer(), strText.GetLength()*sizeof(TCHAR), 0);
		SetDlgItemText(IDC_EDIT_WRITE, _T(""));
	}
}
