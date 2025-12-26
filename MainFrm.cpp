// MainFrm.cpp : CMainFrame 类的实现
//>
#include "stdafx.h"
#include "Stl3DLaset.h"
#include "CMenuEx.h"
#include "MainFrm.h"
#include "CPDFManager.h"
#include "OpenFileView.h"
#include "CoordinateXYZDlg.h"
#include "feature.h"  // 功能模块头文件

//#include "MFCLibrary3.h"
// MFC 框架头文件
#include <afxwinappex.h>
#include <afxvisualmanager.h>
#include <afxribbonstatusbar.h>
#include <afxribbonbar.h>
#include <afxvisualmanagerofficexp.h>
#include <afxvisualmanageroffice2007.h>
#include <afxvisualmanagerwindows.h>
#include <afxmsg_.h>
#include <afxbasetabctrl.h>
#include <afxoutlookbartabctrl.h>
#include <afxribbonstatusbarpane.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMainFrame
IMPLEMENT_DYNCREATE(CMainFrame, CFrameWndEx)
BEGIN_MESSAGE_MAP(CMainFrame, CFrameWndEx)
    ON_WM_CREATE()
    //ON_BN_CLICKED(IDC_BUTTON_PICK_TARGET, &COccView::OnBnClickedPickTarget)
    //ON_COMMAND(ID_TEST_SAVE_IMAGE, &COccView::OnBnClickedButtonSaveImage)
    ON_COMMAND(IDB_BUILDLINE1, &CMainFrame::OnButtonsaveml)
    //	ON_COMMAND(ID_VIEW_CAPTION, &CMainFrame::OnBnClickedButtonSelectTarget)//ON_COMMAND_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_OFF_2007_BLACK, &CMainFrame::OnApplicationLook)
        //ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_APPLOOK_WIN_2000, ID_VIEW_APPLOOK_OFF_2007_BLACK, &CMainFrame::OnUpdateApplicationLook)
    ON_COMMAND(ID_VIEW_CAPTION_BAR, &CMainFrame::OnViewCaptionBar)
    ON_UPDATE_COMMAND_UI(ID_VIEW_CAPTION_BAR, &CMainFrame::OnUpdateViewCaptionBar)
    ON_COMMAND(ID_TOOLS_OPTIONS, &CMainFrame::OnOptions)
    ON_COMMAND(IDC_BTN_OPEN_3D, &OpenFileView::OnButton3D)
    ON_BN_CLICKED(IDC_BTN_START, &OpenFileView::OnButtonStartCollecting)
    ON_BN_CLICKED(IDC_BTN_SAVE, &OpenFileView::OnButtonSaveSTL)
    ON_BN_CLICKED(IDC_BTN_PCD2STL, &OpenFileView::OnButtonPCD2STL)
    ON_BN_CLICKED(IDC_BTN_CANCEL, &OpenFileView::OnButtonCancelCollecting)
    ON_BN_CLICKED(IDC_BTN_PAUSE, &OpenFileView::OnButtonPause)
    ON_BN_CLICKED(IDC_BTN_REPORT, &CMainFrame::OnButtonReport1)
    ON_BN_CLICKED(IDC_BTN_CLEAR, &OpenFileView::OnButtonClear)
    ON_BN_CLICKED(IDC_BTN_REPORT2, &CMainFrame::OnButtonReport1m)
    //ON_COMMAND(IDB_COORDINATE32, &CMainFrame::OnButtonCoordinate32)
    ON_BN_CLICKED(IDB_COORDINATE32, OnCoordinateplane)  //
    ON_BN_CLICKED(IDB_COORDINATE3, OnCoordinateplane1)  //
    ON_BN_CLICKED(IDB_COORDINATE, OnCoordinateplane2)  //
    ON_COMMAND(IDB_BUILDPLANE1, OnBuildPlane1)
    ON_BN_CLICKED(IDB_Line1, OnBuildLine)
    ON_BN_CLICKED(IDB_Point1, OnBuildPoint)
    ON_COMMAND(IDB_COORDINATE321, &feature::OnButtonCoordinate321)
    ON_COMMAND(IDB_COORDINATEXYZ, &feature::OnButtonCoordinateXYZ)
    ON_COMMAND(IDB_COORDINATE3PLANE, &feature::OnButtonCoordinate3Plane)
    ON_COMMAND(IDB_ADDCOORD, &feature::OnButtonAddCoord)
    ON_COMMAND(IDB_BUILDPLANE, &feature::OnButtonBuildPlane)
    ON_COMMAND(IDB_BUILDLINE, &feature::OnButtonBuildLine)
    ON_COMMAND(IDB_BUILDPOINT, &feature::OnButtonBuildPoint)
    ON_COMMAND(IDB_OK, &BuildLineDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &BuildLineDlg::OnBnClickedCancel)
    ON_BN_CLICKED(IDB_PREVIEW, &BuildLineDlg::OnBnPreview)
    //ON_BN_CLICKED(IDC_BTN_REPORT, &OpenFileView::OnButtonReport)
    ON_CBN_SELCHANGE(ID_COMBOXCHOICE, &BuildLineDlg::OnChoiceChange)
    ON_BN_CLICKED(IDB_OK, &BuildPlaneDlg::OnBnClickedOk)
    ON_BN_CLICKED(IDCANCEL, &BuildPlaneDlg::OnBnClickedCancel)
    ON_BN_CLICKED(IDB_PREVIEW, &BuildPlaneDlg::OnBnPreview)
    //ON_BN_CLICKED(IDC_BTN_GENERATE_VIEWS, OnBnClickedGenerateViews)
    //	ON_COMMAND(ID_ZOOM_1X, &COccView::OnZoom1x)
    ON_COMMAND(ID_ZOOM_2X, OnZoom2x)
    ON_COMMAND(ID_ZOOM_4X, OnZoom4x)
    ON_CBN_SELCHANGE(ID_COMBOXCHOICE, &BuildPlaneDlg::OnChoiceChange)
    ON_COMMAND(ID_32796, &CMainFrame::OnFileOpen)
    ON_COMMAND(ID_32797, &CMainFrame::OnScan)
    ON_COMMAND(ID_32798, &CMainFrame::OnView)
    ON_COMMAND(ID_32799, &CMainFrame::OnEdit)
    ON_COMMAND(ID_32800, &CMainFrame::Onfeature)
    ON_COMMAND(ID_32801, &CMainFrame::OnMeasure)
    ON_COMMAND(ID_32802, &CMainFrame::OnCompare)
    ON_COMMAND(ID_32803, &CMainFrame::OnAnalysis)
    //ON_COMMAND(IDB_BUILDPOINT, &feature::OnRibbonButtonClicked)
    ON_COMMAND(ID_32804, &CMainFrame::OnReport)
    ON_COMMAND(ID_32805, &CMainFrame::OnSwitch3D2D)
    //ON_COMMAND(ID_MENUSAMPLING, &SelectedFeature::ONMenuSampling)
    ON_COMMAND(ID_MENUMEASURE, &SelectedFeature::ONMenuMeasure)
    ON_COMMAND(ID_MENUHIDE, &SelectedFeature::ONMenuHide)
    ON_COMMAND(ID_MENUSHOW, &SelectedFeature::ONMenuShow)
    ON_COMMAND(ID_MENUDELETE, &SelectedFeature::ONMenuDelete)
    ON_BN_CLICKED(IDB_PLANE, &feature::OnButtonplane)
    ON_BN_CLICKED(IDB_CURVE, &feature::OnButtonCurve)
    ON_BN_CLICKED(IDB_VERTEX, &feature::OnButtonVertex)
    ON_BN_CLICKED(IDB_PREVIEW, &MeasureArcDlg::OnBnPreview)
    ON_BN_CLICKED(IDB_MEASURE, &MeasureArcDlg::OnBnMeasure)
    ON_BN_CLICKED(IDB_PREVIEW, &MeasurePlnDlg::OnBnPreview)
    ON_BN_CLICKED(IDB_MEASURE, &MeasurePlnDlg::OnBnPreview)
    ON_CBN_SELCHANGE(IDC_COMBO_REFER, &MeasurePlnDlg::OnReferenceChange)
    ON_BN_CLICKED(IDCANCEL, &MeasurePlnDlg::OnBnClickedCancel)
    ON_BN_CLICKED(IDB_PREVIEW, &MeasureLNDlg::OnBnPreview)
    ON_BN_CLICKED(IDB_MEASURE, &MeasureLNDlg::OnBnMeasure)
    ON_BN_CLICKED(IDB_PREVIEW, &MeasureCYLDlg::OnBnPreview)
    ON_BN_CLICKED(IDB_MEASURE, &MeasureCYLDlg::OnBnMeasure)
    ON_BN_CLICKED(IDB_PREVIEW, &MeasureCir::OnBnPreview)
    ON_BN_CLICKED(IDB_MEASURE, &MeasureCir::OnBnMeasure)
    ON_BN_CLICKED(IDB_PREVIEW, &MeasureArcDlg::OnBnPreview)
    ON_BN_CLICKED(IDB_MEASURE, &MeasureArcDlg::OnBnMeasure)

    ON_WM_WINDOWPOSCHANGED()
    ON_BN_CLICKED(IDC_BTN_FRONT_VIEW, &ShowView::OnButtonFront)
    ON_BN_CLICKED(IDC_BTN_SIDE_VIEW, &ShowView::OnButtonSide)
    ON_BN_CLICKED(IDC_BTN_VERTICAL_VIEW, &ShowView::OnButtonUp)
    ON_BN_CLICKED(IDB_PLANE1, OnButtonplane1)

    ON_COMMAND(ID_VIEW_TOGGLEGRID, &CMainFrame::OnToggleGrid)

    ON_WM_MOUSEMOVE()
    ON_WM_MOUSEWHEEL()
    ON_WM_LBUTTONDOWN()
    ON_WM_LBUTTONUP()
    ON_WM_MBUTTONDOWN()
    ON_WM_CLOSE()
    ON_WM_MBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_WM_TIMER()
    ON_WM_SIZE()
    ON_WM_MOVE()

