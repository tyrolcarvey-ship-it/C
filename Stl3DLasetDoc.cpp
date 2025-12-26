
// Stl3DLasetDoc.cpp :  CStl3DLasetDoc 类的实现
//
//#include "pch.h"
#include "stdafx.h"
#include "Stl3DLaset.h"
#include "Fileview.h"
#include "Stl3DLasetDoc.h"
#include "Stl3DLasetView.h"
#include "MainFrm.h"
#include "GocatorDialogConf.h"
#include <crtdbg.h>
using namespace std;
//std::mutex mtx;
#include"CApplication.h"
#include"CFont0.h"
#include"CRange.h"
#include"CWorkbook.h"
#include"CWorkbooks.h"
#include"CWorksheet.h"
#include"CWorksheets.h"
#include <STEPCAFControl_Writer.hxx>
#include "WaferSetDlg.h"

//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif

#define WM_PCL_MESSAGE		(WM_USER + 102)

HWND	h_pOwner0;
// CStl3DLasetDoc
CStl3DLasetView* viewpointer;

IMPLEMENT_DYNCREATE(CStl3DLasetDoc, CDocument)

BEGIN_MESSAGE_MAP(CStl3DLasetDoc, CDocument)
	ON_COMMAND(IDD_FILE_STLDATA, OnFile3Ddata)
	ON_COMMAND(ID_OPEN_DATA, &CStl3DLasetDoc::OnOpenData)
	ON_COMMAND(ID_LOAD_PROBE, &CStl3DLasetDoc::OnLoadProbe)
	ON_COMMAND(ID_SCAN, &CStl3DLasetDoc::OnLoadScan)
	ON_COMMAND(ID_PARAME_SET, &CStl3DLasetDoc::OnParameSet)
	ON_COMMAND(ID_menuSensor, &CStl3DLasetDoc::OnMenuSensorConf)
	ON_COMMAND(ID_menuShowSensorData, &CStl3DLasetDoc::OnMenuShowSensorData)
	ON_COMMAND(ID_FILE_SAVE_AS, &CStl3DLasetDoc::OnFileSave)
	ON_COMMAND(ID_CPD2STL, &CStl3DLasetDoc::OnCpd2Stl)
END_MESSAGE_MAP()


// CStl3DLasetDoc 构造/析构

CStl3DLasetDoc::CStl3DLasetDoc() noexcept
{
	// TODO: 在此添加一次性构造代码
	pSTLModel_Pcd = NULL;
	pSTLModel = NULL;
	pSTLModel_CT = new CSTLModel();
	pSTLModel_TXT = new CSTLModel();
	//m_goManager = GocatorManager::Instance();
	//m_goManager->Init();
	Handle(OpenGl_GraphicDriver) theGraphicDriver = ((CStl3DLasetApp*)AfxGetApp())->GetGraphicDriver();
	myViewer = new V3d_Viewer(theGraphicDriver);
	myViewer->SetDefaultLights();
	myViewer->SetLightOn();
	//Quantity_Color background_color = Quantity_Color(152 / 255.0, 152 / 255.0, 152 / 255.0, Quantity_TOC_RGB);
	Quantity_Color background_color = Quantity_Color(1, 1, 1, Quantity_TOC_RGB);
	myViewer->SetDefaultBackgroundColor(background_color);//改变背景颜色
	myAISContext = new AIS_InteractiveContext(myViewer);  // 创建一个交互文档
	////myAISContext->SetDisplayMode(AIS_WireFrame, true);  //设置显示模式为网状体
	//myAISContext->SetDisplayMode(AIS_Shaded, true);     //设置显示模式为实体
}
CStl3DLasetDoc::~CStl3DLasetDoc()
{
    POSITION pos = m_objects.GetHeadPosition();
    while (pos != NULL)
        delete m_objects.GetNext(pos);
    delete pSTLModel_CT;
    delete pSTLModel_TXT;
}


BOOL CStl3DLasetDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	// TODO: 在此添加重新初始化代码
	// (SDI 文档将重用该文档)
	m_Part.RemoveAllEntity();

	return TRUE;
}


void CStl3DLasetDoc::Add(CFigureObj* pObj)
{
	m_objects.AddTail(pObj);
	pObj->m_pDocument = this;
	SetModifiedFlag();
}


// CStl3DLasetDoc 序列化

void CStl3DLasetDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
		// TODO: 在此添加存储代码
	}
	else
	{
		// TODO: 在此添加加载代码
	}
	//	SerializeCoordinateSystem(ar);
}


// CStl3DLasetDoc 诊断

#ifdef _DEBUG
void CStl3DLasetDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CStl3DLasetDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

