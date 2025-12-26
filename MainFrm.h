// MainFrm.h : CMainFrame 类的接口
//
#pragma once  // 应该放在文件顶部

#include <afxribbonbar.h>
#include <afxribbonstatusbar.h>
#include "Stl3DLasetDoc.h"
#include "Stl3DLasetView.h"
#include "OpenFileView.h"
#include "ShowView.h"
#include "AnalysisView.h"
#include "CompareView.h"
#include "MeasureView.h"
#include "TopView.h"
#include "ScanView.h"
#include "feature.h"
#include"Fileview.h"
#include"Attribute.h"
#include"AngleView.h"
#include"CMenuEx.h"
#include"build3plane.h"
#include"BlankView.h"
#include"AddCoord.h"
#include "CGoodVisionDlg.h"
#include "COccView.h"
#include"CMySplitterWnd.h"
#include <afxbasetabctrl.h>
#include"CProcedureView.h"
#include "SelectedFeature.h"
#include <afxoutlookbar.h>
#include <afxcaptionbar.h>
#include "resource.h"
#include <afxframewndex.h>
#include <afxshelltreectrl.h>
//#include "MFCLibrary3.h"  // 确保包含定义MFCLIBRARY3_API的头文件
//#includeCOccView.h"
// 放在 MainFrm.h 中

enum ActiveView
{
	OccView = 0, StlView
};
#pragma once
class FeatureCache {
public:
	// Define the copy assignment operator
	FeatureCache& operator=(const FeatureCache& other) {
		if (this != &other) {
			// Copy the necessary members from 'other' to 'this'
			// Example:
			// this->member = other.member;
		}
		return *this;
	}

	// Other members of FeatureCache...
};
class COutlookBar : public CMFCOutlookBar
{
	virtual BOOL AllowShowOnPaneMenu() const { return TRUE; }
	virtual void GetPaneName(CString& strName) const { BOOL bNameValid = strName.LoadString(IDS_OUTLOOKBAR); ASSERT(bNameValid); if (!bNameValid) strName.Empty(); }
};
class CMainFrame : public CFrameWndEx
{
public: // 仅从序列化创建
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)
	//	afx_msg void OnUpdateApplicationLook(CCmdUI* pCmdUI);
		//CMFCRibbonBar m_wndRibbonBar;
	CMFCRibbonCategory* m_pCategoryFeature;
	CMFCRibbonCategory* m_pCategoryHome;
	CMFCRibbonCategory* m_pCategorysm;
	CMFCRibbonCategory* m_pCategoryst;
	CMFCRibbonCategory* m_pCategorycl;
	CMFCRibbonCategory* m_pCategoryqh;
	CMFCRibbonCategory* m_pCategorybg;
	// 属性
public:
	void UpdateCaptionBarText(); // 更新标题栏文本
	CSplitterWnd m_wndSplitterH;  // 水平分割器
	CSplitterWnd m_wndSplitterV;  // 垂直分割器
	CString m_currentFileName; // 当前打开的文件名
	CGreenBtn m_BtnCoordinate321;
	afx_msg
		//	void OnButtonCoordinate32();
		CMFCRibbonButton m_btnOpen3D;   // 导入3D文件按钮
	CMFCRibbonButton m_btnOpenScan; // 导入扫描文件按钮
	CMFCRibbonBar     m_wndRibbonBar;
	CMFCRibbonStatusBar  m_wndStatusBar;
	//CMFCToolBar m_wndToolBar; // 声明工具栏成员变量（现代 MFC 推荐使用 CMFCToolBar）
	// 操作
public:
	COutlookBar       m_wndNavigationBar;
	// 重写
public:
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	//void OnWindowManager();
	//void OnApplicationLook(UINT id);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//void SwitchFeatureViewMode(int mode);
	// 声明导航栏和树控件
	CMFCOutlookBar m_wndOutlookBar;  // 成员变量声明
	CMFCShellTreeCtrl m_treeCtrl;  // 添加这一行
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	//	afx_msg LRESULT OnNcHitTest(CPoint point);

	BOOL CreateOutlookBar(CMFCOutlookBar& bar, CMFCShellTreeCtrl& tree, UINT uiID, int nInitialWidth);
	//BOOL CreateOutlookBar(CMFCOutlookBar& bar, UINT uiID, int nInitialWidth);
	//BOOL CMainFrame::CreateOutlookBar(CMFCOutlookBar& bar, CMFCShellTreeCtrl& tree, UINT uiID, int nInitialWidth);
	CMFCCaptionBar    m_wndCaptionBar;
	//BOOL CreateOutlookBar(CMFCOutlookBar& bar, UINT uiID, int nInitialWidth);
	CMFCShellTreeCtrl m_wndTree;
	// MainFrm.h
	void CMainFrame::RemEntity();
	CTreeCtrl m_Tree;
	CDockablePane m_wnd2DPane;  // 停靠窗格
	//	CGoodVisionDlg m_GoodVisonDlg; // 2D界面对话框
	BOOL CreateCaptionBar();

	// 实现
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	void SyncFeatureSelection(const CString& featureName);
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // 控件条嵌入成员
	//CStatusBar  m_wndStatusBar;
	//CToolBar    m_wndToolBar;
	//CMFCMenuBar m_wndmenuBar;
	CStl3DLasetView* m_pView1;
	CStl3DLasetView* m_pView2;
	//CIndexTreeView* m_CoordView;