END_MESSAGE_MAP()

// CMainFrame 构造/析构

// CMainFrame 实现
CMainFrame::CMainFrame()
    : m_b2DIf(FALSE), m_strReportSavePath(_T(""))  // 初始化：未选择目录
{
    // 或者根据实际项目结构获取feature实例
         //theApp.m_nAppLook = theApp.GetInt(_T("ApplicationLook"), ID_VIEW_APPLOOK_OFF_2007_BLUE);
        // 在程序初始化时调用黑色主题（例如在 CMainFrame::OnCreate 末尾）
    m_bAutoDelete = TRUE;
    OnApplicationLook(ID_VIEW_APPLOOK_OFF_2007_BLACK); // 直接触发黑色主题设置
    // 成员变量声明
    CGoodVisionDlg m_GoodVisonDlg;
    BOOL m_b2DIf;
    // TODO: 在此添加成员初始化代码
}
#include <afxribboncategory.h>  // Ribbon类别定义
#include <afxribbonpanel.h>  // Ribbon面板定义
CMainFrame::~CMainFrame()
{
}
int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CFrameWndEx::OnCreate(lpCreateStruct) == -1)
        return -1;
    BOOL bNameValid;
    m_wndRibbonBar.Create(this);
    //m_wndRibbonBar.LoadFromResource(IDR_RIBBON);
    //if (!m_wndStatusBar.Create(this)) {
    //	TRACE0("未能创建状态栏\n");
    //	return -1;      // 未能创建
    //}
    // 创建标题栏: 

    m_pCategoryHome
        = m_wndRibbonBar.AddCategory(
            _T("主页"),           // 分类名称
            IDB_BITMAP38,    // 小图标资源ID
            IDB_BITMAP37	  // 大图标资源ID
        );
    CMFCRibbonPanel
        * pPanelElement1 = m_pCategoryHome->AddPanel(_T("加载数据"));
    CMFCRibbonPanel
        * pPanelElement2 = m_pCategoryHome->AddPanel(_T("保存"));


    CMFCRibbonPanel
        * pPanelElement4 = m_pCategoryHome->AddPanel(_T("分析"));

    CMFCRibbonPanel
        * pPanelElement5 = m_pCategoryHome->AddPanel(_T("方向"));
    CMFCRibbonPanel
        * pPanelElement6 = m_pCategoryHome->AddPanel(_T("报告"));
    CMFCRibbonPanel
        * pPanelElement7 = m_pCategoryHome->AddPanel(_T("生成网格"));
    if (pPanelElement1) {
        //	pPanelElement1->Add(new CMFCRibbonButton(IDD_FILE_STLDATA, _T("导入扫描文件"), 0, 0, TRUE));  // 索引4
        pPanelElement1->Add(new CMFCRibbonButton(IDD_FILE_STLDATA, _T("导入3D文件"), 0, 0, TRUE));  // 索引4

        //	pPanelElement1->Add(new CMFCRibbonButton(IDD_FILE_STLDATA, _T("加载探头"), 2, 2, TRUE));  // 索引5
        pPanelElement2->Add(new CMFCRibbonButton(IDC_BTN_SAVE, _T("保存"), 1, 1, TRUE));  // 索引6
        //	pPanelElement3->Add(new CMFCRibbonButton(IDC_BTN_PCD2STL, _T("PCD2STL"), 2, 2, TRUE));  // 索引4
        pPanelElement4->Add(new CMFCRibbonButton(IDC_BTN_PCD2STL1, _T("厚度分布图"), 2, 2, TRUE));  // 索引4
        pPanelElement4->Add(new CMFCRibbonButton(IDC_BTN_PCD2STL2, _T("中平面分布图"), 3, 3, TRUE));  // 索引4

        pPanelElement5->Add(new CMFCRibbonButton(IDC_BTN_FRONT_VIEW, _T("正视图"), 4, 4, TRUE));  // 索引6
        pPanelElement5->Add(new CMFCRibbonButton(IDC_BTN_VERTICAL_VIEW, _T("侧视图"), 5, 5, TRUE));  // 索引4
        pPanelElement5->Add(new CMFCRibbonButton(IDC_BTN_SIDE_VIEW, _T("俯视图"), 6, 6, TRUE));  // 索引5
        //pPanelElement6->Add(new CMFCRibbonButton(IDB_BUILDLINE1, _T("PDF设置"), 7, 7, TRUE));  // 索引5
        pPanelElement6->Add(new CMFCRibbonButton(IDC_BTN_REPORT2, _T("生成报告"), 8, 8, TRUE));  // 索引6
        //pPanelElement6->Add(new CMFCRibbonButton(IDB_BUILDLINE, _T("清理报表数据"), 9, 9, TRUE));  // 索引5
        //pPanelElement6->Add(new CMFCRibbonButton(IDB_BUILDLINE2, _T("自动生成报告"), 10, 10, TRUE));  // 索引5
        pPanelElement7->Add(new CMFCRibbonButton(ID_VIEW_TOGGLEGRID, _T("显示网格"), 10, 10, TRUE));
    }

    m_pCategoryst
        = m_wndRibbonBar.AddCategory(
            _T("视图"),           // 分类名称
            IDB_BITMAP15
            ,    // 小图标资源ID
            IDB_BITMAP14
        );   // 大图标资源ID
    // 添加面板到分类
    CMFCRibbonPanel
        * pPanelst = m_pCategoryst->AddPanel(_T("方向1"));
    CMFCRibbonPanel
        * pPanelst1 = m_pCategoryst->AddPanel(_T("方向2"));
    CMFCRibbonPanel
        * pPanelst2 = m_pCategoryst->AddPanel(_T("方向3"));
    // 在添加按钮时指定图标索引
    if (pPanelst) {
        // 使用图标索引(对应InitRibbonBitmaps中添加的顺序)
        pPanelst->Add(new CMFCRibbonButton(IDC_BTN_FRONT_VIEW, _T("正视图"), 0, 0, TRUE));  // 索引1
        pPanelst1->Add(new CMFCRibbonButton(IDC_BTN_SIDE_VIEW, _T("侧视图"), 1, 1, TRUE));  // 索引2
        pPanelst2->Add(new CMFCRibbonButton(IDC_BTN_VERTICAL_VIEW, _T("俯视图"), 2, 2, TRUE));  // 索引3
    }
    m_pCategorysm
        = m_wndRibbonBar.AddCategory(
            _T("扫描"),           // 分类名称
            IDB_BITMAP17
            ,    // 小图标资源ID
            IDB_BITMAP16
        );   // 大图标资源ID
    // 添加面板到分类
    CMFCRibbonPanel
        * pPanelsm = m_pCategorysm->AddPanel(_T("开始"));
    CMFCRibbonPanel
        * pPanelsm1 = m_pCategorysm->AddPanel(_T("结束"));
    CMFCRibbonPanel
        * pPanelsm2 = m_pCategorysm->AddPanel(_T("暂停/继续"));
    // 在添加按钮时指定图标索引
    if (pPanelst) {
        // 使用图标索引(对应InitRibbonBitmaps中添加的顺序)
        pPanelsm->Add(new CMFCRibbonButton(IDC_BTN_START, _T("开始扫描"), 0, 0, TRUE));  // 索引1
        pPanelsm1->Add(new CMFCRibbonButton(IDC_BTN_CANCEL, _T("结束扫描"), 1, 1, TRUE));  // 索引2
        pPanelsm2->Add(new CMFCRibbonButton(IDC_BTN_PAUSE, _T("暂停/继续"), 2, 2, TRUE));  // 索引3
    }
    //m_pCategorycl
    //	= m_wndRibbonBar.AddCategory(
    //		_T("测量"),           // 分类名称
    //		IDB_BITMAP19
    //		,    // 小图标资源ID
    //		IDB_BITMAP18
    //	);   // 大图标资源ID
    //// 添加面板到分类
    //CMFCRibbonPanel
    //	* pPanelcl = m_pCategorycl->AddPanel(_T("直线"));
    //CMFCRibbonPanel
    //	* pPanelcl1 = m_pCategorycl->AddPanel(_T("结束"));
    //CMFCRibbonPanel
    //	* pPanelcl2 = m_pCategorycl->AddPanel(_T("暂停/继续"));
    //CMFCRibbonPanel
    //	* pPanelcl3 = m_pCategorycl->AddPanel(_T("暂停/继续"));
    //CMFCRibbonPanel
    //	* pPanelcl4 = m_pCategorycl->AddPanel(_T("暂停/继续"));
    //// 在添加按钮时指定图标索引
    //if (pPanelcl) {
    //	// 使用图标索引(对应InitRibbonBitmaps中添加的顺序)
    //	pPanelcl->Add(new CMFCRibbonButton(IDC_BUTTON_PICK_TARGET, _T("相对"), 0, 0, TRUE));
    //	pPanelcl1->Add(new CMFCRibbonButton(IDC_BUTTON_GET_CAMERA, _T("相机位置变化"), 1, 1, TRUE));
    //	pPanelcl2->Add(new CMFCRibbonButton(ID_TEST_SAVE_IMAGE, _T("成像"), 2, 2, TRUE));
    //	pPanelcl3->Add(new CMFCRibbonButton(IDC_BUTTON_PICK_TARGET1, _T("景深"), 3, 3, TRUE));
    //	pPanelcl4->Add(new CMFCRibbonButton(ID_ZOOM_4X, _T("暂停/继续4"), 4, 4, TRUE));
    //}  // 索引3
    m_pCategoryFeature
        = m_wndRibbonBar.AddCategory(
            _T("特征"),           // 分类名称
            IDB_BITMAP21
            ,    // 小图标资源ID
            IDB_BITMAP20
        );   // 大图标资源ID
    // 添加面板到分类
    CMFCRibbonPanel
        * pPanelElement = m_pCategoryFeature->AddPanel(_T("元素选取"));
    CMFCRibbonPanel
        * pPanelCoordinate = m_pCategoryFeature->AddPanel(_T("坐标系建立"));
    CMFCRibbonPanel
        * pPanelBuild = m_pCategoryFeature->AddPanel(_T("构造元素"));
    // 在添加按钮时指定图标索引
    if (pPanelElement) {
        // 使用图标索引(对应InitRibbonBitmaps中添加的顺序)
        pPanelElement->Add(new CMFCRibbonButton(IDB_PLANE, _T("面型元素"), 0, 0, TRUE));  // 索引1
        pPanelElement->Add(new CMFCRibbonButton(IDB_CURVE, _T("线型元素"), 1, 1, TRUE));  // 索引2
        pPanelElement->Add(new CMFCRibbonButton(IDB_VERTEX, _T("点元素"), 2, 2, TRUE));  // 索引3
    }
    if (pPanelCoordinate) {
        pPanelCoordinate->Add(new CMFCRibbonButton(IDB_COORDINATE32, _T("3-2-1基准"), 3, 3, TRUE));  // 索引4
        pPanelCoordinate->Add(new CMFCRibbonButton(IDB_COORDINATE3, _T("XYZ基准"), 4, 4, TRUE));  // 索引5
        pPanelCoordinate->Add(new CMFCRibbonButton(IDB_COORDINATE, _T("三面基准"), 5, 5, TRUE));  // 索引6
    }
    if (pPanelBuild) {
        pPanelBuild->Add(new CMFCRibbonButton(IDB_BUILDPLANE1, _T("构造平面"), 6, 6, TRUE));  // 索引4
        pPanelBuild->Add(new CMFCRibbonButton(IDB_Line1, _T("构造直线"), 7, 7, TRUE));  // 索引5
        pPanelBuild->Add(new CMFCRibbonButton(IDB_Point1, _T("构造点"), 8, 8, TRUE));  // 索引6
    }
    m_pCategorybg
        = m_wndRibbonBar.AddCategory(
            _T("报告"),           // 分类名称
            IDB_BITMAP25
            ,    // 小图标资源ID
            IDB_BITMAP24
        );   // 大图标资源ID
    CMFCRibbonPanel
        * pPanelbg = m_pCategorybg->AddPanel(_T("查看报告"));
    CMFCRibbonPanel
        * pPanelbg1 = m_pCategorybg->AddPanel(_T("清理报告"));
    if (pPanelbg) {
        pPanelbg->Add(new CMFCRibbonButton(IDC_BTN_REPORT2, _T("生成报告"), 0, 0, TRUE));  // 索引1
        //pPanelbg1->Add(new CMFCRibbonButton(IDB_BUILDLINE, _T("清理报告数据"), 1, 1, TRUE));  //补上
    }


    //m_Menu.Detach();
    CString strTitlePane1;
    CString strTitlePane2;
    bNameValid = strTitlePane1.LoadString(IDS_STATUS_PANE1);
    bNameValid = strTitlePane2.LoadString(IDS_STATUS_PANE2);
    m_wndStatusBar.AddElement(new CMFCRibbonStatusBarPane(ID_STATUSBAR_PANE1, strTitlePane1, TRUE), strTitlePane1);
    m_wndStatusBar.AddExtendedElement(new CMFCRibbonStatusBarPane(ID_STATUSBAR_PANE2, strTitlePane2, TRUE), strTitlePane2);
    return 0;
}
//321z建立坐标系按钮的处理函数
void CMainFrame::OnCoordinateplane()
{
    // 2. 调用 feature 的方法
    if (m_FeatureMenu) // 检查指针有效性
    {
        m_FeatureMenu->OnButtonCoordinate321();
    }
}//构造平面按钮的处理函数
void CMainFrame::OnCoordinateplane1()
{	// 2. 调用 feature 的方法
    if (m_FeatureMenu) // 检查指针有效性
    {
        m_FeatureMenu->OnButtonCoordinateXYZ();
    }
}
void CMainFrame::OnCoordinateplane2()
{	// 2. 调用 feature 的方法
    if (m_FeatureMenu) // 检查指针有效性
    {
        m_FeatureMenu->OnButtonCoordinate3Plane();
    }
}//构造平面按钮的处理函数
void CMainFrame::OnBuildPlane1()
{

    // 2. 调用 feature 的方法
    if (m_FeatureMenu) // 检查指针有效性
    {
        m_FeatureMenu->OnButtonBuildPlane();
    }
}
void CMainFrame::OnBuildLine()
{
    if (m_FeatureMenu) // 检查指针有效性
    {
        m_FeatureMenu->OnButtonBuildLine();
    }
}
// MainFrm.cpp
// 构造点按钮的处理函数															  
void CMainFrame::OnBuildPoint()
{
    // 2. 调用 feature 的方法
    if (m_FeatureMenu) // 检查指针有效性
    {
        m_FeatureMenu->OnButtonBuildPoint();
    }
}
void CMainFrame::OnButtonplane1()
{

    // 2. 调用 feature 的方法
    if (m_FeatureMenu) // 检查指针有效性
    {
        m_FeatureMenu->OnButtonplane();
    }
}
//

