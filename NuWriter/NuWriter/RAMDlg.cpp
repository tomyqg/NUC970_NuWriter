// RAMDlg.cpp : implementation file
//
#include "stdafx.h"
#include "NuWriter.h"
#include "NuWriterDlg.h"


// CRAMDlg dialog

IMPLEMENT_DYNAMIC(CRAMDlg, CDialog)

CRAMDlg::CRAMDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CRAMDlg::IDD, pParent)
{
	m_fileinfo = _T("");
	m_address = _T("");
	m_autorun = 0;
	InitFlag=0;
}

CRAMDlg::~CRAMDlg()
{
}

void CRAMDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);	

	DDX_Control(pDX, IDC_SDRAM_DOWNPROGRESS, m_progress);
	DDX_Text(pDX, IDC_SDRAM_FILENAME, m_fileinfo);
	DDX_Text(pDX, IDC_SDRAM_BUFFER_ADDRESS, m_address);
	DDV_MaxChars(pDX, m_address, 8);
	DDX_Radio(pDX, IDC_SDRAM_AUTORUN, m_autorun);	
	DDX_Control(pDX, IDC_SDRAMSTATUS, sdramstatus);

	DDX_Control(pDX, IDC_SDRAM_DOWNLOAD, m_download);
	DDX_Control(pDX, IDC_SDRAM_BROWSE, m_browse);
}


BEGIN_MESSAGE_MAP(CRAMDlg, CDialog)
	ON_BN_CLICKED(IDC_SDRAM_DOWNLOAD, &CRAMDlg::OnBnClickedSdramDownload)
	ON_BN_CLICKED(IDC_SDRAM_BROWSE, &CRAMDlg::OnBnClickedSdramBrowse)
	ON_MESSAGE(WM_SDRAM_PROGRESS,ShowStatus)
	ON_WM_CTLCOLOR()
	ON_WM_SHOWWINDOW()
END_MESSAGE_MAP()


// CRAMDlg message handlers
BOOL CRAMDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();

	m_ExitEvent=CreateEvent(NULL,TRUE,FALSE,NULL);	
	
	m_progress.SetRange(0,100);	
	m_progress.SetBkColor(COLOR_DOWNLOAD);
	m_fileinfo=_T(" ...");
	m_filename=_T("");
	m_address=_T("8000");
	UpdateData(FALSE);

	CNuWriterDlg* mainWnd=(CNuWriterDlg*)(AfxGetApp()->m_pMainWnd);
	if(mainWnd->m_inifile.ReadFile())
	{
		CString _path=mainWnd->m_inifile.GetValue(_T("SDRAM"),_T("PATH"));
		GetDlgItem(IDC_SDRAM_FILENAME)->SetWindowText(_path);
		m_filename=_path;
	}

	COLORREF col = RGB(0xFF, 0x00, 0xFF);
	m_download.setBitmapId(IDB_WRITE_DEVICE, col);
	m_download.setGradient(true);
	m_browse.setBitmapId(IDB_BROWSE, col);
	m_browse.setGradient(true);	

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

BOOL CRAMDlg::InitFile(int flag)
{
	CString tmp;
	CNuWriterDlg* mainWnd=(CNuWriterDlg*)(AfxGetApp()->m_pMainWnd);
	if(!mainWnd->m_inifile.ReadFile()) return false;
	switch(flag)
	{
		case 0:
			m_filename=mainWnd->m_inifile.GetValue(_T("SDRAM"),_T("PATH"));	
			GetDlgItem(IDC_SDRAM_FILENAME)->SetWindowText(m_filename);
			tmp=mainWnd->m_inifile.GetValue(_T("SDRAM"),_T("EXEADDR"));
			GetDlgItem(IDC_SDRAM_BUFFER_ADDRESS)->SetWindowText(tmp);			
			tmp=mainWnd->m_inifile.GetValue(_T("SDRAM"),_T("TYPE"));			
			((CButton *)GetDlgItem(IDC_SDRAM_AUTORUN))->SetCheck(FALSE);
			switch(_wtoi(tmp))
			{
				case 0: ((CButton *)GetDlgItem(IDC_SDRAM_AUTORUN))->SetCheck(TRUE); break;
				case 1: ((CButton *)GetDlgItem(IDC_SDRAM_NOAUTORUN))->SetCheck(TRUE); break;				
			}
			break;
		case 1:
			tmp.Format(_T("%d"),m_autorun);
			mainWnd->m_inifile.SetValue(_T("SDRAM"),_T("TYPE"),tmp);	
			mainWnd->m_inifile.SetValue(_T("SDRAM"),_T("PATH"),m_filename);
			mainWnd->m_inifile.SetValue(_T("SDRAM"),_T("EXEADDR"),m_address);
			mainWnd->m_inifile.WriteFile();
			break;
	}
return true;
}

void CRAMDlg::OnShowWindow(BOOL bShow, UINT nStatus)
{
	CDialog::OnShowWindow(bShow, nStatus);

	
	// TODO: Add your message handler code here		
	if(InitFlag==0)
	{
		InitFlag=1;
		InitFile(0);
	}
	//UpdateData (TRUE);
}

unsigned WINAPI CRAMDlg:: Download_proc(void* args)
{
	CRAMDlg* pThis = reinterpret_cast<CRAMDlg*>(args);
	pThis->Download();
	return 0;
}