void CStl3DLasetDoc::GetHwnd(HWND hWnd)
{
	h_pOwner0 = hWnd;
}
// CStl3DLasetDoc 命令
void CStl3DLasetDoc::Remove(CFigureObj* pObj)
{
	POSITION pos = m_objects.Find(pObj);
	if (pos != NULL)
	{
		m_objects.RemoveAt(pos);
		pObj->Remove();

	}
	SetModifiedFlag();
}
//停用
void ProgressUpdate(int add)
{
	//std::lock_guard<std::mutex> lock(mtx);
	static int progress = 0;
	//::SendMessage(h_pOwner0, WM_USER_UPDATE_PROGRESS, ++progress, 0);
	//CStl3DLasetView* pView = (CStl3DLasetView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1));
	//viewpointer->ProCtr.OffsetPos(++progress);
	//CString pos; pos.Format(_T("%d%%"), (pView->ProCtr.GetPos() - 1) * 10);
	//pView->showprogress.SetWindowTextA(pos);
}
//开线程打开stl文件，主线程更新进度条
void CStl3DLasetDoc::Import_Stl(CString name, INT pyte)
{
	// 添加线程同步
	static CCriticalSection cs;
	CSingleLock lock(&cs, TRUE);

	((CMainFrame*)AfxGetMainWnd())->SwitchView(1);
	CStl3DLasetView* pView = (CStl3DLasetView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1));

	// 检查视图有效性
	if (!pView || !::IsWindow(pView->GetSafeHwnd())) {
		AfxMessageBox(_T("视图无效或已销毁"));
		return;
	}

	viewpointer = pView;
	viewpointer->m_bRenderScene = 1;
	pView->viewClear();
	ClosePCDView();

	// 检查DC有效性
	if (pView->m_pDC) {
		pView->m_pDC->m_Camera.init();
		pView->m_pDC->m_Camera.set_screen(pView->glnWidth, pView->glnHeight);
	}

	// 进度条重启
	if (::IsWindow(pView->ProCtr.GetSafeHwnd())) {
		pView->ProCtr.ShowWindow(SW_SHOWNORMAL);
		pView->ProCtr.SetPos(0);
		pView->StartUpdateProgress();
	}

	std::thread th(&CStl3DLasetDoc::ImportStl, this, name, pyte);
	th.detach();
}
void  CStl3DLasetDoc::ImportStl(CString name, INT pyte)
{
	CString strName = name;
	if (pyte == 0)
	{
		/*if (pSTLModel_CT != NULL)
		{
			pSTLModel_CT->CloseCT();
		}*/
		pSTLModel = new CSTLModel();
		pSTLModel->LoadSTLFile(strName, pyte);
		if (pSTLModel->IsEmpty())
			delete pSTLModel;
		else
		{
			CBox3D box3D;
			//pSTLModel->MoveToCenter();
			pSTLModel->GetBox(box3D);
			m_Part.AddEntity(pSTLModel);//所有导入stl文件存储
			//double z = m_Part.GetEntity(0)->m_TriList[1]->vex->z;
			activeNum = m_Part.GetEntitySize() - 1;
			FileName = strName; newfile = true;
		}
		/*for (int i = 0; i < pSTLModel->m_TriList.GetSize() * 3; i++)
		{
			pSTLModel->m_TriList[(i / 3)]->vex[(i % 3)].z -= 6.8f;
		}*/
		COLOR3D col[3];
		for (int i = 0; i < 3; i++)
		{
			col[i].r = 0.1;
			col[i].g = 0.5;
			col[i].b = 0.2;
		}
		pSTLModel->ChangeColor(0, col[0], col[1], col[2]);//设置颜色，第一个参数为区别不同的文件
		m_stlRead = 1;
		CFigureObj* pObj;
		POSITION pos = m_objects.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = m_objects.GetNext(pos);
			Remove(pObj);
		}
		viewpointer->m_stlTriReadBool = 1;//	其实和m_stlRead有重复
		//让view知道，是否有文件已经读取
	}
	else if (pyte == 1)
	{//CString strName = dlg.GetPathName();
		pSTLModel->LoadSTLFile(strName, 1);
		/*if (pSTLModel_Scan->IsEmpty_Scan())
			delete pSTLModel_Scan;
		else
			m_Part.AddEntity(pSTLModel_Scan);*/
			//UpdateAllViews(NULL);
		COLOR3D col[3];
		for (int i = 0; i < 3; i++)
		{
			col[i].r = 0.6;
			col[i].g = 0.2;
			col[i].b = 0.1;
		}
		pSTLModel->ChangeColor(1, col[0], col[1], col[2]);//设置颜色，第一个参数为区别不同的文件
		m_stlRead = 1;
		CFigureObj* pObj;
		POSITION pos = m_objects.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = m_objects.GetNext(pos);
			Remove(pObj);
		}
		viewpointer->m_stlScanReadBool = 1;//	其实和m_stlRead有重复
	}
	////////////////
	switchflag = true;
	//::SendMessage(h_pOwner0, WM_PAINT, 0, 0);
}void CStl3DLasetDoc::SetPDFPath(CString path)
{
    m_PDFSavePath = path;
    m_wafer_params.out_dir = (LPCSTR)m_PDFSavePath.GetBuffer();
    pSTLModel_Pcd->SetReportSaveDir(m_wafer_params.out_dir);
    CString iniPath = (GetAppPath() + _T("dbset.ini")).c_str();
    ::WritePrivateProfileString(_T("PDFSet"), _T("PDFSavePath"), path, iniPath);
}
//打开pcd文件
void  CStl3DLasetDoc::ImportPcd(CString name, INT pyte)
{
	//CStl3DLasetView *pView = (CStl3DLasetView *)((CMainFrame *)AfxGetApp()->GetMainWnd())->GetActiveView();
	((CMainFrame*)AfxGetMainWnd())->SwitchView(1);
	CStl3DLasetView* pView = (CStl3DLasetView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1));
	if (pyte == 0)
	{
		/*if (!ShareData::ZScaleSetOK)
		{
			ParameSet dlg;
			dlg.DoModal();
			if (!ShareData::ZScaleSetOK)
			{
				AfxMessageBox("未完成参数设置，请重新打开");
				return;
			}
		}*/

		//if (pSTLModel != NULL)
		//{
		//	//m_Part.RemoveEntity(pSTLModel);//打开之前清空视图
		//	pSTLModel->Clear();
		//	pSTLModel = NULL;
		//	::SendMessage(h_pOwner0, WM_PCL_MESSAGE, (WPARAM)3, (LPARAM)0);
		//}
		pView->m_bRenderScene = 0;
		pView->viewClear();
		ClosePCDView();
		pSTLModel_Pcd = new CSTLModel();
		/*if (pSTLModel_CT != NULL)
		{
			pSTLModel_CT->CloseCT();
		}*/

		if (ShareData::ZScale == 0)  return;

		CString strName = name;
		PCLMSG* pclmsg = new(PCLMSG);
		CRect rect;
		pView->GetClientRect(&rect);
		//bool ret = pSTLModel_Pcd->LoadPCDFile(strName, pyte, ShareData::ZScale, rect, pView->m_hWnd, pclmsg);
		//if (!ret)
		//{
		//	ShareData::ZScaleSetOK = 0;
		//	return;//没打开pcd，退出程序
		//}
		if (name.GetLength() <= 0) {
			/*auto data = m_goManager->GetXyzData();
			if (data) {
				pSTLModel_Pcd->ShowScanData(*data, ShareData::ZScale,rect, pView->m_hWnd, pclmsg);
			}*/
		}
		else {
			bool ret = pSTLModel_Pcd->LoadPCDFile(strName, pyte, ShareData::ZScale, rect, pView->m_hWnd, pclmsg);
			if (!ret)
			{
				ShareData::ZScaleSetOK = 0;
				return;//没打开pcd，退出程序
			}
		}
		Sleep(500);
		::SendMessage(h_pOwner0, WM_PCL_MESSAGE, (WPARAM)1, (LPARAM)pclmsg);
		::SendMessage(h_pOwner0, WM_PCL_MESSAGE, (WPARAM)4, (LPARAM)0);
		/*if (pSTLModel_Pcd->IsEmpty())
			delete pSTLModel_Pcd;
		else
			m_Part.AddEntity(pSTLModel_Pcd);*/
			//UpdateAllViews(NULL);
			//pSTLModel->UpdatePCDView();//更新显示pcd
			//pSTLModel->ChangeColor(0, 0.1, 0.5, 0.2);//设置颜色，第一个参数为区别不同的文件
		m_stlRead = 1;
		CFigureObj* pObj;
		POSITION pos = m_objects.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = m_objects.GetNext(pos);
			Remove(pObj);
		}
		pView->m_stlTriReadBool = 1;//	其实和m_stlRead有重复
		//让view知道，是否有文件已经读取
		FileName = strName;//切换视图时保留pcd文件
		if (!switchflag)
			newfile = true;
		::SendMessage(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd2.GetPane(0, 0)->GetSafeHwnd(), WM_PAINT, 0, 0);
		pView->Invalidate();
	}
	else if (pyte == 1)
	{
		//pSTLModel->StlMatchPcd(name);

		pSTLModel->OpenScanPcd(name);

		pView->Invalidate();
		::SendMessage(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd2.GetPane(0, 0)->GetSafeHwnd(), WM_PAINT, 0, 0);

		m_stlRead = 1;
		CFigureObj* pObj;
		POSITION pos = m_objects.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = m_objects.GetNext(pos);
			Remove(pObj);
		}
		//pSTLModel->StlMatchPcd();
	}
}
void  CStl3DLasetDoc::ImportStep_Stp(CString name, INT pyte)
{
	CStl3DLasetView* pView = (CStl3DLasetView*)((CMainFrame*)AfxGetApp()->GetMainWnd())->GetActiveView();
	pView->viewClear();
	pView->m_bRenderScene = 1;
	//m_Part.RemoveAllEntity();//打开之前清空视图
	if (pSTLModel != NULL)
	{
		m_Part.RemoveEntity(pSTLModel);//打开之前清空视图
		pSTLModel->Clear();
		pSTLModel = NULL;
		::SendMessage(h_pOwner0, WM_PCL_MESSAGE, (WPARAM)3, (LPARAM)0);
	}
	if (pSTLModel_Pcd != NULL)
	{
		pSTLModel_Pcd->ClosePCDView();
		//m_Part.RemoveEntity(pSTLModel_Pcd);//打开之前清空视图
		::SendMessage(h_pOwner0, WM_PCL_MESSAGE, (WPARAM)2, (LPARAM)0);

	}
	/*if (pSTLModel_CT != NULL)
	{
		pSTLModel_CT->CloseCT();
	}*/
	pSTLModel = new CSTLModel();
	CString strName = name;
	pSTLModel->LoadStep_StpFile(strName, pyte);
	if (pSTLModel->IsEmpty())
		delete pSTLModel;
	else
		m_Part.AddEntity(pSTLModel);
	UpdateAllViews(NULL);
	COLOR3D col[3];
	for (int i = 0; i < 3; i++)
	{
		col[i].r = 0.1;
		col[i].g = 0.5;
		col[i].b = 0.2;
	}
	pSTLModel->ChangeColor(1, col[0], col[1], col[2]);//设置颜色，第一个参数为区别不同的文件
	m_stlRead = 1;
	CFigureObj* pObj;
	POSITION pos = m_objects.GetHeadPosition();
	while (pos != NULL)
	{
		pObj = m_objects.GetNext(pos);
		Remove(pObj);
	}
	pView->m_stlTriReadBool = 1;//	其实和m_stlRead有重复
	//让view知道，是否有文件已经读取

}
//打开excel文件，以固定行列作为输入点集，计算偏差值和面轮廓度
void CStl3DLasetDoc::ImportExcel(CString name, INT pyte)
{
	//打开指定Excel文件，如果不存在就创建
	//char path[MAX_PATH];
	//GetCurrentDirectory(MAX_PATH, (TCHAR*)path);//获取当前路径
	//CString strExcelFile = (TCHAR*)path;
	//CString strdevName = _T("\\data.xlsx");	   //xls也行
	//strExcelFile += strdevName;
	COccView* pview = ((CMainFrame*)AfxGetMainWnd())->m_OccView;
	//pview->RemoveCollectedData();
	CApplication App;//创建应用实例
	CWorkbooks Books;//工作簿，多个Excel文件
	CWorkbook Book;//单个工作簿
	CWorksheets sheets;//多个sheets页面
	CWorksheet sheet;//单个sheet页面
	CRange range;//操作单元格
	CRange iCell;
	LPDISPATCH lpDisp;
	COleVariant vResult;  //COleVariant类是对VARIANT结构的封装
	COleVariant covOptional((long)DISP_E_PARAMNOTFOUND, VT_ERROR);

	if (!App.CreateDispatch(_T("Excel.Application"), NULL))
	{
		AfxMessageBox(_T("无法启动Excel服务器!"));
		return;
	}

	Books.AttachDispatch(App.get_Workbooks());
	lpDisp = Books.Open(name, covOptional, covOptional,
		covOptional, covOptional, covOptional, covOptional, covOptional,
		covOptional, covOptional, covOptional, covOptional, covOptional,
		covOptional, covOptional);

	//得到Workbook    
	Book.AttachDispatch(lpDisp);
	//得到Worksheets   
	sheets.AttachDispatch(Book.get_Worksheets());
	//得到当前活跃sheet 
	//如果有单元格正处于编辑状态中，此操作不能返回，会一直等待 
	lpDisp = Book.get_ActiveSheet();
	sheet.AttachDispatch(lpDisp);

	//获取行列号
	range = sheet.get_UsedRange();
	range = range.get_Rows();
	long rows = range.get_Count();
	range = range.get_Columns();
	long columns = range.get_Count();
	long aim_col = 1, startRow = 1, endRow = rows;
	bool flag = true;
	for (long i = 1; i < rows + 1 && flag; i++)
	{
		for (long j = 1; j < columns + 1; j++)
		{
			range.AttachDispatch(sheet.get_Cells());
			range.AttachDispatch(range.get_Item(COleVariant(i), COleVariant(j)).pdispVal);   //第一变量是行，第二个变量是列，即第i行第j列
			vResult = range.get_Value2();
			if (vResult.vt == VT_BSTR)
			{
				CString str; str = vResult.bstrVal;
				if (str == "名义值")
				{
					aim_col = j; startRow = i;
					flag = false; continue;//break;
				}
				if (str == "平面度")
				{
					endRow = i; break;
				}
			}
		}
	}
	vector<gp_Pnt> position;
	for (long i = startRow; i < endRow - 1; i += 3)
	{
		gp_Pnt point;
		range.AttachDispatch(sheet.get_Cells());
		range.AttachDispatch(range.get_Item(COleVariant(i + 1), COleVariant(aim_col)).pdispVal);   //第一变量是行，第二个变量是列，即第i行第j列
		vResult = range.get_Value2();
		if (vResult.vt == VT_R8)
		{
			point.SetX(vResult.dblVal);
		}
		range.AttachDispatch(sheet.get_Cells());
		range.AttachDispatch(range.get_Item(COleVariant(i + 2), COleVariant(aim_col)).pdispVal);   //第一变量是行，第二个变量是列，即第i行第j列
		vResult = range.get_Value2();
		if (vResult.vt == VT_R8)
		{
			point.SetY(vResult.dblVal);
		}
		range.AttachDispatch(sheet.get_Cells());
		range.AttachDispatch(range.get_Item(COleVariant(i + 3), COleVariant(aim_col)).pdispVal);   //第一变量是行，第二个变量是列，即第i行第j列
		vResult = range.get_Value2();
		if (vResult.vt == VT_R8)
		{
			point.SetZ(vResult.dblVal);
		}
		position.push_back(point);
	}
	pview->SetSphereData(position);
	pview->CompareCollecting();
	((CMainFrame*)AfxGetMainWnd())->m_FileMenu->m_btnCompare.EnableWindow(false);
	Books.Close();
	App.Quit();
	//释放对象      
	range.ReleaseDispatch();
	sheet.ReleaseDispatch();
	sheets.ReleaseDispatch();
	Book.ReleaseDispatch();
	Books.ReleaseDispatch();
	App.ReleaseDispatch();

}
//载入txt点集作为偏差值计算输入，区分stl和occ两种视图情况
void  CStl3DLasetDoc::ImportTxt(CString name, INT pyte)
{
	/////////////////////////////////
	//pclmsg = new(PCLMSG);
	//CRect rect;
	//pView->GetClientRect(&rect);
	//if (pyte == 1)
	//{
	//	pSTLModel_Pcd->OpenScanTxt(/*Dis, pts,*/ name, pyte, ShareData::ZScale, rect, pView->m_hWnd, pclmsg);
	//}
	///*if (pSTLModel_Pcd->IsEmpty())
	//	delete pSTLModel_Pcd;
	//else*/
	//	m_Part.AddEntity(pSTLModel_Pcd);
	//	activenum = m_Part.GetEntitySize() - 1;
	//	pView->m_bRenderScene = 1;
	///////////////////////////////////
	if (((CMainFrame*)AfxGetMainWnd())->GetActiveView() == StlView)
		ImportSTLTxt(name, pyte);
	else
		ImportSTPTxt(name, pyte);
	OpenFileView* pview = ((CMainFrame*)AfxGetMainWnd())->m_FileMenu;
	pview->m_btnCancelCollecting.EnableWindow(TRUE);
}