// 非模态对话框版本（保持对话框打开）
#include "afxvisualmanagerwindows7.h" // Windows 7视觉管理器
#include <afxshelltreectrl.h>
#include <afxwinappex.h>
#include<afxmdiframewndex.h>
// MainFrm.cpp




BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    if (!CFrameWndEx::PreCreateWindow(cs))
        return FALSE;
    // TODO: 在此处通过修改 CREATESTRUCT cs 来修改窗口类或
    // 样式
    cs.style = WS_OVERLAPPED | WS_CAPTION | FWS_ADDTOTITLE
        | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_MAXIMIZE | WS_SYSMENU;
    return TRUE;
}
// MainFrm.cpp 中完善导航栏创建函数
// MainFrm.cpp 中添加同步函数
BOOL CMainFrame::CreateOutlookBar(CMFCOutlookBar& bar, CMFCShellTreeCtrl& tree, UINT uiID, int nInitialWidth)
{
    bar.SetMode2003();
    BOOL bNameValid;
    CString strTemp;
    bNameValid = strTemp.LoadString(IDS_SHORTCUTS);

    if (!bar.Create(strTemp, this, CRect(0, 0, nInitialWidth, 32000), uiID, WS_CHILD | WS_VISIBLE | CBRS_LEFT))
    {
        return FALSE;
    }

    CMFCOutlookBarTabCtrl* pOutlookBar = (CMFCOutlookBarTabCtrl*)bar.GetUnderlyingWindow();
    pOutlookBar->EnableInPlaceEdit(TRUE);

    static UINT uiPageID = 1;
    DWORD dwStyle = AFX_CBRS_FLOAT | AFX_CBRS_AUTOHIDE | AFX_CBRS_RESIZE;
    CRect rectDummy(0, 0, 0, 0);
    const DWORD dwTreeStyle = WS_CHILD | WS_VISIBLE | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS;

    tree.Create(dwTreeStyle, rectDummy, &bar, 1200);
    bNameValid = strTemp.LoadString(IDS_FOLDERS);

    pOutlookBar->AddControl(&tree, strTemp, 2, TRUE, dwStyle);
    bar.SetPaneStyle(bar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC);

    // 修改：使用 AfxGetApp() 替代 theApp
    CWinApp* pApp = AfxGetApp();
    BOOL bHiColorIcons = FALSE;
    if (pApp)
    {
        // bHiColorIcons = pApp->m_bHiColorIcons;
    }

    pOutlookBar->SetImageList(bHiColorIcons ? IDB_PAGES_HC : IDB_PAGES, 24);
    pOutlookBar->SetToolbarImageList(bHiColorIcons ? IDB_PAGES_SMALL_HC : IDB_PAGES_SMALL, 16);
    pOutlookBar->RecalcLayout();
    bar.SetButtonsFont(&afxGlobalData.fontBold);

    return TRUE;
}