void CRAMDlg::OnBnClickedSdramDownload()
{

	CString dlgText;
	
	UpdateData(TRUE);
	InitFile(1);
	if(m_address.IsEmpty())
	{
		AfxMessageBox(_T("Please input execute address"));
		return;
	}

	GetDlgItem(IDC_SDRAM_DOWNLOAD)->GetWindowText(dlgText);

	if(dlgText.CompareNoCase(_T("Download"))==0)
	{

		UpdateData(TRUE);

		unsigned Thread1;
		HANDLE hThread;
		
	 	m_progress.SetPos(0);
		m_progress.SetBkColor(COLOR_DOWNLOAD);		
		hThread=(HANDLE)_beginthreadex(NULL,0,Download_proc,(void*)this,0,&Thread1);
		CloseHandle(hThread);
	}
	else
	{
		//m_vcom.Close();
		SetEvent(m_ExitEvent);
		GetDlgItem(IDC_SDRAM_DOWNLOAD)->EnableWindow(FALSE);
		m_progress.SetPos(0);
	}
}

void CRAMDlg::OnBnClickedSdramBrowse()
{
	//CAddFileDialog dlg(TRUE,NULL,NULL,OFN_FILEMUSTEXIST | OFN_HIDEREADONLY ,_T("Bin,Img Files  (*.bin;*.img;*.gz)|*.bin;*.img;*.gz|All Files (*.*)|*.*||"));
	CAddFileDialog dlg(TRUE,NULL,NULL,OFN_FILEMUSTEXIST | OFN_HIDEREADONLY ,_T("Bin Files (*.bin)|*.bin|All Files (*.*)|*.*||"));
	dlg.m_ofn.lpstrTitle=_T("Choose burning file...");

	CNuWriterDlg* mainWnd=(CNuWriterDlg*)(AfxGetApp()->m_pMainWnd);

	if(!mainWnd->m_inifile.ReadFile())
		dlg.m_ofn.lpstrInitialDir=_T("c:");
	else
	{
		CString _path;
		_path=mainWnd->m_inifile.GetValue(_T("SDRAM"),_T("PATH"));
		int len=_path.GetLength();		
		int i;
		for(i=len;i>0;i--)
		{
			if(_path.GetAt(i)=='\\')
			{			
				len=i;
				break;
			}
		}
		CString filepath=_path.Left(len);
		if(filepath.IsEmpty())
			dlg.m_ofn.lpstrInitialDir=_T("c:");
		else
			dlg.m_ofn.lpstrInitialDir=_path;
	}
		
	BOOLEAN ret=dlg.DoModal();

	if(ret==IDCANCEL)
	{
		return;
	}

	m_filename=dlg.GetPathName();
	m_fileinfo=_T(" ")+m_filename;
	UpdateData(FALSE);	

	mainWnd->m_inifile.SetValue(_T("SDRAM"),_T("PATH"),m_filename);
	mainWnd->m_inifile.WriteFile();
}

LRESULT CRAMDlg::ShowStatus( WPARAM  pos, LPARAM message)
{
	m_progress.SetPos((int)pos);
	return true;
}

void CRAMDlg:: Download()
{

	int i=0;
	CNuWriterDlg* mainWnd=(CNuWriterDlg*)(AfxGetApp()->m_pMainWnd);

	//UpdateData(FALSE);  //shanchun modified 20121003

	if(!m_filename.IsEmpty())
	{
		mainWnd->m_gtype.EnableWindow(FALSE);
		ResetEvent(m_ExitEvent);
		GetDlgItem(IDC_SDRAM_DOWNLOAD)->SetWindowText(_T("Abort"));
		GetDlgItem(IDC_SDRAM_DOWNLOAD)->EnableWindow(0);
		GetDlgItem(IDC_SDRAMSTATUS)->SetWindowText(_T("Download"));
		//load nuc970 ddr init 
		mainWnd->UpdateBufForDDR();
		
		if(XUSB(mainWnd->m_portName,m_filename))
		{
			GetDlgItem(IDC_SDRAMSTATUS)->SetWindowText(_T("Download successfully"));
			//AfxMessageBox(_T("Download successfully"));
		}
		else
		{
			GetDlgItem(IDC_SDRAMSTATUS)->SetWindowText(_T("Download failed!! Please check device"));
			//AfxMessageBox("Download unsuccessfully!! Please check device");
		}
		GetDlgItem(IDC_SDRAM_DOWNLOAD)->EnableWindow(TRUE);
		GetDlgItem(IDC_SDRAM_DOWNLOAD)->SetWindowText(_T("Download"));
		GetDlgItem(IDC_SDRAM_DOWNLOAD)->EnableWindow(1);
		//this->UpdateData(FALSE);  //shanchun modified 20121003
		mainWnd->m_gtype.EnableWindow(TRUE);

	}
	else
		AfxMessageBox(_T("Please choose image file !"));
	
	return ;

}

BOOL CRAMDlg::PreTranslateMessage(MSG* pMsg) 
{
	if((pMsg->message==WM_KEYDOWN)&&(pMsg->wParam==0x0d))
	{
		return TRUE;
	}
	
	return CDialog::PreTranslateMessage(pMsg);
}

HBRUSH CRAMDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialog::OnCtlColor(pDC, pWnd, nCtlColor);
	 if (pWnd->GetDlgCtrlID() == IDC_SDRAMSTATUS)
     {
		 CString word;
		 GetDlgItem(IDC_SDRAMSTATUS)->GetWindowText(word);
		 if(!word.Compare(_T("Download")))
			pDC->SetTextColor(RGB(255,0,0));		 
		 else
			pDC->SetTextColor(RGB(0,0,0));

		 if(!word.Compare(_T("Download failed!! Please check device")))
			pDC->SetTextColor(RGB(255,0,0));
		 else
			pDC->SetTextColor(RGB(0,0,0));
     } 
	 
	return hbr;
}