void CStl3DLasetDoc::ImportSTPTxt(CString name, INT pyte)
{
	Sleep(100);
	CString path = (GetAppPath()).c_str();
	CString num;
	num.Format(_T("\\png\\objectSnap.png"));
	path += num;
	((CMainFrame*)AfxGetMainWnd())->m_OccView->CaptureScreenPng(path);
	FILE* file = fopen(name, "r");
	glm::vec3 vert;
	vector<gp_Pnt> position; //vector<Handle(AIS_Shape)> Sphere;
	BRep_Builder builder; TopoDS_Compound compound; builder.MakeCompound(compound);
	if (file != NULL)
	{
		fscanf(file, "%*s%*s%*s");
		while (fscanf(file, "%f %f %f", &vert.x, &vert.y, &vert.z) == 3)
		{
			gp_Pnt pos = { vert.x,vert.y,vert.z };
			position.push_back(pos);
		}
	}
	CString note; note.Format("即将导入%d个数据点", position.size());
	if (MessageBox(NULL, note, "提示", MB_OKCANCEL) == IDOK)
	{
		((CMainFrame*)AfxGetMainWnd())->m_OccView->RemoveCollectedData();
		((CMainFrame*)AfxGetMainWnd())->m_OccView->SetSphereData(position);
		//((OpenFileView*)((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd.GetPane(0, 0))->m_btnCompare.EnableWindow(true);
		((CMainFrame*)AfxGetMainWnd())->m_FileMenu->m_btnCompare.EnableWindow(false);
	}
	fclose(file);
}