public:
	//子视图
	AddCoord* m_AddCoordView;
	//build3plane* m_CoordView;
	feature* m_FeatureMenu;
	SelectedFeature* m_FeatureView;
	OpenFileView* m_FileMenu;
	ScanView* m_ScanMenu;
	ShowView* m_ViewMenu;
	TopView* m_CutMenu;
	MeasureView* m_MeasureMenu;
	CompareView* m_CompareMenu;
	AnalysisView* m_AnalysisMenu;
	AngleView* m_AngleView;
	BlankView* m_BlankView;
	CStl3DLasetView* m_MainView;
	COccView* m_OccView;
	CProcedureView* m_ProcedureView;
	CoordinateXYZDlg m_coordinateXYZDlg;
protected:


	// Tree control data


public:
	void SwitchView(UINT id);
	//获取正在显示的视图类型
	ActiveView GetActiveView();
	//分割子窗口
	CMySplitterWnd m_wndSplitterWnd;
	CMySplitterWnd m_wndSplitterWnd1;
	CMySplitterWnd m_wndSplitterWnd2;
	CoordinateDlg m_coordinateDlg;
	// CoordinateXYZDlg m_coordinateXYZDlg;
	Coordinate3PlaneDlg m_coordinate3PlaneDlg;
private:
	CMenuEx m_Menu;
	CGoodVisionDlg m_GoodVisonDlg;
	BOOL m_b2DIf;
	ActiveView activeindex;
	feature* m_pFeature;
	feature m_FeatureM;
	// 生成的消息映射函数
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	void CMainFrame::OnTreeCtrlRClick(NMHDR* pNMHDR, LRESULT* pResult);
	CString m_strCurrentFileName;
	void OnCoordinateplane();

	void OnCoordinateplane1();
	void OnCoordinateplane2();

	void OnBuildPlane1();

	void OnBuildLine();

	void OnBuildPoint();

	void OnButtonplane1();


	DECLARE_MESSAGE_MAP()
	virtual BOOL OnCreateClient(LPCREATESTRUCT lpcs, CCreateContext* pContext);
	// 在头文件中声明

	//小按钮
	CGreenBtn m_BtnPlane;
	CGreenBtn m_BtnCurve;
	CGreenBtn m_BtnVertex;
	//CGreenBtn m_BtnCoordinate321;
	CGreenBtn m_BtnCoordinateXYZ;
	CGreenBtn m_BtnCoordinate3Plane;
	CGreenBtn m_BtnAddCoord;

	CGreenBtn m_BtnBuildPlane;
	CGreenBtn m_BtnBuildLine;
	CGreenBtn m_BtnBuildVertex;

	AddCoordDlg m_addcoordDlg;
	//CoordinateDlg m_coordinateDlg;
	//CoordinateXYZDlg m_coordinateXYZDlg;
	//Coordinate3PlaneDlg m_coordinate3PlaneDlg;
	BuildVertexDlg m_buildVertexDlg;
	BuildLineDlg m_buildLineDlg;
	BuildPlaneDlg m_buildPlaneDlg;
public:
	CComboBox m_comboxPlane1, m_comboxPlane2, m_comboxChoice, m_comboxPoint, m_comboxLine;
	//occ/stl视图切换
	void Switch(int nIndex);
	//左侧窗口视图切换
	void SwitchFunction(int functionnum);
	//菜单栏切换
	void MenuSwitch(int menuindex);
	//void OnApplicationLook(UINT id);
	afx_msg void OnFileOpen();
	afx_msg void OnScan();
	afx_msg void OnEdit();
	afx_msg void OnMeasure();
	afx_msg void OnCompare();
	afx_msg void OnAnalysis();
	afx_msg void OnReport();
	afx_msg void OnView();
	afx_msg void Onfeature();
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDrawItemStruct);
	void UpdateMenu(int menuindex);
	//void UpdateMenu(int menuindex);
	//void UpdateMenu(UINT nID);
	//afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	//afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	//标准软件和3d切换
	afx_msg void OnSwitch3D2D();
	CRect GetAdjustedClientRect();
	//LRESULT OnUpdateTreeControl(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnMove(int x, int y);
	BOOL m_bFeatureViewVisible;
	BOOL m_bFileViewVisible;
	void AdjustGoodVisionDialog();

	void OnApplicationLook(UINT id);

	afx_msg void OnWindowPosChanged(WINDOWPOS* lpwndpos);
	void OnViewCaptionBar();
	void OnUpdateViewCaptionBar(CCmdUI* pCmdUI);
	void OnOptions();
	CView* m_pActiveView;
	void SetActiveView(CView* pView) { m_pActiveView = pView; }
	void CMainFrame::OnClose();
	void OnZoom4x();
	void OnZoom2x();
	void CMainFrame::OnButtonReport1();
    void OnButtonReport1m();
    void OnButtonsaveml();
	CString m_strReportSavePath;
	CString GetReportSavePath() const { return m_strReportSavePath; }//void OnBnClickedButtonSelectTarget();
	CStl3DLasetDoc* GetDocument();
    BOOL m_bAutoDelete = FALSE;
    virtual void PostNcDestroy();

protected:
    bool m_bGridOn = true;
    double m_gridStep = 10.0;

    afx_msg void OnToggleGrid();

};




