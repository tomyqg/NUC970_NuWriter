// InitDlg.cpp : implementation file
//

#include "stdafx.h"
#include "NuWriter.h"
#include "NuWriterDlg.h"
#include "InitDlg.h"
#include "NucWinUsb.h"


// CInitDlg dialog

//IMPLEMENT_DYNAMIC(CInitDlg, CDialog)

CInitDlg::CInitDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CInitDlg::IDD, pParent)
{
	m_status = _T("");
}

//CInitDlg::~CInitDlg()
//{
//}

void CInitDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DOWNPROGRESS, m_progress);
	DDX_Text(pDX, IDC_DOWNLOADSTATUS, m_status);
}


BEGIN_MESSAGE_MAP(CInitDlg, CDialog)
	ON_MESSAGE(WM_CLOSEIT,Close)
	ON_MESSAGE(WM_SHOWPROGRESS,ShowProgress)
	ON_NOTIFY(NM_CUSTOMDRAW, IDC_DOWNPROGRESS, &CInitDlg::OnNMCustomdrawDownprogress)
END_MESSAGE_MAP()


// CInitDlg message handlers
LRESULT CInitDlg::ShowProgress( WPARAM  p1, LPARAM p2)
{
	INT i=(INT)p1;
	m_progress.SetPos(i);
	return true;
}

LRESULT CInitDlg::Close( WPARAM  p1, LPARAM p2)
{
	DestroyWindow();
	EndDialog(0);
	return true;
}

void CInitDlg::SetData()
{
//	m_port=port;
}

DWORD GetRamAddress(FILE* fp)
{
	BINHEADER head;
	UCHAR SIGNATURE[]={'W','B',0x5A,0xA5};	
	
	fread((CHAR*)&head,sizeof(head),1,fp);

	if(head.signature==*(DWORD*)SIGNATURE)
	{
		return head.address;
	}
	else
		return 0xFFFFFFFF;
	
}

int CInitDlg::DDRtoDevice(char *buf,unsigned int len)
{
	BOOL bResult;
	char *pbuf;	
	unsigned int scnt,rcnt,ack;
	AUTOTYPEHEAD head;
	CNuWriterDlg* mainWnd=(CNuWriterDlg*)(AfxGetApp()->m_pMainWnd);
	head.address=mainWnd->DDRAddress;
	head.filelen=len;
	Sleep(20);
	bResult=NucUsb.NUC_WritePipe(0,(unsigned char*)&head,sizeof(AUTOTYPEHEAD));
	if(bResult==FALSE) goto failed;
	Sleep(5);
	pbuf=buf;

	scnt=len/BUF_SIZE;
	rcnt=len%BUF_SIZE;	
	while(scnt>0)
	{
		bResult=NucUsb.NUC_WritePipe(0,(UCHAR *)pbuf,BUF_SIZE);
		if(bResult==TRUE)
		{
			bResult=NucUsb.NUC_ReadPipe(0,(UCHAR *)&ack,4);

			if(bResult==FALSE || (int)ack==(BUF_SIZE+1))
			{				
				
				if(bResult==TRUE && (int)ack==(BUF_SIZE+1))
				{
					// xub.bin is running on device
					NucUsb.NUC_CloseHandle();
					return FW_CODE;
				}
				 else
					goto failed;
			}		
		}		
		scnt--;
		pbuf+=BUF_SIZE;
	}

	if(rcnt>0)
	{
		bResult=NucUsb.NUC_WritePipe(0,(UCHAR *)pbuf,rcnt);
		if(bResult==TRUE)
		{
			bResult=NucUsb.NUC_ReadPipe(0,(UCHAR *)&ack,4);
			if(bResult==FALSE || (int)ack==(BUF_SIZE+1))
			{				
				
				if(bResult==TRUE && (int)ack==(BUF_SIZE+1))
				{
					// xub.bin is running on device
					NucUsb.NUC_CloseHandle();
					return FW_CODE;
				}
				 else
					goto failed;
			}
		}
	}

	 bResult=NucUsb.NUC_ReadPipe(0,(UCHAR *)&ack,4); 
	 if(bResult==TRUE && ack==0x55AA55AA)
		return TRUE;

failed:		
	NucUsb.NUC_CloseHandle();	
	return FALSE;
}