void CStl3DLasetDoc::ImportSTLTxt(CString name, INT pyte)
{
	CStl3DLasetView* pView = (CStl3DLasetView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1));
	FILE* file = fopen(name, "r");
	glm::vec3 vert; m_Scanvertex.clear();
	if (file != NULL)
	{
		fscanf(file, "%*s%*s%*s");
		while (fscanf(file, "%f %f %f", &vert.x, &vert.y, &vert.z) == 3)
		{
			m_Scanvertex.emplace_back(vert.x); m_Scanvertex.emplace_back(vert.y); m_Scanvertex.emplace_back(vert.z);
		}
	}
	CString note; note.Format("即将导入%d个数据点", m_Scanvertex.size() / 3);
	if (MessageBox(NULL, note, "提示", MB_OKCANCEL) == IDOK)
	{
		pView->SetUpdate(DrawType::sphere);
	}
	else
		m_Scanvertex.clear();
	fclose(file);
}

void  CStl3DLasetDoc::ImportCT(CString name, INT pyte)
{

	CStl3DLasetView* pView = (CStl3DLasetView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1));
	CRect rect;
	pView->GetClientRect(&rect);
	pView->m_bRenderScene = 0;
	pView->viewClear();
	if (pyte == 0)
	{
		if (pSTLModel != NULL)
		{
			m_Part.RemoveEntity(pSTLModel);//打开之前清空视图
			pSTLModel->Clear();
			pSTLModel = NULL;
			::SendMessage(h_pOwner0, WM_PCL_MESSAGE, (WPARAM)3, (LPARAM)0);
		}
		if (pSTLModel_Pcd != NULL)
		{
			pSTLModel_Pcd->ClosePCDView();
			//m_Part.RemoveEntity(pSTLModel_Pcd);//打开之前清空视图
			::SendMessage(h_pOwner0, WM_PCL_MESSAGE, (WPARAM)2, (LPARAM)0);
			/*pSTLModel_Pcd->Clear();*/
			//pSTLModel_Pcd = NULL;
		}
	/*	pSTLModel_CT->LoadCTFile(name, rect, pView->m_hWnd);*/
		FileName = name;//切换视图时保留pcd文件
		//if (!switchflag)
		newfile = true;
		UpdateAllViews(NULL);
	}

}

//外接api打开文件
void CStl3DLasetDoc::Load3DData(LPCTSTR fullpath)
{
    if (!fullpath || *fullpath == 0) return;
    CString path(fullpath);
    CString ext = path.Mid(path.ReverseFind('.') + 1);
    ext.MakeLower();

    if (ext == _T("stl")) {
        Import_Stl(fullpath, 0);
    }
    else if (ext == _T("pcd")) {
        ImportPcd(fullpath, 0);
    }
    else if (ext == _T("step") || ext == _T("stp")) {
        ImportSTEP(fullpath, 1);
    }
    else if (ext == _T("igs") || ext == _T("iges")) {
        ImportIGS(fullpath, 1);
    }
    else if (ext == _T("txt")) {
        // ImportTxt(fullpath, 1);
    }
    else if (ext == _T("csv")) {
        // ImportCsv(fullpath, 1);
    }
    else if (ext == _T("tif")) {
        // ImportTif(fullpath, 1);
    }
    else if (ext == _T("xlsx") || ext == _T("xls")) {
        ImportExcel(fullpath, 1);
    }
    else {
        AfxMessageBox(_T("不支持的文件类型：") + ext);
        return;
    }

    SyncFileToTreeAfterLoad(fullpath);

    UpdateAllViews(NULL);
}

//打开3d文件
void CStl3DLasetDoc::OnFile3Ddata()
{
	CFileDialog dlg(TRUE, NULL, NULL, NULL, __T("所有文件(*.*)|*.*|File(*.stl)|*.STL|PCD(*.pcd)|*.PCD|STEP(*.step)|*.STEP|STP(*.stp)|*.STP|3D(*.TXT)|*.txt|3ds(*.CSV)|*.csv|image(*.tif)|*.TIF|Excel File(*.xlsx)|*.xls|"));
	CString strName, Extname;
	CString extfilename[30] = { _T("stl"),_T("STL"),_T("pcd"),_T("PCD"),_T("step"),_T("STEP"),_T("stp"),_T("STP"),_T("igs"),_T("IGS"),_T("iges"),_T("IGES"),_T("txt"),_T("TXT"),_T("csv"),_T("CSV"),_T("tif"),_T("TIF"),_T("xlsx"),_T("xls") };
	if (dlg.DoModal() == IDOK)
	{
		strName = dlg.GetPathName();
		Extname = dlg.GetFileExt();
		int i = 0;
		for (i = 0; i < 20; i++)
		{
			if (Extname == extfilename[i])
			{
				break;
			}
		}
		switch (i)
		{
		case 0:
		case 1:
			//((CMainFrame*)AfxGetMainWnd())->SwitchView(1);
			Import_Stl(strName, 0);
			break;
		case 2:
		case 3:
			//((CMainFrame*)AfxGetMainWnd())->SwitchView(1);
			ImportPcd(strName, 0);
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			//((CMainFrame*)AfxGetMainWnd())->SwitchView(0); 
			//ClearAISContext();
			ImportSTEP(strName, 1);
			break;
		case 8:
		case 9:
		case 10:
		case 11:
			//((CMainFrame*)AfxGetMainWnd())->SwitchView(0);
			//ClearAISContext();
			ImportIGS(strName, 1);
			break;
		case 12:
		case 13:
			//ImportTxt(strName, 1);
			break;
		case 18:
		case 19:
			ImportExcel(strName, 1);
			break;
		default:
			break;
		}
		// 加载成功后，同步到树控
		SyncFileToTreeAfterLoad(strName);

		// 通知视图更新（仅用于刷新显示，不处理同步）
		UpdateAllViews(NULL);
	}

}
// 新增：同步文件到树控的函数
void CStl3DLasetDoc::SyncFileToTreeAfterLoad(const CString& filePath)
{
	// 获取文件视图（FileView）实例
	CMainFrame* pMainFrame = (CMainFrame*)AfxGetMainWnd();
	FileView* pFileView = (FileView*)pMainFrame->m_wndSplitterWnd2.GetPane(0, 0);
	if (!pFileView) return;
	// 获取文件索引（根据当前文件列表长度计算）
	int index = pFileView->m_List.GetItemCount();
	// 提取文件名（不含路径）
	CString fileName = filePath.Right(filePath.GetLength() - filePath.ReverseFind('\\') - 1);

	// 添加到FileView的列表控件（即使视图隐藏，列表数据仍需更新）
	pFileView->m_List.InsertItem(index, fileName);
	pFileView->fileName[index] = CStringA(fileName).GetString(); // 更新FileView的文件名存储

	// 同步到树控（调用原SyncFileToTree函数）
	pFileView->SyncFileToTree(index, CStringA(filePath).GetString());

	// 更新FileView的选中状态
	if (index > 0)
		pFileView->m_List.SetItemState(index - 1, 0, LVIS_SELECTED);
	pFileView->m_List.SetItemState(index, LVIS_SELECTED, LVIS_SELECTED);
	pFileView->olditem = index;
}
void CStl3DLasetDoc::OnLoadScan()
{
	// TODO: 在此添加命令处理程序代码
	CFileDialog dlg(TRUE, NULL, NULL, NULL, __T("所有文件(*.*)|*.*|File(*.stl)|*.STL|PCD(*.pcd)|*.PCD|STEP(*.step)|*.STEP|STP(*.stp)|*.STP|3D(*.TXT)|*.txt|3ds(*.CSV)|*.csv|image(*.tif)|*.TIF|"));
	CString strName, Extname;
	CString extfilename[20] = { _T("stl"),_T("STL"),_T("pcd"),_T("PCD"),_T("step"),_T("STEP"),_T("stp"),_T("STP"),_T("igs"),_T("IGS"),_T("iges"),_T("IGES"),_T("txt"),_T("TXT"),_T("csv"),_T("CSV"),_T("tif"),_T("TIF") };
	if (dlg.DoModal() == IDOK)
	{
		strName = dlg.GetPathName();
		Extname = dlg.GetFileExt();
		int i = 0;
		for (i = 0; i < 10; i++)
		{
			if (Extname == extfilename[i])
			{
				break;
			}
		}
		switch (i)
		{
		case 0:
		case 1:
			((CMainFrame*)AfxGetMainWnd())->SwitchView(1);
			Import_Stl(strName, 1);
			break;
		case 2:
		case 3:
			((CMainFrame*)AfxGetMainWnd())->SwitchView(1);
			ImportPcd(strName, 1);
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			//((CMainFrame*)AfxGetMainWnd())->SwitchView(0);
			//ClearAISContext();
			ImportSTEP(strName, 1);
			break;
		case 8:
		case 9:
		case 10:
		case 11:
			//((CMainFrame*)AfxGetMainWnd())->SwitchView(0);
			//ClearAISContext();
			ImportIGS(strName, 1);
			break;
		case 12:
		case 13:
			ImportTxt(strName, 1);
			break;
		}
	}

}
void CStl3DLasetDoc::OnLoadProbe()
{
	// TODO: 在此添加命令处理程序代码
	CFileDialog dlg(TRUE, "stl", NULL,
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		"Stereo Lithograpic File(*.stl)|*.stl||", NULL);
	CStl3DLasetView* pView = (CStl3DLasetView*)((CMainFrame*)AfxGetApp()->GetMainWnd())->GetActiveView();
	//pView->clearAll();
	if (dlg.DoModal() == IDOK)
	{
		pView->viewClear();
		if (pSTLModel_Probe != NULL)
		{
			m_Part.RemoveEntity(pSTLModel_Probe);//打开之前清空视图
		}
		pSTLModel_Probe = new CSTLModel();
		CString strName = dlg.GetPathName();
		pSTLModel_Probe->LoadSTLFile(strName, 2);
		if (pSTLModel_Probe->IsEmpty_Probe())
			delete pSTLModel_Probe;
		else
			m_Part.AddEntity(pSTLModel_Probe);
		UpdateAllViews(NULL);
		COLOR3D col[3];
		for (int i = 0; i < 3; i++)
		{
			col[i].r = 0.6;
			col[i].g = 0.2;
			col[i].b = 0.5;
		}
		pSTLModel_Probe->ChangeColor(1, col[0], col[1], col[2]);//设置颜色，第一个参数为区别不同的文件
		m_stlRead = 1;
		CFigureObj* pObj;
		POSITION pos = m_objects.GetHeadPosition();
		while (pos != NULL)
		{
			pObj = m_objects.GetNext(pos);
			Remove(pObj);
		}
		pView->m_stlProbeReadBool = 1;//	其实和m_stlRead有重复
		//让view知道，是否有文件已经读取
	}
}