BOOL CMainFrame::CreateCaptionBar()
{
    if (!m_wndCaptionBar.Create(WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS, this, ID_VIEW_CAPTION_BAR, -1, TRUE))
    {
        TRACE0("未能创建标题栏\n");
        return FALSE;
    }

    BOOL bNameValid;

    CString strTemp, strTemp2;

    // 设置按钮
    bNameValid = strTemp.LoadString(IDS_CAPTION_BUTTON);

    m_wndCaptionBar.SetButton(strTemp, ID_TOOLS_OPTIONS, CMFCCaptionBar::ALIGN_LEFT, FALSE);
    bNameValid = strTemp.LoadString(IDS_CAPTION_BUTTON_TIP);

    m_wndCaptionBar.SetButtonToolTip(strTemp);

    // 动态设置标题文本 - 使用当前文件名
    UpdateCaptionBarText();

    // 设置图标
    m_wndCaptionBar.SetBitmap(IDB_INFO, RGB(255, 255, 255), FALSE, CMFCCaptionBar::ALIGN_LEFT);
    bNameValid = strTemp.LoadString(IDS_CAPTION_IMAGE_TIP);

    bNameValid = strTemp2.LoadString(IDS_CAPTION_IMAGE_TEXT);

    m_wndCaptionBar.SetImageToolTip(strTemp, strTemp2);

    return TRUE;
}

// 添加更新标题栏文本的方法
void CMainFrame::UpdateCaptionBarText()
{
    CString titleText;

    // 如果有当前文件名，显示"文件名 - 应用标题"
    if (!m_currentFileName.IsEmpty())
    {
        CString appTitle;
        appTitle.LoadString(IDS_APP_TITLE); // 应用程序标题
        titleText.Format(_T("%s - %s"), m_currentFileName, appTitle);
    }
    else
    {
        // 没有文件名时，显示默认标题
        titleText.LoadString(IDS_CAPTION_TEXT);
    }

    // 设置标题文本
    m_wndCaptionBar.SetText(titleText, CMFCCaptionBar::ALIGN_LEFT);
    m_wndCaptionBar.Invalidate(); // 强制重绘
}
#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
    CFrameWndEx::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
    CFrameWndEx::Dump(dc);
}