BOOL CInitDlg::XUSB(CString& m_pathName)
{
	BOOL bResult;
	CString posstr;
	CString tempstr;
	int count=0;
	FILE* fp;
	int pos=0;
	AUTOTYPEHEAD fhead;
	XBINHEAD xbinhead;
	DWORD version;
	unsigned int total,file_len,scnt,rcnt,ack;

	/***********************************************/
		NucUsb.EnableWinUsbDevice();
			
		int fw_flag;
		CNuWriterDlg* mainWnd=(CNuWriterDlg*)(AfxGetApp()->m_pMainWnd);

		mainWnd->UpdateBufForDDR();
		fw_flag=DDRtoDevice(mainWnd->DDRBuf,mainWnd->DDRLen);
		if(fw_flag==FALSE) return FALSE;
		if(fw_flag==FW_CODE) return TRUE;
				
		ULONG cbSize = 0;
		unsigned char* lpBuffer = new unsigned char[BUF_SIZE];		
		fp=_wfopen(m_pathName,_T("rb"));

		if(!fp)
		{
			delete []lpBuffer;
			NucUsb.NUC_CloseHandle();
			AfxMessageBox(_T("File Open error\n"));
			return FALSE;
		}

		fread((char*)&xbinhead,sizeof(XBINHEAD),1,fp);
		version=xbinhead.version;

		//07121401 -> 測桶 2007/12/14 - 01 唳
		m_version.Format(_T(" 20%02x/%02x/%02x-V%02x"),(version>>24)&0xff,(version>>16)&0xff,\
						(version>>8)&0xff,(version)&0xff);

		fseek(fp,0,SEEK_END);
		file_len=ftell(fp)-sizeof(XBINHEAD);
		fseek(fp,0,SEEK_SET);

		if(!file_len)
		{
			delete []lpBuffer;
			NucUsb.NUC_CloseHandle();
			AfxMessageBox(_T("File length is zero\n"));
			return FALSE;
		}
	
		fhead.filelen = file_len;
		fhead.address = GetRamAddress(fp);//0x8000;

		if(fhead.address==0xFFFFFFFF)
		{
			delete []lpBuffer;
			NucUsb.NUC_CloseHandle();			
			fclose(fp);
			AfxMessageBox(_T("Invalid Image !"));
			return FALSE;
		}

		memcpy(lpBuffer,(unsigned char*)&fhead,sizeof(fhead));
		bResult=NucUsb.NUC_WritePipe(0,(unsigned char*)&fhead,sizeof(fhead));
		if(bResult==FALSE)
		{
			delete []lpBuffer;			
			NucUsb.NUC_CloseHandle();
			fclose(fp);
			return FALSE;
		}			
		scnt=file_len/BUF_SIZE;
		rcnt=file_len%BUF_SIZE;
		
		total=0;

		while(scnt>0)
		{
			 fread(lpBuffer,BUF_SIZE,1,fp);
			 bResult=NucUsb.NUC_WritePipe(0,lpBuffer,BUF_SIZE);
			 if(bResult==TRUE)
			 {
				total+=BUF_SIZE;

				pos=(int)(((float)(((float)total/(float)file_len))*100));
				posstr.Format(_T("%d%%"),pos);

				bResult=NucUsb.NUC_ReadPipe(0,(UCHAR *)&ack,4);
				if(bResult==FALSE || (int)ack!=BUF_SIZE)
				{
					
					delete []lpBuffer;					
					NucUsb.NUC_CloseHandle();				
					fclose(fp);

					if(bResult==TRUE && (int)ack==(BUF_SIZE+1))
					{
						// xub.bin is running on device
						return TRUE;
					}else
						return FALSE;
				}

			 }

			 scnt--;

			 if(pos%5==0)
			 {
				PostMessage(WM_SHOWPROGRESS,(LPARAM)pos,0);
			 }
 		}

		if(rcnt>0)
		{
			fread(lpBuffer,rcnt,1,fp);
			bResult=NucUsb.NUC_WritePipe(0,lpBuffer,BUF_SIZE);
			if(bResult==TRUE)
			{
				total+=rcnt;								
				bResult=NucUsb.NUC_ReadPipe(0,(UCHAR *)&ack,4);
				if(bResult==FALSE)
				{
					delete []lpBuffer;					
					NucUsb.NUC_CloseHandle();					
					fclose(fp);
					return FALSE;
				}
			}

			pos=(int)(((float)(((float)total/(float)file_len))*100));

			if(pos>=100)
			{
				pos=100;
			}
			posstr.Format(_T("%d%%"),pos);

			if(pos%5==0)
			{
				PostMessage(WM_SHOWPROGRESS,(LPARAM)pos,0);
			}
		}

	delete []lpBuffer;
	NucUsb.NUC_CloseHandle();
	fclose(fp);
	return TRUE;

}
/////////////////////////////////////////////////////////////////////////////
// CInitDlg message handlers

BOOL CInitDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	unsigned Thread1;
	HANDLE hThread;

	// TODO: Add extra initialization here
	m_progress.SetRange(0,100);
	m_progress.SetBkColor(RGB(245, 92, 61));
	m_status="Initializing :";
	UpdateData(FALSE);	
	hThread=(HANDLE)_beginthreadex(NULL,0,Download_proc,(void*)this,0,&Thread1);	
	CloseHandle(hThread);
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CInitDlg:: Download()
{
	int i=0;
	TCHAR path[MAX_PATH]; 
	GetModuleFileName(NULL, path, MAX_PATH);  
	CString temp=path;
	CNuWriterDlg* mainWnd=(CNuWriterDlg*)(AfxGetApp()->m_pMainWnd);


	temp = temp.Left(temp.ReverseFind('\\') + 1);
	//OutputDebugString(temp);
	CString filename=NULL;	

	switch(mainWnd->DDRFileName.GetAt(8))
	{
		case '5':
			filename.Format(_T("%sxusb.bin"),temp);
			break;
		case '6':
			filename.Format(_T("%sxusb64.bin"),temp);
			break;
		case '7':
			filename.Format(_T("%sxusb128.bin"),temp);
			break;
		default:
			filename.Format(_T("%sxusb16.bin"),temp);
			break;
	};

	printf("download: %s\n", filename);
	XUSB(filename);
	//this->PostMessage(WM_CLOSEIT,0,0);
	this->PostMessage(WM_CLOSE,0,0);

	return ;
}

unsigned WINAPI CInitDlg:: Download_proc(void* args)
{
	CInitDlg* pThis = reinterpret_cast<CInitDlg*>(args);
	pThis->Download();
	return 0;
}

void CInitDlg::OnNMCustomdrawDownprogress(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);	
	*pResult = 0;
}