void CStl3DLasetDoc::OnOpenData()
{
	CFileDialog dlg(TRUE, NULL, NULL, NULL, __T("所有文件(*.*)|*.*|File(*.stl)|*.STL|PCD(*.pcd)|*.PCD|STEP(*.step)|*.STEP|STP(*.stp)|*.STP|3D(*.TXT)|*.txt|3ds(*.CSV)|*.csv|image(*.tif)|*.TIF|"));
	CString strName, Extname;
	CString extfilename[20] = { _T("stl"),_T("STL"),_T("pcd"),_T("PCD"),_T("step"),_T("STEP"),_T("stp"),_T("STP"),_T("txt"),_T("TXT"),_T("csv"),_T("CSV"),_T("tif"),_T("TIF") };
	h_pOwner0 = ((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1)->GetSafeHwnd();
	if (dlg.DoModal() == IDOK)
	{
		strName = dlg.GetPathName();
		Extname = dlg.GetFileExt();
		int i = 0;
		for (i = 0; i < 10; i++)
		{
			if (Extname == extfilename[i])
			{
				break;
			}
		}
		switch (i)
		{
		case 0:Import_Stl(strName, 1); break;
		case 1:Import_Stl(strName, 1); break;
		case 2:ImportPcd(strName, 1); break;
		case 3:ImportPcd(strName, 1); break;
		case 4:ImportStep_Stp(strName, 1); break;
		case 5:ImportStep_Stp(strName, 1); break;
		case 6:ImportStep_Stp(strName, 1); break;
		case 7:ImportStep_Stp(strName, 1); break;
		case 8:ImportTxt(strName, 0); break;
		case 9:ImportTxt(strName, 0); break;
		case 10:break;
		case 11:break;
		case 12:break;
		case 13:break;
		}
	}
}
//参数设置
void CStl3DLasetDoc::OnParameSet()
{
	// TODO: 在此添加命令处理程序代码
	//ParameSet dlg;
	//dlg.DoModal();
}


void CStl3DLasetDoc::OnMenuSensorConf()
{
	// TODO: 在此添加命令处理程序代码
	//GocatorDialogConf goConf;
	//goConf.DoModal();
}


void CStl3DLasetDoc::OnMenuShowSensorData()
{
	// TODO: 在此添加命令处理程序代码
	ImportPcd("", 0);
}

//切换显示文件为pcd
void CStl3DLasetDoc::SwitchToPCD(string name)
{
	// TODO: 在此处添加实现代码.
	((CMainFrame*)AfxGetMainWnd())->SwitchView(1);
	switchflag = true;
	ImportPcd(name.c_str(), 0);
	switchflag = false;
}

void CStl3DLasetDoc::SwitchToCT(string name)
{
	// TODO: 在此处添加实现代码.
	switchflag = true;
	ImportCT(name.c_str(), 0);
	UpdateWindow(h_pOwner0);
}

//切换显示文件为stl
void CStl3DLasetDoc::SwitchToSTL(int stlnum)
{
	// TODO: 在此处添加实现代码.
	((CMainFrame*)AfxGetMainWnd())->SwitchView(1);
	CStl3DLasetView* pView = (CStl3DLasetView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1));
	pView->viewClear();
	activeNum = stlnum; switchflag = true; pView->m_bRenderScene = 1;
	pView->m_pDC->m_Camera.init(); pView->m_pDC->m_Camera.set_screen(pView->glnWidth, pView->glnHeight);
	pView->Invalidate(); ClosePCDView();
}
//切换显示文件为occ
void CStl3DLasetDoc::SwitchToOcc(int index)
{
	((CMainFrame*)AfxGetMainWnd())->SwitchView(0);
	((CMainFrame*)AfxGetMainWnd())->m_OccView->SwitchAISContext(index);
	//activeContext = index;
}