#endif //_DEBUG

// CMainFrame.cpp
// MainFrm.cpp
// MainFrm.cpp
// 假设 CMainFrame 是当前类

// CMainFrame 消息处理程序
//窗口切割拆分，初始化各个窗口视图
BOOL CMainFrame::OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext)
{
    // TODO: 在此添加专用代码和/或调用基类
    CRect rc;
    GetClientRect(&rc);//获取客户区
    m_wndSplitterWnd.CreateStatic(this, 2, 1);
    m_wndSplitterWnd.SetRowInfo(0, rc.Height() * 0.0001, 20);
    m_wndSplitterWnd.SetRowInfo(1, rc.Height() * 0.9999, 20);
    //将（1，0）区域进行二次拆分
    m_wndSplitterWnd1.CreateStatic(&m_wndSplitterWnd, 1, 2, WS_CHILD | WS_VISIBLE, m_wndSplitterWnd.IdFromRowCol(1, 0));
    ////m_wndSplitterWnd1.CreateView(0, 0, RUNTIME_CLASS(CIndexTreeView), CSize(200, 0), pContext);
    m_wndSplitterWnd1.SetColumnInfo(0, rc.Width() / 5, 20);
    m_wndSplitterWnd1.SetColumnInfo(1, rc.Width() * 4 / 5, 20);
    ////////////////////////////////////////////////////按钮切换
    //m_CoordView = DYNAMIC_DOWNCAST(CIndexTreeView, RUNTIME_CLASS(CIndexTreeView)->CreateObject());
    //将（0，0）区域进行二次拆分
    m_wndSplitterWnd2.CreateStatic(&m_wndSplitterWnd1, 2, 1, WS_CHILD | WS_VISIBLE, m_wndSplitterWnd1.IdFromRowCol(0, 0));
    m_wndSplitterWnd2.CreateView(0, 0, RUNTIME_CLASS(FileView), CSize(10, 0), pContext);
    // 固定第一行下方的分割线（索引0）

    // 创建按钮切换视图
    // 在第二行创建一个包含按钮的对话框栏
    //////////////////////////////
    //m_ProcedureView= DYNAMIC_DOWNCAST(CProcedureView, RUNTIME_CLASS(CProcedureView)->CreateObject());
    m_FeatureView = DYNAMIC_DOWNCAST(SelectedFeature, RUNTIME_CLASS(SelectedFeature)->CreateObject());
    m_AngleView = DYNAMIC_DOWNCAST(AngleView, RUNTIME_CLASS(AngleView)->CreateObject());
    m_BlankView = DYNAMIC_DOWNCAST(BlankView, RUNTIME_CLASS(BlankView)->CreateObject());
    //m_AddCoordView= DYNAMIC_DOWNCAST(AddCoord, RUNTIME_CLASS(AddCoord)->CreateObject());
    m_FeatureView->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd2, 0xFFFF, pContext);
    m_AngleView->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd2, 0xFFFF, pContext);
    m_BlankView->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd2, 0xFFFF, pContext);
    /*m_AddCoordView->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd2, 0xFFFF, pContext);*/

        //切换初始化
    SwitchFunction(1); // 1对应SelectedFeature视图
    ////////////////////////////////////////////////////显示视图切换
    //m_wndSplitterWnd1.CreateView(0, 1, RUNTIME_CLASS(CStl3DLasetView), CSize(0, 0), pContext);
    m_OccView = DYNAMIC_DOWNCAST(COccView, RUNTIME_CLASS(COccView)->CreateObject());
    m_MainView = DYNAMIC_DOWNCAST(CStl3DLasetView, RUNTIME_CLASS(CStl3DLasetView)->CreateObject());
    m_MainView->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd1, 0xFFFF, pContext);
    m_OccView->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd1, 0xFFFF, pContext);


    CStl3DLasetDoc* pDoc = (CStl3DLasetDoc*)m_OccView->GetDocument();
    m_OccView->SetViewer(pDoc->GetViewer());
    //m_OccView->SetAISContext(pDoc->myAISContext);


    SwitchView(0);
    SwitchFunction(1); // 显示Feature视图中的树控件
    //m_pView1 = DYNAMIC_DOWNCAST(CStl3DLasetView, RUNTIME_CLASS(CStl3DLasetView)->CreateObject());
    //m_pView2 = DYNAMIC_DOWNCAST(CStl3DLasetView, RUNTIME_CLASS(CStl3DLasetView)->CreateObject());
    /*m_pView1->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd1, 0xFFFF, pContext);*/
        /*m_pView2->Create(NULL, NULL, WS_CHILD,
            CRect(0, 0, 0, 0), &m_wndSplitterWnd1, 0xFFFF, pContext);*/

            //Switch(0);
            ///////////////////一级菜单切换//////////////////////////////////////////
            //m_wndSplitterWnd.CreateView(0, 0, RUNTIME_CLASS(TopView), CSize(0, 50), pContext);
    m_FileMenu = DYNAMIC_DOWNCAST(OpenFileView, RUNTIME_CLASS(OpenFileView)->CreateObject());
    m_ScanMenu = DYNAMIC_DOWNCAST(ScanView, RUNTIME_CLASS(ScanView)->CreateObject());
    m_ViewMenu = DYNAMIC_DOWNCAST(ShowView, RUNTIME_CLASS(ShowView)->CreateObject());
    m_CutMenu = DYNAMIC_DOWNCAST(TopView, RUNTIME_CLASS(TopView)->CreateObject());
    m_MeasureMenu = DYNAMIC_DOWNCAST(MeasureView, RUNTIME_CLASS(MeasureView)->CreateObject());
    m_CompareMenu = DYNAMIC_DOWNCAST(CompareView, RUNTIME_CLASS(CompareView)->CreateObject());
    m_AnalysisMenu = DYNAMIC_DOWNCAST(AnalysisView, RUNTIME_CLASS(AnalysisView)->CreateObject());
    m_FeatureMenu = DYNAMIC_DOWNCAST(feature, RUNTIME_CLASS(feature)->CreateObject());
    m_FileMenu->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd, 0xFFFF, pContext);
    m_ScanMenu->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd, 0xFFFF, pContext);
    m_ViewMenu->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd, 0xFFFF, pContext);
    m_CutMenu->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd, 0xFFFF, pContext);
    m_MeasureMenu->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd, 0xFFFF, pContext);
    m_CompareMenu->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd, 0xFFFF, pContext);
    m_AnalysisMenu->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd, 0xFFFF, pContext);
    m_FeatureMenu->Create(NULL, NULL, WS_CHILD,
        CRect(0, 0, 0, 0), &m_wndSplitterWnd, 0xFFFF, pContext);
    MenuSwitch(0);
    /*m_wndSplitterWnd1.SetColumnInfo(0, 0, 0);*/
    MenuSwitch(4);
    PostMessage(WM_SIZE, 0, 0); // 触发一次布局刷新
    return TRUE;
}


void CMainFrame::SwitchFunction(int functionnum)
{
    switch (functionnum)
    {
    case 0:
        // 空白视图
        ::SetWindowLong(m_BlankView->m_hWnd, GWL_ID, m_wndSplitterWnd2.IdFromRowCol(1, 0));
        m_BlankView->ShowWindow(SW_SHOW);
        // 隐藏其他视图
        break;
    case 1:
        // 特征视图（包含树控件）
        ::SetWindowLong(m_FeatureView->m_hWnd, GWL_ID, m_wndSplitterWnd2.IdFromRowCol(1, 0));
        m_FeatureView->ShowWindow(SW_SHOW);
        // 隐藏其他视图
        break;
    case 2:
        // 角度视图
        ::SetWindowLong(m_AngleView->m_hWnd, GWL_ID, m_wndSplitterWnd2.IdFromRowCol(1, 0));
        m_AngleView->ShowWindow(SW_SHOW);
        // 隐藏其他视图
        break;
    }
    m_wndSplitterWnd2.RecalcLayout();
}