//保存文件
void CStl3DLasetDoc::OnFileSave()
{
    // TODO: 在此添加命令处理程序代码
    FileView* pview = (FileView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd2.GetPane(0, 0));

    // 添加判断逻辑
    if (!pview) {
        AfxMessageBox(_T("无法访问文件视图！"));
        return;
    }

    if (pview->olditem < 0) {
        AfxMessageBox(_T("请先选择要保存的文件！"));
        return;
    }

    // 检查当前是否有模型可保存
    if (((CMainFrame*)AfxGetMainWnd())->GetActiveView() == ActiveView::OccView) {
        COccView* pOccView = ((CMainFrame*)AfxGetMainWnd())->m_OccView;
        if (pOccView->isEmpty()) {
            AfxMessageBox(_T("当前没有可保存的模型！"));
            return;
        }
    }
    else if (((CMainFrame*)AfxGetMainWnd())->GetActiveView() == ActiveView::StlView) {
        if (m_Part.GetEntitySize() == 0) {
            AfxMessageBox(_T("当前没有可保存的模型！"));
            return;
        }
    }

    // 根据文件类型执行保存
    switch (pview->GetFileType(pview->olditem))
    {
    case OpenFileType::STL:
        SaveSTL();
        break;
    case OpenFileType::STP:
        SaveSTEP();
        break;
    case OpenFileType::IGS:
        SaveIGS();
        break;
    default:
        AfxMessageBox(_T("当前文件类型不支持保存！"));
        break;
    }
}

void CStl3DLasetDoc::OnCpd2Stl()
{
	// TODO: 在此添加命令处理程序代码
	//pSTLModel_Pcd->LineScanPcd2Stl(12);

	pSTLModel_Pcd->PCDRender(ShareData::tolerance, pclmsg);
	pSTLModel_Pcd->ClosePCDView();
	FileName = "\\render";//切换视图时保留pcd文件
	//if(!switchflag)
	newfile = true;
	UpdateWindow(h_pOwner0);
	::SendMessage(h_pOwner0, WM_PCL_MESSAGE, (WPARAM)4, (LPARAM)0);
}
//关闭pcd文件并清除视图
void CStl3DLasetDoc::ClosePCDView()
{
	if (pSTLModel_Pcd != NULL)
	{
		pSTLModel_Pcd->ClosePCDView();
		//pSTLModel_Pcd->Clear();
		::SendMessage(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1)->GetSafeHwnd(), WM_PCL_MESSAGE, (WPARAM)3, (LPARAM)0);
	}
}

void CStl3DLasetDoc::SaveSTL()
{
	CString szFilter = "所有文件(*.*)|*.*|File(*.stl)|*.STL||";
	CString fileName;
	CFileDialog dlg(FALSE, _T("stl"), "test", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, NULL);//dlg.m_ofn.lpstrDefExt = "txt";
	FileView* pview = (FileView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd2.GetPane(0, 0));
	CString path = pview->fileName[activeNum].c_str();
	if (dlg.DoModal() == IDOK)
	{
		fileName = dlg.GetPathName();
		//CFile file;
		//FILE* fp = fopen(path, "wb");
		CFile Files;
		Files.Open(path, CFile::modeRead);
		char str[80]{};
		BYTE Size[4];
		BYTE buffer[50];
		Files.Read(str, 80);
		Files.Read(Size, 4);
		Files.Close();
		int Count = m_Part.GetEntity(activeNum)->m_TriList.GetSize();
		BYTE* b = new BYTE[4];
		b[3] = (BYTE)((Count & 0xFF000000) >> 24);
		b[2] = (BYTE)((Count & 0x00FF0000) >> 16);
		b[1] = (BYTE)((Count & 0x0000FF00) >> 8);
		b[0] = (BYTE)((Count & 0x000000FF));
		/*if (!Files.Open(fileName, CFile::modeCreate | CFile::modeReadWrite))
			return;*/
		FILE* file = fopen(fileName, "wb");
		float* dat = new float[12];
		/*Files.Write(str, strlen(str));
		Files.Write(&Count, sizeof(int));*/
		fwrite(str, sizeof(char), 80, file);
		fwrite(b, sizeof(BYTE), 4, file);
		delete[]b;
		for (int i = 0; i < m_Part.GetEntity(activeNum)->m_TriList.GetSize(); i++)
		{
			CPoint3D p = m_Part.GetEntity(activeNum)->m_TriList[i]->vex[0];
			float v1x = p.x;
			float v1y = p.y;
			float v1z = p.z;

			p = m_Part.GetEntity(activeNum)->m_TriList[i]->vex[1];
			float v2x = p.x;
			float v2y = p.y;
			float v2z = p.z;

			p = m_Part.GetEntity(activeNum)->m_TriList[i]->vex[2];
			float v3x = p.x;
			float v3y = p.y;
			float v3z = p.z;

			float nx = (v1y - v3y) * (v2z - v3z) - (v1z - v3z) * (v2y - v3y);
			float ny = (v1z - v3z) * (v2x - v3x) - (v2z - v3z) * (v1x - v3x);
			float nz = (v1x - v3x) * (v2y - v3y) - (v2x - v3x) * (v1y - v3y);

			float nxyz = sqrt(nx * nx + ny * ny + nz * nz);

			dat[0] = nx / nxyz;
			dat[1] = ny / nxyz;
			dat[2] = nz / nxyz;

			dat[3] = v1x;
			dat[4] = v1y;
			dat[5] = v1z;

			dat[6] = v2x;
			dat[7] = v2y;
			dat[8] = v2z;

			dat[9] = v3x;
			dat[10] = v3y;
			dat[11] = v3z;

			fwrite(dat, sizeof(float), 12, file);
			fwrite("wl", sizeof(char), 2, file);

		}
		fclose(file);
		delete[]dat;
	}
	// 保存完成后：设置当前模型为不透明
	COccView* pOccView = (COccView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1));
	if (pOccView)
	{
		int activeShapeIndex = 0; // 假设当前只显示一个形状，索引为0
		pOccView->SetTransparency(activeShapeIndex, 0.0); // 0.0=完全不透明
	}
}
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <TDocStd_Document.hxx>
#include <TDataStd_Name.hxx>
void CStl3DLasetDoc::SaveSTEP()
{
	FileView* pview = (FileView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd2.GetPane(0, 0));
	CString szFilter = "STEP文件(*.stp)|*.STP||";
	CFileDialog dlg(FALSE, _T("stp"), "exported_model", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter);

	if (dlg.DoModal() != IDOK)
		return;

	CString fileName = dlg.GetPathName();

	COccView* pView = (COccView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1));
	if (!pView || pView->isEmpty())
	{
		AfxMessageBox(_T("当前没有模型可保存"));
		return;
	}

	// 初始化 XCAF
	Handle(TDocStd_Document) xdoc;
	Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
	app->NewDocument("MDTV-XCAF", xdoc);

	Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(xdoc->Main());
	Handle(XCAFDoc_ColorTool) colorTool = XCAFDoc_DocumentTool::ColorTool(xdoc->Main());

	// 1. 添加主模型
	TopoDS_Shape mainShape = pView->GetActiveShape();
	if (!mainShape.IsNull())
	{
		TDF_Label mainLabel = shapeTool->AddShape(mainShape);
		TDataStd_Name::Set(mainLabel, "MainShape");
	}


	if (pView->HasTrihedron())
	{
		// 获取坐标系几何
		TopoDS_Shape trihedronShape = pView->GetTrihedronAsShape();
		if (!trihedronShape.IsNull())
		{
			TDF_Label triLabel = shapeTool->AddShape(trihedronShape);
			TDataStd_Name::Set(triLabel, "Trihedron");

			// 获取坐标系颜色
			Quantity_Color xCol, yCol, zCol;
			// 新代码：直接从缓存“读取”颜色

			// 为每个轴线添加子形状并设置颜色
			TDF_LabelSequence subLabels;
			shapeTool->GetSubShapes(triLabel, subLabels);

			for (Standard_Integer i = 1; i <= subLabels.Length(); ++i)
			{
				TDF_Label subLabel = subLabels.Value(i);
				Handle(TDataStd_Name) subNameAttr;
				if (!subLabel.FindAttribute(TDataStd_Name::GetID(), subNameAttr)) continue;

				std::string subName = TCollection_AsciiString(subNameAttr->Get()).ToCString();

				if (subName == "XAxis")
					colorTool->SetColor(subLabel, xCol, XCAFDoc_ColorGen);
				else if (subName == "YAxis")
					colorTool->SetColor(subLabel, yCol, XCAFDoc_ColorGen);
				else if (subName == "ZAxis")
					colorTool->SetColor(subLabel, zCol, XCAFDoc_ColorGen);
			}
		}
	}
	// 4. 写入 STEP 文件（带颜色和结构）
	STEPCAFControl_Writer writer;
	writer.Transfer(xdoc, STEPControl_AsIs);

	IFSelect_ReturnStatus status = writer.Write((Standard_CString)(LPCTSTR)fileName);
	if (status != IFSelect_RetDone)
	{
		AfxMessageBox(_T("保存失败"));
	}
	else
	{
		AfxMessageBox(_T("STEP文件保存成功（含颜色信息）"));
	}
    if (g_3dFileCallback) {
        CStringA fileNameA(fileName);
        g_3dFileCallback(D_index, fileNameA.GetBuffer());
        fileNameA.ReleaseBuffer();
    }
	// 清理
	xdoc->Close();
	pView->SetTransparency(0, 0.0);

}