//主视图切换
void CMainFrame::Switch(int nIndex)
{
    // TODO: 在此处添加实现代码.

    switch (nIndex)
    {
    case 0:
        ::SetWindowLong(m_pView1->m_hWnd, GWL_ID, m_wndSplitterWnd1.IdFromRowCol(0, 1));
        m_pView1->ShowWindow(SW_SHOW);
        ::SetWindowLong(m_pView2->m_hWnd, GWL_ID, 0xFFFF);
        m_pView2->ShowWindow(SW_HIDE);
        break;
    case 1:
        ::SetWindowLong(m_pView1->m_hWnd, GWL_ID, 0xFFFF);
        m_pView1->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_pView2->m_hWnd, GWL_ID, m_wndSplitterWnd1.IdFromRowCol(0, 1));
        m_pView2->ShowWindow(SW_SHOW);
        break;
    }
    m_wndSplitterWnd1.RecalcLayout();
}
//occ/stl视图切换
void CMainFrame::SwitchView(UINT id)
{
    // 检查视图有效性
    if (!m_OccView || !::IsWindow(m_OccView->GetSafeHwnd()) ||
        !m_MainView || !::IsWindow(m_MainView->GetSafeHwnd())) {
        AfxMessageBox(_T("视图未正确初始化"));
        return;
    }

    switch (id)
    {
    case 0:
        ::SetWindowLong(m_OccView->m_hWnd, GWL_ID, m_wndSplitterWnd1.IdFromRowCol(0, 1));
        m_OccView->ShowWindow(SW_SHOW);
        ::SetWindowLong(m_MainView->m_hWnd, GWL_ID, 0xFFFF);
        m_MainView->ShowWindow(SW_HIDE);
        activeindex = OccView;
        break;
    case 1:
        ::SetWindowLong(m_MainView->m_hWnd, GWL_ID, m_wndSplitterWnd1.IdFromRowCol(0, 1));
        m_MainView->ShowWindow(SW_SHOW);
        if (m_MainView) {
            m_MainView->ClearImage();
        }
        ::SetWindowLong(m_OccView->m_hWnd, GWL_ID, 0xFFFF);
        m_OccView->ShowWindow(SW_HIDE);
        activeindex = StlView;
        break;
    }

    if (m_wndSplitterWnd1.GetSafeHwnd()) {
        m_wndSplitterWnd1.RecalcLayout();
    }
}//获取当前活动的视图索引
ActiveView CMainFrame::GetActiveView()
{

    return activeindex;
}
//根据菜单栏切换对应的菜单显示
void CMainFrame::MenuSwitch(int menuindex)
{
    // TODO: 在此处添加实现代码
    switch (menuindex)
    {
    case 0:
        ::SetWindowLong(m_FileMenu->m_hWnd, GWL_ID, m_wndSplitterWnd.IdFromRowCol(0, 0));
        m_FileMenu->ShowWindow(SW_SHOW);
        ::SetWindowLong(m_ScanMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_ScanMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_ViewMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_ViewMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_CutMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_CutMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_MeasureMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_MeasureMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_CompareMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_CompareMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_AnalysisMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_AnalysisMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_FeatureMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_FeatureMenu->ShowWindow(SW_HIDE);
        break;
    case 1:
        ::SetWindowLong(m_FileMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_FileMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_ScanMenu->m_hWnd, GWL_ID, m_wndSplitterWnd.IdFromRowCol(0, 0));
        m_ScanMenu->ShowWindow(SW_SHOW);
        ::SetWindowLong(m_ViewMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_ViewMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_CutMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_CutMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_MeasureMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_MeasureMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_CompareMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_CompareMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_AnalysisMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_AnalysisMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_FeatureMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_FeatureMenu->ShowWindow(SW_HIDE);
        break;
    case 2:
        ::SetWindowLong(m_FileMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_FileMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_ScanMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_ScanMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_ViewMenu->m_hWnd, GWL_ID, m_wndSplitterWnd.IdFromRowCol(0, 0));
        m_ViewMenu->ShowWindow(SW_SHOW);
        ::SetWindowLong(m_CutMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_CutMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_MeasureMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_MeasureMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_CompareMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_CompareMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_AnalysisMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_AnalysisMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_FeatureMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_FeatureMenu->ShowWindow(SW_HIDE);
        break;
    case 3:
        ::SetWindowLong(m_FileMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_FileMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_ScanMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_ScanMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_ViewMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_ViewMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_CutMenu->m_hWnd, GWL_ID, m_wndSplitterWnd.IdFromRowCol(0, 0));
        m_CutMenu->ShowWindow(SW_SHOW);
        ::SetWindowLong(m_MeasureMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_MeasureMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_CompareMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_CompareMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_AnalysisMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_AnalysisMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_FeatureMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_FeatureMenu->ShowWindow(SW_HIDE);
        break;
    case 4:
        ::SetWindowLong(m_FileMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_FileMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_ScanMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_ScanMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_ViewMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_ViewMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_CutMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_CutMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_MeasureMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_MeasureMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_CompareMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_CompareMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_AnalysisMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_AnalysisMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_FeatureMenu->m_hWnd, GWL_ID, m_wndSplitterWnd.IdFromRowCol(0, 0));
        m_FeatureMenu->ShowWindow(SW_SHOW);
        break;
    case 5:
        ::SetWindowLong(m_FileMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_FileMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_ScanMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_ScanMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_ViewMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_ViewMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_CutMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_CutMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_MeasureMenu->m_hWnd, GWL_ID, m_wndSplitterWnd.IdFromRowCol(0, 0));
        m_MeasureMenu->ShowWindow(SW_SHOW);
        ::SetWindowLong(m_CompareMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_CompareMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_AnalysisMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_AnalysisMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_FeatureMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_FeatureMenu->ShowWindow(SW_HIDE);
        break;
    case 6:
        ::SetWindowLong(m_FileMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_FileMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_ScanMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_ScanMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_ViewMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_ViewMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_CutMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_CutMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_MeasureMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_MeasureMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_CompareMenu->m_hWnd, GWL_ID, m_wndSplitterWnd.IdFromRowCol(0, 0));
        m_CompareMenu->ShowWindow(SW_SHOW);
        ::SetWindowLong(m_AnalysisMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_AnalysisMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_FeatureMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_FeatureMenu->ShowWindow(SW_HIDE);
        break;
    case 7:
        ::SetWindowLong(m_FileMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_FileMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_ScanMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_ScanMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_ViewMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_ViewMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_CutMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_CutMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_MeasureMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_MeasureMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_CompareMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_CompareMenu->ShowWindow(SW_HIDE);
        ::SetWindowLong(m_AnalysisMenu->m_hWnd, GWL_ID, m_wndSplitterWnd.IdFromRowCol(0, 0));
        m_AnalysisMenu->ShowWindow(SW_SHOW);
        ::SetWindowLong(m_FeatureMenu->m_hWnd, GWL_ID, 0xFFFF);
        m_FeatureMenu->ShowWindow(SW_HIDE);
        break;
    }

    m_wndSplitterWnd.RecalcLayout();
}



//以下为一级菜单响应函数
void CMainFrame::OnFileOpen()
{
    // TODO: 在此添加命令处理程序代码
    UpdateMenu(0);
}

void CMainFrame::OnScan()
{
    // TODO: 在此添加命令处理程序代码
    UpdateMenu(1);
}


void CMainFrame::OnEdit()
{
    // TODO: 在此添加命令处理程序代码
    //UpdateMenu(3);
}


void CMainFrame::OnMeasure()
{
    // TODO: 在此添加命令处理程序代码
    //UpdateMenu(5);
}


void CMainFrame::OnCompare()
{
    // TODO: 在此添加命令处理程序代码
    UpdateMenu(6);
}


void CMainFrame::OnAnalysis()
{
    // TODO: 在此添加命令处理程序代码
    UpdateMenu(7);
}
void CMainFrame::OnReport()
{
    // TODO: 在此添加命令处理程序代码

    //CStl3DLasetView* pview = (CStl3DLasetView*)(((CMainFrame*)AfxGetMainWnd())->m_wndSplitterWnd1.GetPane(0, 1));
    //pview->ActiveMenu = 8;
    //pview->UpdateFunction();//更新
    //SwitchFunction(0);
    //AfxGetMainWnd()->DrawMenuBar(); 
    UpdateMenu(8);
}
void CMainFrame::OnView()
{
    // TODO: 在此添加命令处理程序代码
    UpdateMenu(2);
}
void CMainFrame::Onfeature()
{
    // TODO: 在此添加命令处理程序代码
    UpdateMenu(4);
    //SwitchFunction(1);
}
void CMainFrame::OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    CFrameWndEx::OnDrawItem(nIDCtl, lpDrawItemStruct);
}

CStl3DLasetDoc* CMainFrame::GetDocument()
{
    CStl3DLasetDoc* pDoc;
    switch (GetActiveView())
    {
    case OccView:
        pDoc = (CStl3DLasetDoc*)m_OccView->GetDocument();
        break;
    case StlView:
        pDoc = (CStl3DLasetDoc*)m_MainView->GetDocument();
        break;
    }

    return pDoc;
}
void CMainFrame::UpdateMenu(int menuindex)
{
    CStl3DLasetDoc* pDoc;
    switch (GetActiveView())
    {
    case OccView:
        pDoc = (CStl3DLasetDoc*)m_OccView->GetDocument();
        if (m_OccView->m_drawBias)
            MessageBox("请先停止扫描");
        else
        {
            m_OccView->RemoveCollectedData();
            MenuSwitch(menuindex);
            if (menuindex == 4)
            {
                SwitchFunction(1);
                m_OccView->SetTransparent();
            }
            else if (pDoc->ActiveMenu == 4)
                m_OccView->SetSolid();
        }
        break;
    case StlView:
        MenuSwitch(menuindex);
        pDoc = (CStl3DLasetDoc*)m_MainView->GetDocument();
        break;
    }
    pDoc->ActiveMenu = menuindex;
    AfxGetMainWnd()->DrawMenuBar();
}
//标准软件和3d软件界面切换
void CMainFrame::OnSwitch3D2D()
{
    if (m_b2DIf) {
        // 切换到3D模式
        m_b2DIf = FALSE;
        if (m_GoodVisonDlg.GetSafeHwnd()) {
            m_GoodVisonDlg.ShowWindow(SW_HIDE);
        }
        SwitchView(0); // 显示3D视图
    }
    else {
        // 切换到2D模式
        m_b2DIf = TRUE;
        if (!m_GoodVisonDlg.GetSafeHwnd()) {
            // 创建对话框
            if (m_GoodVisonDlg.Create(IDD_DIALOG_GOOD_VISION, this)) {
                // 初始调整位置
                AdjustGoodVisionDialog();
                m_GoodVisonDlg.NewProcess();
            }
        }
        else {
            // 如果对话框已存在，确保它显示在正确位置
            AdjustGoodVisionDialog();
        }
        m_GoodVisonDlg.ShowWindow(SW_SHOW);
        SwitchView(1); // 隐藏3D视图
    }
}
CRect CMainFrame::GetAdjustedClientRect()
{
    CRect rc;
    GetClientRect(&rc);
    // 只排除应用程序按钮区域
    if (m_wndRibbonBar.IsVisible())
    {
        // 获取应用程序按钮
        CMFCRibbonApplicationButton* pAppButton = m_wndRibbonBar.GetApplicationButton();
        if (pAppButton && pAppButton->IsVisible())
        {
            // 获取应用程序按钮的矩形（在RibbonBar坐标中）
            CRect rectButton = pAppButton->GetRect();
            if (!rectButton.IsRectEmpty())
            {
                // 将按钮位置转换为屏幕坐标
                CRect rectScreen = rectButton;
                m_wndRibbonBar.ClientToScreen(&rectScreen);
                // 将屏幕坐标转换为客户区坐标
                ScreenToClient(&rectScreen);
                // 只排除应用程序按钮区域的高度2
                rc.top += rectScreen.Height();
            }
        }
    }

    return rc;
}

// 在 OnSize 和 OnMove 中增加重绘调用

void CMainFrame::OnWindowPosChanged(WINDOWPOS* lpwndpos)
{
    CFrameWndEx::OnWindowPosChanged(lpwndpos);

    // 当窗口位置或大小改变时调整对话框
    if (!(lpwndpos->flags & (SWP_NOMOVE | SWP_NOSIZE)))
    {
        AdjustGoodVisionDialog();
    }
}
void CMainFrame::AdjustGoodVisionDialog()
{
    if (m_b2DIf && m_GoodVisonDlg.GetSafeHwnd() && ::IsWindow(m_GoodVisonDlg.m_hWnd))
    {
        // 获取调整后的客户区矩形
        CRect rc = GetAdjustedClientRect();

        // 转换为屏幕坐标
        ClientToScreen(&rc);

        // 设置对话框位置和大小
        m_GoodVisonDlg.SetWindowPos(
            NULL,
            rc.left,
            rc.top,
            rc.Width(),
            rc.Height(),
            SWP_NOZORDER | SWP_NOACTIVATE
        );

        // 确保对话框重绘
        m_GoodVisonDlg.RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
    }
}
void CMainFrame::OnApplicationLook(UINT id)
{
    CWaitCursor wait;

    // 修改：使用 AfxGetApp() 获取应用实例
    CWinApp* pApp = AfxGetApp();
    if (!pApp) return;

    // 保存当前外观设置（如果需要）
    // pApp->m_nAppLook = id;

    switch (id)
    {
    case ID_VIEW_APPLOOK_WIN_2000:
        CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManager));
        m_wndRibbonBar.SetWindows7Look(FALSE);
        break;

    case ID_VIEW_APPLOOK_OFF_XP:
        CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOfficeXP));
        m_wndRibbonBar.SetWindows7Look(FALSE);
        break;

    case ID_VIEW_APPLOOK_WIN_XP:
        CMFCVisualManagerWindows::m_b3DTabsXPTheme = TRUE;
        CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerWindows));
        m_wndRibbonBar.SetWindows7Look(FALSE);
        break;

    case ID_VIEW_APPLOOK_OFF_2003:
        CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2003));
        CDockingManager::SetDockingMode(DT_SMART);
        m_wndRibbonBar.SetWindows7Look(FALSE);
        break;

    default:
        switch (id)
        {
        case ID_VIEW_APPLOOK_OFF_2007_BLUE:
            CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_LunaBlue);
            break;

        case ID_VIEW_APPLOOK_OFF_2007_BLACK:
            CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_ObsidianBlack);
            break;

        case ID_VIEW_APPLOOK_OFF_2007_SILVER:
            CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Silver);
            break;

        case ID_VIEW_APPLOOK_OFF_2007_AQUA:
            CMFCVisualManagerOffice2007::SetStyle(CMFCVisualManagerOffice2007::Office2007_Aqua);
            break;
        }

        CMFCVisualManager::SetDefaultManager(RUNTIME_CLASS(CMFCVisualManagerOffice2007));
        CDockingManager::SetDockingMode(DT_SMART);
        m_wndRibbonBar.SetWindows7Look(FALSE);
    }

    RedrawWindow(nullptr, nullptr, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW | RDW_FRAME | RDW_ERASE);

    // 如果需要保存设置
    // if (pApp) {
    //     pApp->WriteInt(_T("ApplicationLook"), id);
    // }
}
void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
    CFrameWndEx::OnSize(nType, cx, cy);
    AdjustGoodVisionDialog();
}

void CMainFrame::OnMove(int x, int y)
{
    CFrameWndEx::OnMove(x, y);
    AdjustGoodVisionDialog();
}


void CMainFrame::OnViewCaptionBar()
{
    m_wndCaptionBar.ShowWindow(m_wndCaptionBar.IsVisible() ? SW_HIDE : SW_SHOW);
    RecalcLayout(FALSE);
}

void CMainFrame::OnUpdateViewCaptionBar(CCmdUI* pCmdUI)
{
    pCmdUI->SetCheck(m_wndCaptionBar.IsVisible());
}
#include "afxribboncustomizedialog.h"  // 包含Ribbon自定义对话框类
#include "WaferSetDlg.h"
void CMainFrame::OnOptions()
{
    CMFCRibbonCustomizeDialog* pOptionsDlg = new CMFCRibbonCustomizeDialog(this, &m_wndRibbonBar);
    ASSERT(pOptionsDlg != nullptr);

    pOptionsDlg->DoModal();
    delete pOptionsDlg;
}
// 在CMainFrame的OnClose方法中添加
void CMainFrame::OnClose()
{
    /*CFrameWndEx::OnClose();*/
    //ShowWindow(SW_HIDE);
    DestroyWindow();
}

void CMainFrame::PostNcDestroy()
{
    CWinApp* pApp = AfxGetApp();
    if (pApp && pApp->m_pMainWnd == this)
        pApp->m_pMainWnd = nullptr;

    m_OccView = nullptr; m_MainView = nullptr; m_FileMenu = nullptr; m_ScanMenu = nullptr; m_ViewMenu = nullptr; m_CutMenu = nullptr;
        m_MeasureMenu = nullptr; m_CompareMenu = nullptr; m_AnalysisMenu = nullptr; m_FeatureMenu = nullptr;

    CFrameWndEx::PostNcDestroy(); 
}