void CStl3DLasetDoc::SaveIGS()
{
	FileView* pview = (FileView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd2.GetPane(0, 0));
	CString szFilter = "所有文件(*.*)|*.*|File(*.igs)|*.IGS||";
	CString fileName;
	CFileDialog dlg(FALSE, _T("igs"), "test", OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, szFilter, NULL);//dlg.m_ofn.lpstrDefExt = "txt";
	CString path = pview->fileName[pview->olditem].c_str();
	if (dlg.DoModal() == IDOK)
	{
		fileName = dlg.GetPathName();
		COccView* pView = (COccView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1));
		TopoDS_Shape shape = pView->GetActiveShape();
		IGESControl_Writer writer;
		writer.AddShape(shape);

		// 保存为IGES文件
		if (writer.Write(fileName)) {
			std::cout << "IGES file saved successfully." << std::endl;
		}
		else {
			AfxMessageBox(_T("保存失败"));
		}
		COccView* pOccView = (COccView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1));
		if (pOccView)
		{
			int activeShapeIndex = 0; // 假设当前只显示一个形状，索引为0
			pOccView->SetTransparency(activeShapeIndex, 0.0); // 0.0=完全不透明
		}
	}
}
//读取STEP文件
void CStl3DLasetDoc::ReadSTEP()
{
	STEPControl_Reader aReader_Step;
	aReader_Step.ReadFile("./Document/dd1-gz2-27.stp");
	//检查文件加载状态
	aReader_Step.PrintCheckLoad(Standard_False, IFSelect_ItemsByEntity);
	//加载step文件
	// 获取可转移根的数量
	Standard_Integer NbRoots = aReader_Step.NbRootsForTransfer();
	//翻译所有可转换的根，并返回成功翻译的次数
	Standard_Integer num = aReader_Step.TransferRoots();
	//读取到TopoDS_Shape结构中
	TopoDS_Shape aShape = aReader_Step.OneShape();
	Handle(AIS_Shape) myAISSphere = new AIS_Shape(aShape);
	myAISContext->Display(myAISSphere, Standard_False);
	//myAISSphere->SetColor(Quantity_NOC_CYAN);//设置模型颜色
	COccView* pView = (COccView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1));
	//pView->selectMode(myAISSphere);
	//设置边界线条
	myAISContext->DefaultDrawer()->SetFaceBoundaryDraw(true);
	myAISContext->DefaultDrawer()->SetFaceBoundaryAspect(
		new Prs3d_LineAspect(Quantity_NOC_CYAN, Aspect_TOL_SOLID, 1.0));
	myAISContext->DefaultDrawer()->SetIsoOnTriangulation(true);    //（显示线框）
}