void CMainFrame::OnZoom4x()
{
    // 在某个按钮点击事件或初始化中调用
    int targetZoomIndex = 1; // 假设m_zoomLevels中索引1对应2x变倍
    COccView* pOccView = m_OccView;
    //pOccView->Zoom(targetZoomIndex); // 切换变倍
}
void CMainFrame::OnZoom2x()
{
    // 在某个按钮点击事件或初始化中调用
    int targetZoomIndex = 2; // 假设m_zoomLevels中索引1对应2x变倍
    COccView* pOccView = m_OccView;
    //	pOccView->Zoom(targetZoomIndex); // 切换变倍
}
// CMainFrame::OnButtonReport1 实现：调用 OpenFileView 的 OnButtonReport
void CMainFrame::OnButtonReport1()
{
    // 1. 检查 OpenFileView 实例（m_FileMenu）是否有效，避免空指针访问
    if (m_FileMenu != nullptr && ::IsWindow(m_FileMenu->GetSafeHwnd()))
    {
        // 2. 调用 OpenFileView 的 OnButtonReport（触发PDF报告生成）
        m_FileMenu->OnButtonReport();
    }
    else
    {
        // 3. 若实例无效，提示用户（可选，增强容错性）
        AfxMessageBox(_T("文件视图未初始化，无法生成报告！"), MB_ICONWARNING);
    }
}
// CMainFrame::OnButtonReport1m 实现：调用 OpenFileView 的 OnButtonReport2（面轮廓度
void CMainFrame::OnButtonReport1m()
{
    // 1. 检查 OpenFileView 实例（m_FileMenu）是否有效，避免空指针访问
    if (m_FileMenu != nullptr && ::IsWindow(m_FileMenu->GetSafeHwnd()))
    {
        // 2. 调用 OpenFileView 的 OnButtonReport（触发PDF报告生成）
        m_FileMenu->OnButtonReport();
    }
    else
    {
        // 3. 若实例无效，提示用户（可选，增强容错性）
        AfxMessageBox(_T("文件视图未初始化，无法生成报告！"), MB_ICONWARNING);
    }
}
// 在 CMainFrame 类的头文件中添加
CString m_strPDFSavePath;

// 在 CMainFrame 类的实现文件中修改 OnButtonsaveml 函数
void CMainFrame::OnButtonsaveml()
{
    WaferSetDlg dlg;
    dlg.DoModal();
    return;

    // 1. 检查是否已选择过目录：若已选择，询问用户是否重新修改
    if (!m_strReportSavePath.IsEmpty())
    {
        CString strMsg;
        strMsg.Format(_T("当前报告保存目录：\n%s\n是否需要重新选择目录？"),
            m_strReportSavePath);

        // 弹出确认框：YES=重新选择，NO=取消
        int nRet = MessageBox(strMsg, _T("目录已存在"),
            MB_YESNO | MB_ICONINFORMATION | MB_TOPMOST);
        if (nRet == IDNO)
        {
            return;  // 用户不修改，直接返回
        }
    }

    // 2. 使用传统API实现文件夹选择（兼容所有MFC版本）
    TCHAR szPath[MAX_PATH] = { 0 };
    BROWSEINFO bi = { 0 };
    bi.hwndOwner = m_hWnd;  // 父窗口句柄
    bi.lpszTitle = _T("选择报告保存目录（仅需选择一次）");  // 对话框标题
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;  // 只允许选择文件系统目录

    // 3. 显示文件夹选择对话框
    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
    if (pidl != NULL)
    {
        // 4. 将选择的目录转换为字符串路径
        if (SHGetPathFromIDList(pidl, szPath))
        {
            m_strReportSavePath = szPath;
            // 去除路径末尾可能存在的反斜杠
            if (m_strReportSavePath.Right(1) == _T("\\"))
            {
                m_strReportSavePath.TrimRight(_T("\\"));
            }

            CStl3DLasetDoc* pDoc = GetDocument();
            pDoc->SetPDFPath(m_strReportSavePath);
            // 5. 提示用户目录已保存
            CString strTip;
            strTip.Format(_T("报告保存目录已设置：\n%s\n后续生成报告将自动使用此目录！"),
                m_strReportSavePath);
            MessageBox(strTip, _T("目录设置成功"), MB_OK | MB_ICONINFORMATION | MB_TOPMOST);
        }
        // 释放内存
        CoTaskMemFree(pidl);
    }
}


void CMainFrame::OnToggleGrid()
{
    if (!m_OccView || !::IsWindow(m_OccView->GetSafeHwnd()))
        return;

    if (activeindex != OccView)
        SwitchView(0);

    m_OccView->CycleViewerGrid(10.0);
}