//读取igs
void CStl3DLasetDoc::ReadIGS()
{
	IGESControl_Reader myIgesReader;
	Standard_Integer nIgesFaces, nTransFaces;
	//myIgesReader.ReadFile("../x64/debug/biaozhun3d.igs");
	myIgesReader.ReadFile("./Document/biaozhun3d.igs");

	Handle(TColStd_HSequenceOfTransient) myList = myIgesReader.GiveList("iges-faces");
	//selects all IGES faces in the file and puts them into a list called //MyList,
	nIgesFaces = myList->Length();
	nTransFaces = myIgesReader.TransferList(myList);
	//translates MyList,
	/*cout << “IGES Faces : " << nIgesFaces << " Transferred : ” << nTransFaces << endl;*/
	TopoDS_Shape aShape = myIgesReader.OneShape();
	Handle(AIS_Shape) anAisModel = new AIS_Shape(aShape);
	anAisModel->SetColor(Quantity_NOC_BLUE1);
	anAisModel->SetTransparency(0.9);
	myAISContext->Display(anAisModel, Standard_False);
	COccView* pView = (COccView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1));
	pView->selectMode(anAisModel);
	//设置边界线条
	myAISContext->DefaultDrawer()->SetFaceBoundaryDraw(true);
	myAISContext->DefaultDrawer()->SetFaceBoundaryAspect(
		new Prs3d_LineAspect(Quantity_NOC_CYAN, Aspect_TOL_SOLID, 1.0));
	myAISContext->DefaultDrawer()->SetIsoOnTriangulation(true);    //（显示线框）
}
//打开IGS文件
void CStl3DLasetDoc::ImportIGS(CString name, INT pyte)
{
	((CMainFrame*)AfxGetMainWnd())->SwitchView(0);
	COccView* pView = ((CMainFrame*)AfxGetMainWnd())->m_OccView;
	SelectedFeature* pList = ((CMainFrame*)AfxGetMainWnd())->m_FeatureView;
	IGESControl_Reader myIgesReader;
	Standard_Integer nIgesFaces, nTransFaces;
	myIgesReader.ReadFile(name);
	Handle(TColStd_HSequenceOfTransient) myList = myIgesReader.GiveList("iges-faces");
	//selects all IGES faces in the file and puts them into a list called //MyList,
	nIgesFaces = myList->Length();
	nTransFaces = myIgesReader.TransferList(myList);
	//translates MyList,
	/*cout << “IGES Faces : " << nIgesFaces << " Transferred : ” << nTransFaces << endl;*/
	TopoDS_Shape aShape = myIgesReader.OneShape();
	Handle(AIS_Shape) anAisModel = new AIS_Shape(aShape);
	anAisModel->SetColor(Quantity_NOC_BLUE1);
	anAisModel->SetTransparency(0.9);
	Handle(AIS_InteractiveContext) newContext;
	newContext = new AIS_InteractiveContext(myViewer);  // 创建一个交互文档
	newContext->SetDisplayMode(AIS_Shaded, true);     //设置显示模式为实体
	pList->ClearList();
	pView->SetAISContext(newContext);
	//newContext->Display(anAisModel, Standard_False);
	//TopoDS_Compound compound; BRep_Builder builder; builder.MakeCompound(compound);
	//TopoDS_Shape shape1, shape2;
	//gp_Pnt center1 = { 2.0,0.0,2.0 }; gp_Pnt center2 = { -2.0,0.0,2.0 };
	//shape1 = BRepPrimAPI_MakeSphere(center1, 0.3); shape2 = BRepPrimAPI_MakeSphere(center2, 0.3);
	////builder.Add(compound,shape1); builder.Add(compound, shape2);
	////Handle(AIS_Shape) anSphere = new AIS_Shape(compound);
	//Handle(AIS_Shape) anSphere1 = new AIS_Shape(shape1); Handle(AIS_Shape) anSphere2 = new AIS_Shape(shape2);
	//newContext->Display(anSphere1, Standard_True); newContext->Display(anSphere2, Standard_True);
	//obj.push_back(anSphere1); obj.push_back(anSphere2);
	//anSphere->SetColor(Quantity_NameOfColor::Quantity_NOC_RED);
	myIgesReader.ClearShapes();
	pView->Add(anAisModel);
	pView->FitAll();
	//刷新列表
	if (!switchflag)
		newfile = true;
	FileName = name;
	::SendMessage(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd2.GetPane(0, 0)->GetSafeHwnd(), WM_PAINT, 0, 0);
}
//打开step文件
#include <STEPCAFControl_Reader.hxx>
void CStl3DLasetDoc::ImportSTEP(CString name, INT pyte)
{
	((CMainFrame*)AfxGetMainWnd())->SwitchView(0);
	COccView* pView = ((CMainFrame*)AfxGetMainWnd())->m_OccView;
	SelectedFeature* pList = ((CMainFrame*)AfxGetMainWnd())->m_FeatureView;
	m_userModel.Nullify();
	m_autoModel.Nullify();

	/* 1. 用 XCAF 读取 STEP */
	Handle(TDocStd_Document) xdoc;
	Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
	app->NewDocument("MDTV-XCAF", xdoc);

	STEPCAFControl_Reader reader;
	IFSelect_ReturnStatus status = reader.ReadFile((Standard_CString)(LPCTSTR)name);
	if (status != IFSelect_RetDone) return;
	reader.Transfer(xdoc);

	Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(xdoc->Main());
	Handle(XCAFDoc_ColorTool) colorTool = XCAFDoc_DocumentTool::ColorTool(xdoc->Main());

	/* 2. 主模型 */
	TDF_LabelSequence freeShapes;
	shapeTool->GetFreeShapes(freeShapes);
	if (freeShapes.IsEmpty()) return;

	TopoDS_Shape aShape = shapeTool->GetShape(freeShapes.Value(1));
	if (aShape.IsNull()) return;

	Handle(AIS_Shape) myAISShape = new AIS_Shape(aShape);
	Handle(AIS_InteractiveContext) newCtx = new AIS_InteractiveContext(myViewer);
	newCtx->SetDisplayMode(AIS_Shaded, Standard_True);
	if (!pView->SetAISContext(newCtx)) return;

	m_userModel = myAISShape;

	auto loadStepShape = [](const CString& filePath, TopoDS_Shape& outShape) -> bool {
		if (filePath.IsEmpty())
			return false;
		Handle(TDocStd_Document) doc;
		Handle(XCAFApp_Application) app = XCAFApp_Application::GetApplication();
		app->NewDocument("MDTV-XCAF", doc);
		STEPCAFControl_Reader localReader;
		IFSelect_ReturnStatus status = localReader.ReadFile((Standard_CString)(LPCTSTR)filePath);
		if (status != IFSelect_RetDone)
			return false;
		localReader.Transfer(doc);
		Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());
		TDF_LabelSequence freeShapes;
		shapeTool->GetFreeShapes(freeShapes);
		if (freeShapes.IsEmpty())
			return false;
		outShape = shapeTool->GetShape(freeShapes.Value(1));
		return !outShape.IsNull();
	};

	CString appPath = CA2T(GetAppPath().c_str());
	CString fixedModelPath = appPath + _T("\\fixed.step");
	TopoDS_Shape fixedShape;
	if (loadStepShape(fixedModelPath, fixedShape))
	{
		m_autoModel = new AIS_Shape(fixedShape);
		m_autoModel->SetColor(Quantity_NOC_GRAY50);
		pView->Add(m_autoModel);
		g_modelopened.ClearOffscreenExcludes();
		g_modelopened.AddOffscreenExclude(m_autoModel);
	}
	else
	{
		g_modelopened.ClearOffscreenExcludes();
	}

	pView->Add(myAISShape);
	pView->FitAll();

	// 读取坐标系信息
	TDF_LabelSequence allShapes
		;
	shapeTool
		->GetShapes(allShapes);

	bool hasTrihedron = false;
	Quantity_Color xCol
		, yCol, zCol;
	gp_Pnt
		origin(0, 0, 0);
	gp_Dir
		xDir(1, 0, 0);
	gp_Dir
		yDir(0, 1, 0);
	gp_Dir
		zDir(0, 0, 1);
	for (Standard_Integer i = 1; i <= allShapes.Length(); ++i) {
		TDF_Label label
			= allShapes.Value(i);
		Handle(TDataStd_Name) nameAttr;
		if (!label.FindAttribute(TDataStd_Name::GetID(), nameAttr)) continue;

		std
			::string shapeName = TCollection_AsciiString(nameAttr->Get()).ToCString();
		if (shapeName == "Trihedron") {
			hasTrihedron
				= true;

			// 获取坐标系几何形状
			TopoDS_Shape triShape
				= shapeTool->GetShape(label);
			if (!triShape.IsNull() && triShape.ShapeType() == TopAbs_COMPOUND) {
				// 解析子形状
				TDF_LabelSequence subLabels
					;
				shapeTool
					->GetSubShapes(label, subLabels);

				std
					::map<std::string, TopoDS_Edge> axisMap;

				for (Standard_Integer j = 1; j <= subLabels.Length(); ++j) {
					TDF_Label subLabel
						= subLabels.Value(j);
					Handle(TDataStd_Name) subNameAttr;
					if (!subLabel.FindAttribute(TDataStd_Name::GetID(), subNameAttr)) continue;

					std
						::string subName = TCollection_AsciiString(subNameAttr->Get()).ToCString();

					// 获取颜色
					Quantity_Color c
						;
					if (colorTool->GetColor(subLabel, XCAFDoc_ColorGen, c)) {
						if (subName == "XAxis") xCol = c;
						else if (subName == "YAxis") yCol = c;
						else if (subName == "ZAxis") zCol = c;
					}

					// 获取几何边
					TopoDS_Shape subShape
						= shapeTool->GetShape(subLabel);
					if (!subShape.IsNull() && subShape.ShapeType() == TopAbs_EDGE) {
						axisMap
							[subName] = TopoDS::Edge(subShape);
					}
				}
				// 提取坐标系几何信息
				if (axisMap.count("XAxis") && axisMap.count("YAxis") && axisMap.count("ZAxis")) {
					TopoDS_Vertex vStart
						, vEnd;

					// 提取原点 (假设所有轴共享起点)
					TopExp::Vertices(axisMap["XAxis"], vStart, vEnd);
					origin
						= BRep_Tool::Pnt(vStart);

					// 提取方向向量
					TopExp::Vertices(axisMap["XAxis"], vStart, vEnd);
					gp_Vec
						xVec(BRep_Tool::Pnt(vStart), BRep_Tool::Pnt(vEnd));
					if (xVec.Magnitude() > Precision::Confusion())
						xDir
						= gp_Dir(xVec);

					TopExp::Vertices(axisMap["ZAxis"], vStart, vEnd);
					gp_Vec
						zVec(BRep_Tool::Pnt(vStart), BRep_Tool::Pnt(vEnd));
					if (zVec.Magnitude() > Precision::Confusion())
						zDir
						= gp_Dir(zVec);
				}
			}
			break;
		}
	}

	// 重建实际坐标系
	// ImportSTEP 里
	if (hasTrihedron) {
		gp_Ax2 actualCoord(origin, zDir, xDir);
		pView->CreateCoordinate(actualCoord);   // 传地址
	}

    // 再拼接路径
    CString snapPath = appPath + _T("\\png\\objectSnap.png");
    pView->CaptureScreenPng(snapPath);
	/* 5. 刷新界面 */
	if (!switchflag) newfile = true;
	FileName = name;
	::SendMessage(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd2.GetPane(0, 0)->GetSafeHwnd(),
		WM_PAINT, 0, 0);
}


void CStl3DLasetDoc::DrawSphere()
{
	gp_Pnt center = { 0.0,0.0,0.0 };
	BRepPrim_Sphere(center, 5.0);
}


