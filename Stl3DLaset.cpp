// Stl3DLaset.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include "Stl3DLaset.h"
#include "MainFrm.h"
#include "ModelImport.h"
#include "Stl3DLasetDoc.h"
#include "Stl3DLasetView.h"
#include "COccView.h"
#include <GdiPlus.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <cstring>
#include <strsafe.h>
using namespace Gdiplus;
static AFX_MODULE_STATE* g_pDllModuleState = NULL;
//#ifdef _DEBUG
//#define new DEBUG_NEW
//#endif
// 全局变量，用于存储GDI+令牌和图形驱动
ULONG_PTR gdiplusToken = 0;
Handle(OpenGl_GraphicDriver) m_GraphicDriver;

// CStl3D

// CStl3DLasetApp

BEGIN_MESSAGE_MAP(CStl3DLasetApp, CWinApp)
    ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
    // 基于文件的标准文档命令
    ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
    ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
    // 标准打印设置命令
    ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()


// CStl3DLasetApp 构造

CStl3DLasetApp::CStl3DLasetApp()
{
    // TODO: 在此处添加构造代码，
    // 将所有重要的初始化放置在 InitInstance 中
    try {
        Handle(Aspect_DisplayConnection) aDisplayConnection = new Aspect_DisplayConnection();
        m_GraphicDriver = new OpenGl_GraphicDriver(aDisplayConnection);
    }
    catch (Standard_Failure) {
        AfxMessageBox(_T("(Error Ocured in Initializing the Opencascade graphic variable.)"));
    }
}

// 唯一的一个 CStl3DLasetApp 对象
CStl3DLasetApp theApp;

// CStl3DLasetApp 初始化
BOOL CStl3DLasetApp::InitInstance()
{
    g_pDllModuleState = AfxGetModuleState();


    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    InitCommonControls();
    CWinApp::InitInstance();

    GdiplusStartupInput gdipluwStartupInput;
    GdiplusStartup(&gdiplusToken, &gdipluwStartupInput, NULL);

    if (!AfxOleInit())
    {
        AfxMessageBox(IDP_OLE_INIT_FAILED);
        return FALSE;
    }
    AfxEnableControlContainer();
    return TRUE;
}
CStl3DLasetApp::~CStl3DLasetApp()
{
    //GdiplusShutdown(gdiplusupToken);
}
// 用于应用程序“关于”菜单项的 CAboutDlg 对话框
class CAboutDlg : public CDialog
{
public:
    CAboutDlg();

    // 对话框数据
    enum { IDD = IDD_ABOUTBOX };

protected:
    virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
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
// 用于运行对话框的应用程序命令
void CStl3DLasetApp::OnAppAbout()
{
    //CAboutDlg aboutDlg;
    //aboutDlg.DoModal();
}
// CStl3DLasetApp::ExitInstance（修正后）

int CStl3DLasetApp::ExitInstance()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    // 先隐藏主窗口
    if (m_pMainWnd && ::IsWindow(m_pMainWnd->GetSafeHwnd())) {
        m_pMainWnd->ShowWindow(SW_HIDE);
    }

    // 然后释放资源
    if (!m_GraphicDriver.IsNull()) {
        m_GraphicDriver.Nullify();
    }

    if (gdiplusToken != 0) {
        GdiplusShutdown(gdiplusToken);
        gdiplusToken = 0;
    }

    AfxOleTerm(FALSE);
    return CWinApp::ExitInstance();
}
// 1. 原有显示窗口的函数（保持void返回类型）
// SZZR_2D3D_DLL的导出函数（修正后）
//extern "C" MYMFCDLL1_API void ShowDllMDI(CWnd* pParentWnd)
//{
//    AFX_MANAGE_STATE(AfxGetStaticModuleState());
//    CStl3DLasetApp* pApp = (CStl3DLasetApp*)AfxGetApp();
//    if (!pApp)
//    {
//        AfxMessageBox(_T("获取DLL应用实例失败！"));
//        return;
//    }
//
//    // 1. 检查是否已创建文档模板（避免重复创建）
//    if (pApp->GetFirstDocTemplatePosition() == NULL)
//    {
//        // 注册文档模板（仅在第一次调用时注册）
//        CSingleDocTemplate* pDocTemplate = new CSingleDocTemplate(
//            IDR_MAINFRAME,
//            RUNTIME_CLASS(CStl3DLasetDoc),
//            RUNTIME_CLASS(CMainFrame), // 框架窗口
//            RUNTIME_CLASS(CStl3DLasetView) // 视图
//        );
//        if (!pDocTemplate)
//        {
//            AfxMessageBox(_T("创建文档模板失败！"));
//            return;
//        }
//        pApp->AddDocTemplate(pDocTemplate);
//    }
//
//    // 2. 检查是否已创建主窗口（避免重复创建）
//    if (!pApp->m_pMainWnd || !::IsWindow(pApp->m_pMainWnd->GetSafeHwnd()))
//    {
//        // 创建空文档（触发文档、框架、视图的初始化）
//        CCommandLineInfo cmdInfo;
//        cmdInfo.m_nShellCommand = CCommandLineInfo::FileNew;
//        pApp->ProcessShellCommand(cmdInfo); // 标准文档创建流程
//
//        // 设置主窗口父句柄（可选，绑定到调用端窗口）
//        if (pParentWnd && ::IsWindow(pParentWnd->GetSafeHwnd()))
//        {
//            pApp->m_pMainWnd->SetParent(pParentWnd);
//        }
//
//        // 显示主窗口
//        pApp->m_pMainWnd->SetWindowText(_T("Gview3D"));
//        pApp->m_pMainWnd->ShowWindow(SW_SHOWMAXIMIZED);
//        pApp->m_pMainWnd->UpdateWindow();
//    }
//    else
//    {
//        // 若已创建，直接激活窗口
//        pApp->m_pMainWnd->SetForegroundWindow();
//    }
//}
//
//extern "C" MYMFCDLL1_API void* GetDllOccView()
//{
//    AFX_MANAGE_STATE(AfxGetStaticModuleState());
//
//    CWinApp* pApp = AfxGetApp();
//    if (!pApp || !pApp->m_pMainWnd || !::IsWindow(pApp->m_pMainWnd->GetSafeHwnd()))
//    {
//        AfxMessageBox(_T("主窗口未初始化！"));
//        return nullptr;
//    }
//
//    // 转换为主窗口类（已获取pApp->m_pMainWnd，直接使用即可）
//    CMainFrame* pMainFrame = dynamic_cast<CMainFrame*>(pApp->m_pMainWnd);
//    if (!pMainFrame)
//    {
//        AfxMessageBox(_T("主窗口类型转换失败！"));
//        return nullptr;
//    }
//
//    // 1. 获取分割窗口的(0,1)面板，先检查面板是否有效
//    CWnd* pPane = pMainFrame->m_wndSplitterWnd1.GetPane(0, 1);
//    if (!pPane || !::IsWindow(pPane->GetSafeHwnd()))
//    {
//        AfxMessageBox(_T("分割窗口面板(0,1)无效！"));
//        return nullptr;
//    }
//
//    // 2. 安全转换为COccView（使用dynamic_cast检查类型）
//    COccView* pOccView = dynamic_cast<COccView*>(pPane);
//    if (!pOccView)  // 仅当转换失败时才提示错误
//    {
//        AfxMessageBox(_T("获取COccView实例失败！面板(0,1)不是COccView类型。"));
//        return nullptr;
//    }
//
//    return pOccView;
//}
// CloseDll 函数（修正后）
// DLL 中 Stl3DLaset.cpp 的 CloseDll 实现
extern "C" MYMFCDLL1_API void __stdcall CloseDll()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState()); // 切换到 DLL 模块状态
    CStl3DLasetApp* pApp = dynamic_cast<CStl3DLasetApp*>(AfxGetApp());
    if (!pApp) return;

    // 1. 销毁 DLL 主窗口（关键：避免窗口资源泄漏）
    if (pApp->m_pMainWnd && ::IsWindow(pApp->m_pMainWnd->GetSafeHwnd()))
    {
        pApp->m_pMainWnd->SendMessage(WM_CLOSE); // 发送关闭消息
        MSG msg;
        while (::PeekMessage(&msg, pApp->m_pMainWnd->GetSafeHwnd(), 0, 0, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        pApp->m_pMainWnd = nullptr;
    }

    // 释放文档模板
    POSITION pos
        = pApp->GetFirstDocTemplatePosition();
    while (pos != NULL)
    {
        CDocTemplate
            * pTemplate = pApp->GetNextDocTemplate(pos);
        delete pTemplate;
    }

    // 2. 释放其他资源（OCC 驱动、文档模板等，根据实际情况补充）
    if (!pApp->m_GraphicDriver.IsNull())
    {
        pApp->m_GraphicDriver.Nullify();
        pApp->m_GraphicDriver = nullptr;
    }
}// 4. 修正后的窗口过程
// 整合 ShowDllMDI 与 Show3DWindow 功能：初始化MFC文档框架 + 调整窗口样式并显示
extern "C" MYMFCDLL1_API HANDLE __stdcall Show3DWindow2(HANDLE fatherHwnd)
{
    // 1. 切换DLL模块状态（MFC DLL必须调用，确保资源正确加载）
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    // 2. 验证父窗口句柄有效性
    HWND hwndParent = reinterpret_cast<HWND>(fatherHwnd);
    if (!::IsWindow(hwndParent))
    {
        AfxMessageBox(_T("无效的父窗口句柄！"));
        return nullptr;
    }

    // 3. 获取DLL应用程序实例（核心：确保操作当前DLL的App对象）
    CStl3DLasetApp* pApp = dynamic_cast<CStl3DLasetApp*>(AfxGetApp());
    if (!pApp)
    {
        AfxMessageBox(_T("获取DLL应用实例失败！"));
        return nullptr;
    }

    // 4. 初始化MFC文档模板（仅首次调用时创建，避免重复注册）
    if (pApp->GetFirstDocTemplatePosition() == NULL)
    {
        CSingleDocTemplate* pDocTemplate = new CSingleDocTemplate(
            IDR_MAINFRAME,                  // 资源ID（菜单、图标等）
            RUNTIME_CLASS(CStl3DLasetDoc),  // 文档类
            RUNTIME_CLASS(CMainFrame),      // 框架窗口类（含分割窗口）
            RUNTIME_CLASS(CStl3DLasetView)  // 视图类（OCC 3D渲染）
        );

        if (!pDocTemplate)
        {
            AfxMessageBox(_T("创建MFC文档模板失败！"));
            return nullptr;
        }

        // 将文档模板添加到App，完成注册
        pApp->AddDocTemplate(pDocTemplate);
    }

    // 5. 初始化主窗口（框架+视图）：仅当窗口未创建/无效时执行
    if (!pApp->m_pMainWnd || !::IsWindow(pApp->m_pMainWnd->GetSafeHwnd()))
    {
        // 配置"新建文档"命令（触发文档/框架/视图的创建流程）
        CCommandLineInfo cmdInfo;
        cmdInfo.m_nShellCommand = CCommandLineInfo::FileNew;

        // 执行命令：MFC标准流程，创建文档→框架→视图
        if (!pApp->ProcessShellCommand(cmdInfo))
        {
            AfxMessageBox(_T("创建3D主窗口（文档/框架/视图）失败！"));
            pApp->m_pMainWnd = nullptr; // 重置无效窗口
            return nullptr;
        }

        // 初始化窗口基础属性
        pApp->m_pMainWnd->SetWindowText(_T("Gview3D")); // 窗口标题
    }

    // 6. 确保主窗口句柄有效（后续样式调整依赖）
    HWND hMainWnd = pApp->m_pMainWnd->GetSafeHwnd();
    if (!::IsWindow(hMainWnd))
    {
        AfxMessageBox(_T("3D主窗口句柄无效！"));
        return nullptr;
    }

    // 7. 调整主窗口样式：从"独立窗口"转为"子窗口"（适配父容器）
    {
        // 获取当前窗口样式与扩展样式
        DWORD dwStyle = ::GetWindowLongPtr(hMainWnd, GWL_STYLE);
        DWORD dwExStyle = ::GetWindowLongPtr(hMainWnd, GWL_EXSTYLE);

        // 移除"独立窗口"样式（弹窗、标题栏、边框、最大化/最小化按钮等）
        const DWORD dwRemoveStyle = WS_POPUP | WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME |
            WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
        // 添加"子窗口"必要样式（嵌入父窗口、裁剪子控件等）
        const DWORD dwAddStyle = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

        // 移除"独立窗口"扩展样式（窗口边缘、任务栏图标等）
        const DWORD dwRemoveExStyle = WS_EX_APPWINDOW | WS_EX_OVERLAPPEDWINDOW |
            WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE;

        // 应用新样式
        ::SetWindowLongPtr(hMainWnd, GWL_STYLE, (dwStyle & ~dwRemoveStyle) | dwAddStyle);
        ::SetWindowLongPtr(hMainWnd, GWL_EXSTYLE, dwExStyle & ~dwRemoveExStyle);

        // 移除窗口菜单（子窗口无需独立菜单）
        ::SetMenu(hMainWnd, NULL);
    }

    // 8. 绑定父窗口+调整大小（适配父容器客户区）
    {
        // 确保父窗口绑定正确
        if (::GetParent(hMainWnd) != hwndParent)
        {
            ::SetParent(hMainWnd, hwndParent);
        }

        // 刷新窗口样式（必须调用，否则样式修改不生效）
        ::SetWindowPos(hMainWnd, NULL, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);

        // 获取父窗口客户区大小，让3D窗口填满父容器
        RECT rcParentClient = { 0 };
        ::GetClientRect(hwndParent, &rcParentClient);
        int nParentWidth = rcParentClient.right - rcParentClient.left;
        int nParentHeight = rcParentClient.bottom - rcParentClient.top;

        // 调整3D窗口大小并显示
        ::SetWindowPos(hMainWnd, NULL, 0, 0, nParentWidth, nParentHeight,
            SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);

        // 通知所有子窗口（如OCC视图）调整大小，避免渲染错位
        ::EnumChildWindows(hMainWnd,
            [](HWND hChildWnd, LPARAM lParam) -> BOOL
            {
                RECT rcChildClient = { 0 };
                ::GetClientRect(hChildWnd, &rcChildClient);
                // 发送WM_SIZE消息，触发子窗口重绘与布局
                ::SendMessage(hChildWnd, WM_SIZE, SIZE_RESTORED,
                    MAKELPARAM(rcChildClient.right - rcChildClient.left,
                        rcChildClient.bottom - rcChildClient.top));
                return TRUE;
            }, 0);
    }

    // 9. 激活3D窗口（确保用户操作焦点）
    ::BringWindowToTop(hMainWnd);
    ::SetFocus(hMainWnd);
    ::UpdateWindow(hMainWnd);

    // 10. 返回3D主窗口句柄（供调用端后续操作）
    return reinterpret_cast<HANDLE>(hMainWnd);
}

// 【废弃】原 ShowDllMDI 函数（功能已整合到 Show3DWindow，可删除或注释）
/*
extern "C" MYMFCDLL1_API void ShowDllMDI(CWnd* pParentWnd)
{
     功能已迁移至 Show3DWindow，此函数不再使用
    AfxMessageBox(_T("ShowDllMDI 已废弃，请调用 Show3DWindow！"));
}
*/


extern "C" MYMFCDLL1_API int Hide3DWindow2(HANDLE hwnd)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HWND hCtrl = reinterpret_cast<HWND>(hwnd);
    if (!::IsWindow(hCtrl))
        return -1;

    ::ShowWindow(hCtrl, SW_HIDE);
    return 0;
}
 //Stl3DLaset.cpp

// 添加辅助函数实现
COccView* GetCurrentOccViewByWindow()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CWinApp* pApp = AfxGetApp();
    if (!pApp || !pApp->m_pMainWnd || !::IsWindow(pApp->m_pMainWnd->GetSafeHwnd()))
    {
        return nullptr;
    }

    // 转换为主窗口类
    CMainFrame* pMainFrame = dynamic_cast<CMainFrame*>(pApp->m_pMainWnd);
    if (!pMainFrame)
    {
        return nullptr;
    }

    // 获取分割窗口的(0,1)面板
    CWnd* pPane = pMainFrame->m_wndSplitterWnd1.GetPane(0, 1);
    if (!pPane || !::IsWindow(pPane->GetSafeHwnd()))
    {
        return nullptr;
    }

    // 安全转换为COccView
    COccView* pOccView = dynamic_cast<COccView*>(pPane);
    return pOccView;
}
//// Stl3DLaset.cpp
//extern "C" MYMFCDLL1_API double SetSurfaceProfilePoint(double x, double y, double z, int flagBit)
//{
//    AFX_MANAGE_STATE(AfxGetStaticModuleState());
//
//    CWinApp* pApp = AfxGetApp();
//    if (!pApp || !pApp->m_pMainWnd) return -1.0;
//    //if （！pApp || !pApp->m_pMainWnd || !::IsWindow(pApp->m_pMainWnd->GetSafeHwnd())) return -1.0;
//
//    CMainFrame* pMainFrame = dynamic_cast<CMainFrame*>(pApp->m_pMainWnd);
//    if (!pMainFrame) return -1.0;
//
//    CWnd* pPane = pMainFrame->m_wndSplitterWnd1.GetPane(0, 1);
//    COccView* pOccView = dynamic_cast<COccView*>(pPane);
//    if (!pOccView) return -1.0;
//
//    if (flagBit < 0 || flagBit > 1) return -2.0;
//
//    std::lock_guard<std::mutex> lock(pOccView->m_profilePointMutex);
//
//    // ===== 关键修改：如果是新的一组点的第一个点，清除之前的数据 =====
//    static int lastFlagBit = -1; // 静态变量记录上一次的flagBit
//
//    if (flagBit == 0 && lastFlagBit == 1) {
//        // 上一次是结束点(flagBit=1)，这次是开始点(flagBit=0)，需要清除之前的数据
//        pOccView->RemoveCollectedData(); // 清除显示数据
//        pOccView->m_tempProfilePoints.clear(); // 清除临时点集
//        pOccView->m_position.clear(); // 清除计算点集
//        pOccView->m_isCalculated = false;
//        pOccView->m_isDllPointsReady = false;
//        pOccView->m_calculatedTolerance = 0.0;
//    }
//
//    lastFlagBit = flagBit; // 更新记录
//
//    // ===== 原有逻辑继续 =====
//    if (pOccView->m_drawBias) return -3.0;
//
//    gp_Pnt newPoint(x, y, z);
//
//    if (flagBit == 0) {
//        pOccView->m_tempProfilePoints.push_back(newPoint);
//        pOccView->m_isDllPointsReady = false;
//        pOccView->m_isCalculated = false;
//        return 0.0; // 还未完成
//    }
//
//    // === 最后一个点传入 ===
//    pOccView->m_tempProfilePoints.push_back(newPoint);
//    pOccView->m_position = pOccView->m_tempProfilePoints; // 复制点集
//    pOccView->m_isDllPointsReady = true;
//
//    // === 直接计算面轮廓度 ===
//    double profileTolerance = pOccView->SearchNstPoints();
//
//    // === 保存结果 ===
//    pOccView->m_calculatedTolerance = profileTolerance;
//    pOccView->m_isCalculated = true;
//
//    return profileTolerance;
//}
//
//// 修改 GetsurfaceProfile 函数
//extern "C" MYMFCDLL1_API double GetsurfaceProfile()
//{
//    AFX_MANAGE_STATE(AfxGetStaticModuleState());
//
//    // 直接内联窗口查找逻辑
//    CWinApp* pApp = AfxGetApp();
//    if (!pApp || !pApp->m_pMainWnd) return -1.0;
//
//    CMainFrame* pMainFrame = dynamic_cast<CMainFrame*>(pApp->m_pMainWnd);
//    if (!pMainFrame) return -1.0;
//
//    CWnd* pPane = pMainFrame->m_wndSplitterWnd1.GetPane(0, 1);
//    COccView* pOccView = dynamic_cast<COccView*>(pPane);
//    if (!pOccView) return -1.0;
//
//    std::lock_guard<std::mutex> lock(pOccView->m_profilePointMutex);
//
//    if (!pOccView->m_isDllPointsReady)
//    {
//        return -2.0;
//    }
//
//    if (!pOccView->m_isCalculated)
//    {
//        return -3.0;
//    }
//
//    return pOccView->m_calculatedTolerance;
//}
//extern "C" MYMFCDLL1_API int SetProfile(char* filePath, int len)
//{
//    AFX_MANAGE_STATE(AfxGetStaticModuleState());
//    if (!filePath || len <= 0) return -1;
//
//    CStringW pathW(filePath);          // 1. 先转宽字符
//    CString path(pathW);               // 2. 再转 CString
//
//    CMainFrame* pMain = (CMainFrame*)AfxGetMainWnd();
//    if (!pMain || !pMain->m_OccView) return -1;
//
//    try
//    {
//        ((CStl3DLasetDoc*)pMain->GetActiveDocument())->ImportSTEP(path, 0);
//        return 0;
//    }
//    catch (...) { return -1; }
//}


extern pfn3DfileCallback g_3dFileCallback = nullptr;
static std::atomic<bool> g_running{ false };
static std::atomic<bool> g_showWindow{ false };

static std::mutex g_dataMutex;
int tempimage_width = 640, tempimage_height = 480;
int g_changeisdone = 1;
int D_index = 0;
static std::thread g_imgThread;
ModelImport g_modelopened;


//extern "C" MYMFCDLL1_API HANDLE Show3DWindow2(HANDLE fatherHwnd)
//{
//    AFX_MANAGE_STATE(AfxGetStaticModuleState());
//
//    HWND hParent = reinterpret_cast<HWND>(fatherHwnd);
//    if (!::IsWindow(hParent)) return nullptr;
//
//    CWinApp* pApp = AfxGetApp();
//    CWnd* pMainWnd = (pApp ? pApp->m_pMainWnd : nullptr);
//    if (!pMainWnd) return nullptr;
//
//    HWND hMain = pMainWnd->GetSafeHwnd();
//    if (!::IsWindow(hMain)) return nullptr;
//
//    DWORD style = (DWORD)::GetWindowLongPtr(hMain, GWL_STYLE);
//    DWORD exstyle = (DWORD)::GetWindowLongPtr(hMain, GWL_EXSTYLE);
//
//    const DWORD kRemoveStyle =
//        WS_POPUP | WS_OVERLAPPED | WS_CAPTION | WS_THICKFRAME |
//        WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;
//    const DWORD kAddStyle = WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
//
//    const DWORD kRemoveEx =
//        WS_EX_APPWINDOW | WS_EX_OVERLAPPEDWINDOW | WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE;
//
//    style &= ~kRemoveStyle;
//    style |= kAddStyle;
//    exstyle &= ~kRemoveEx;
//
//    ::SetWindowLongPtr(hMain, GWL_STYLE, style);
//    ::SetWindowLongPtr(hMain, GWL_EXSTYLE, exstyle);
//    ::SetMenu(hMain, NULL);
//
//    if (::GetParent(hMain) != hParent) {
//        ::SetParent(hMain, hParent);
//    }
//
//    ::SetWindowPos(hMain, NULL, 0, 0, 0, 0,
//        SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
//
//    RECT rc{}; ::GetClientRect(hParent, &rc);
//    const int W = rc.right - rc.left;
//    const int H = rc.bottom - rc.top;
//
//    ::SetWindowPos(hMain, NULL, 0, 0, W, H,
//        SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE | SWP_SHOWWINDOW);
//    EnumChildWindows(hMain,
//        [](HWND w, LPARAM) -> BOOL {
//            RECT r{}; GetClientRect(w, &r);
//            SendMessage(w, WM_SIZE, SIZE_RESTORED,
//                MAKELPARAM(r.right - r.left, r.bottom - r.top));
//            return TRUE;
//        }, 0);
//
//    ::BringWindowToTop(hMain);
//    ::SetFocus(hMain);
//    ::UpdateWindow(hMain);
//
//    g_showWindow = true;
//    return reinterpret_cast<HANDLE>(hMain);
//}
//extern "C" MYMFCDLL1_API int Hide3DWindow2(HANDLE hwnd)
//{
//    AFX_MANAGE_STATE(AfxGetStaticModuleState());
//
//    HWND hCtrl = reinterpret_cast<HWND>(hwnd);
//    if (!::IsWindow(hCtrl))
//        return -1;
//
//    g_showWindow = false;
//    ::ShowWindow(hCtrl, SW_HIDE);
//    return 0;
//}


//extern "C" MYMFCDLL1_API int Show3DWindow() {
//    AFX_MANAGE_STATE(AfxGetStaticModuleState());
//    CWinApp* pApp = AfxGetApp();
//    if (pApp->m_pMainWnd && ::IsWindow(pApp->m_pMainWnd->GetSafeHwnd()))
//    {
//        g_showWindow = true;
//        pApp->m_pMainWnd->ShowWindow(SW_SHOW);
//        pApp->m_pMainWnd->SetForegroundWindow();
//        pApp->m_pMainWnd->UpdateWindow();
//        return 0;
//    }
//    return -1;
//}

extern "C" MYMFCDLL1_API int Show3DWindow()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CStl3DLasetApp* pApp = dynamic_cast<CStl3DLasetApp*>(AfxGetApp());
    if (!pApp)
        return -2; 

    if (pApp->GetFirstDocTemplatePosition() == NULL)
    {
        CSingleDocTemplate* pDocTemplate = new CSingleDocTemplate(
            IDR_MAINFRAME,
            RUNTIME_CLASS(CStl3DLasetDoc),
            RUNTIME_CLASS(CMainFrame),
            RUNTIME_CLASS(CStl3DLasetView));
        if (!pDocTemplate)
            return -3; 

        pApp->AddDocTemplate(pDocTemplate);
    }
    if (!pApp->m_pMainWnd || !::IsWindow(pApp->m_pMainWnd->GetSafeHwnd()))
    {
        CCommandLineInfo cmdInfo;
        cmdInfo.m_nShellCommand = CCommandLineInfo::FileNew;

        if (!pApp->ProcessShellCommand(cmdInfo))
        {
            pApp->m_pMainWnd = nullptr;
            return -4;
        }
        pApp->m_pMainWnd->SetWindowText(_T("Gview3D"));
    }

    if (pApp->m_pMainWnd && ::IsWindow(pApp->m_pMainWnd->GetSafeHwnd()))
    {
        HWND h = pApp->m_pMainWnd->GetSafeHwnd();

        g_showWindow = true;
        pApp->m_pMainWnd->ShowWindow(SW_SHOWNORMAL);
        pApp->m_pMainWnd->SetForegroundWindow();

        ::InvalidateRect(h, nullptr, FALSE);
        return 0;
    }

    return -1;
}

//extern "C" MYMFCDLL1_API int Hide3DWindow() {
//    AFX_MANAGE_STATE(AfxGetStaticModuleState());
//    CWinApp* pApp = AfxGetApp();
//    if (pApp->m_pMainWnd && ::IsWindow(pApp->m_pMainWnd->GetSafeHwnd()))
//    {
//        g_showWindow = false;
//        pApp->m_pMainWnd->ShowWindow(SW_HIDE);
//        return 0;
//    }
//    return -1;
//}

extern "C" MYMFCDLL1_API int Hide3DWindow()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CWinApp* pApp = AfxGetApp();
    CWnd* pMain = (pApp ? pApp->m_pMainWnd : nullptr);
    if (pMain && ::IsWindow(pMain->GetSafeHwnd()))
    {
        g_showWindow = false;
        pMain->ShowWindow(SW_HIDE);
        ::PostMessage(pMain->GetSafeHwnd(), WM_NULL, 0, 0);
        return 0;
    }
    return -1;
}

extern "C" MYMFCDLL1_API int SetImageCallback(pfnImageCallback callback) {
    g_imageCallback = callback;
    return 0;
}

extern "C" MYMFCDLL1_API int SetPositionCallback(pfnPositionCallback callback) {
    g_positionCallback = callback;
    return 0;
}

extern "C" MYMFCDLL1_API int Set3DfileCallback(pfn3DfileCallback callback) {
    g_3dFileCallback = callback;
    return 0;
}

void SendVirtualImage() {
    while (g_running) {
        std::vector<std::vector<unsigned char>> localCopy;
        int w = 0, h = 0;
        {
            std::lock_guard<std::mutex> lk(g_dataMutex);
            const auto& v = g_modelopened.GetAllViewData();
            if (!v.empty()) localCopy = v;
            w = tempimage_width; h = tempimage_height;
        }

        if (g_changeisdone && g_imageCallback && g_showWindow && !localCopy.empty()) {
            for (size_t idx = 0; idx < localCopy.size(); ++idx) {
                const auto& buf = localCopy[idx];
                g_imageCallback(D_index, w, h, buf.data());
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                if (!g_running) break;
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
}


extern "C" MYMFCDLL1_API int Init(int index, const char* file3DPath)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    g_running = true;
    g_showWindow = true;
    D_index = index;
    std::wstring wsFilePath;
    if (file3DPath && strlen(file3DPath) > 0) {
        int len = MultiByteToWideChar(CP_UTF8, 0, file3DPath, -1, NULL, 0);
        if (len > 0) {
            std::vector<wchar_t> buf(len);
            MultiByteToWideChar(CP_UTF8, 0, file3DPath, -1, buf.data(), len);
            wsFilePath.assign(buf.data());
        }
    }
    CWinApp* pApp = AfxGetApp();
    CMainFrame* pFrame = pApp ? (CMainFrame*)pApp->m_pMainWnd : nullptr;
    if (!pFrame) return 0;

    if (!wsFilePath.empty()) {
        CDocument* pDocBase = pFrame->GetActiveDocument();
        CStl3DLasetDoc* pDoc = DYNAMIC_DOWNCAST(CStl3DLasetDoc, pDocBase);
        if (pDoc) {
#ifdef UNICODE
            pDoc->Load3DData(wsFilePath.c_str());
#else
#include <atlconv.h>
            USES_CONVERSION;
            pDoc->Load3DData(W2A(wsFilePath.c_str()));
#endif
            COccView* pOccView = pFrame->m_OccView;
            if (!pOccView) return 0;

            Handle(AIS_InteractiveContext) context = pOccView->GetAISContext();
            Handle(V3d_Viewer)            viewer = pOccView->GetViewer();
            if (viewer.IsNull() || context.IsNull()) return 0;

            int ret = g_modelopened.ImportExportViews(
                viewer, context,
                640, 480,
                true, 50.0);

            if (ret == 1) {
                const std::vector<std::vector<unsigned char>>& viewData = g_modelopened.GetAllViewData();
                tempimage_width = 640, tempimage_height = 480;
            }
        }
        else { return 0; }
    }
    else
    {
        CDocument* pDocBase = pFrame->GetActiveDocument();
        CStl3DLasetDoc* pDoc = DYNAMIC_DOWNCAST(CStl3DLasetDoc, pDocBase);
        if (pDoc) {
            pDoc->OnFile3Ddata();
        }
        COccView* pOccView = pFrame->m_OccView;
        if (!pOccView) return 0;

        Handle(AIS_InteractiveContext) context = pOccView->GetAISContext();
        Handle(V3d_Viewer)            viewer = pOccView->GetViewer();
        if (viewer.IsNull() || context.IsNull()) return 0;

        int ret = g_modelopened.ImportExportViews(
            viewer, context,
            640, 480,
            true, 50.0);

        if (ret == 1) {
            const std::vector<std::vector<unsigned char>>& viewData = g_modelopened.GetAllViewData();
            tempimage_width = 640, tempimage_height = 480;
        }
    }
    if (!g_imgThread.joinable()) {
        g_imgThread = std::thread(SendVirtualImage);
    }
    return 0;
}

extern "C" MYMFCDLL1_API int InitImage(int image_width, int image_height)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    g_changeisdone = 0;
    int ret = g_modelopened.SetImageSize(image_width, image_height);
    g_images.clear();
    if (ret == 1) {
        const std::vector<std::vector<unsigned char>>& viewData = g_modelopened.GetAllViewData();
    }
    tempimage_width = image_width, tempimage_height = image_height;
    g_changeisdone = 1;
    return 0;
}

extern "C" MYMFCDLL1_API int DeInit() {
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    g_running = false;
    if (g_imgThread.joinable()) g_imgThread.join();
    g_images.clear();
    return 0;
}

static void RebuildLightsFollowCenter(const gp_Pnt& center,
    const gp_Vec& Xd,
    const gp_Vec& Yd,
    const gp_Vec& Zd)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (g_lightCache.ringCount <= 0 ||
        g_lightCache.regionCount <= 0 ||
        g_lightCache.data.empty())
        return;

    CWinApp* pApp = AfxGetApp();
    CMainFrame* pFrame = pApp ? static_cast<CMainFrame*>(pApp->m_pMainWnd) : nullptr;
    if (!pFrame) return;

    CDocument* pDoc = pFrame->GetActiveDocument();
    CStl3DLasetDoc* pMyDoc = dynamic_cast<CStl3DLasetDoc*>(pDoc);
    if (!pMyDoc || pMyDoc->myViewer.IsNull())
        return;

    Handle(V3d_Viewer) theViewer = pMyDoc->myViewer;

    std::vector<Handle(Graphic3d_CLight)> oldLights;
    theViewer->InitDefinedLights();
    while (theViewer->MoreDefinedLights())
    {
        oldLights.push_back(theViewer->DefinedLight());
        theViewer->NextDefinedLights();
    }
    for (auto& L : oldLights)
    {
        theViewer->SetLightOff(L);
        theViewer->DelLight(L);
    }

    auto getRaw = [](int ring, int region)->uint16_t
    {
        int idx = ring * g_lightCache.regionCount + region;
        if (idx < 0 || idx >= (int)g_lightCache.data.size())
            return 0;
        return g_lightCache.data[idx];
    };

    auto toLum = [&](uint16_t raw, float gain = 1.0f) -> float
    {
        float k = static_cast<float>(raw) / 65535.0f;
        float lum = std::pow(k, 0.4545f) * gain;
        if (lum < 0.0f) lum = 0.0f;
        if (lum > 1.0f) lum = 1.0f;
        return lum;
    };

    auto bottomLum = [&](uint16_t raw) -> float
    {
        float k = static_cast<float>(raw) / 65535.0f;
        float x = 3.0f * std::pow(k, 0.6f);
        float lum = x / (1.0f + x);
        return lum;
    };

    std::vector<Handle(Graphic3d_CLight)> newLights;
    newLights.reserve(g_lightCache.ringCount * g_lightCache.regionCount + 4);

    float bgLum = 0.0f;

    const Standard_Real ringRadii[4] = { 15,25,35,45 };
    const Standard_Real zTop = 30.0;
    const Standard_Real zBottom = -30.0;

    int maxFaceRing = (std::min)(g_lightCache.ringCount, 4);
    for (int ring = 0; ring < maxFaceRing; ++ring)
    {
        Standard_Real radius = ringRadii[ring];

        for (int region = 0; region < g_lightCache.regionCount; ++region)
        {
            uint16_t raw = getRaw(ring, region);
            if (!raw) continue;

            float lum = toLum(raw, 1.0f);
            float angle = 2.0f * (float)PI * (float)region / (float)g_lightCache.regionCount;

            gp_Vec offset =
                Xd * (radius * std::cos(angle)) +
                Yd * (radius * std::sin(angle)) +
                Zd * zTop;

            gp_Pnt pos = center.Translated(offset);

            Handle(Graphic3d_CLight) faceLight =
                new Graphic3d_CLight(Graphic3d_TypeOfLightSource_Positional);
            faceLight->SetPosition(pos);
            faceLight->SetColor(Quantity_Color(lum, lum, lum, Quantity_TOC_RGB));
            faceLight->SetIntensity(1.0f);

            theViewer->AddLight(faceLight);
            theViewer->SetLightOn(faceLight);
            newLights.push_back(faceLight);
        }
    }

    if (g_lightCache.ringCount > 4)
    {
        // 底光：4,0
        if (g_lightCache.regionCount > 0)
        {
            uint16_t raw = getRaw(4, 0);
            if (raw)
            {
                float lum = bottomLum(raw);
                bgLum = lum;

                gp_Vec off = Zd * zBottom;
                gp_Pnt posBottom = center.Translated(off);

                Handle(Graphic3d_CLight) bottom =
                    new Graphic3d_CLight(Graphic3d_TypeOfLightSource_Positional);
                bottom->SetPosition(posBottom);
                bottom->SetDirection(gp_Dir(-Zd));
                bottom->SetColor(Quantity_Color(lum, lum, lum, Quantity_TOC_RGB));
                bottom->SetIntensity(1.0f);

                theViewer->AddLight(bottom);
                theViewer->SetLightOn(bottom);
                newLights.push_back(bottom);
            }
        }

        // 同轴：4,1
        if (g_lightCache.regionCount > 1)
        {
            uint16_t raw = getRaw(4, 1);
            if (raw)
            {
                float lum = toLum(raw, 2.0f);

                gp_Vec offCoax = Zd * (zTop + 50.0);
                gp_Pnt posCoax = center.Translated(offCoax);

                Handle(Graphic3d_CLight) coax =
                    new Graphic3d_CLight(Graphic3d_TypeOfLightSource_Spot);
                coax->SetPosition(posCoax);
                coax->SetDirection(gp_Dir(-Zd));
                coax->SetColor(Quantity_Color(lum, lum, lum, Quantity_TOC_RGB));
                coax->SetIntensity(1.0f);

                theViewer->AddLight(coax);
                theViewer->SetLightOn(coax);
                newLights.push_back(coax);
            }
        }

        // 落射：4,2
        if (g_lightCache.regionCount > 2)
        {
            uint16_t raw = getRaw(4, 2);
            if (raw)
            {
                float lum = toLum(raw, 1.5f);

                gp_Vec offEpi = Zd * (zTop + 50.0);
                gp_Pnt posEpi = center.Translated(offEpi);

                Handle(Graphic3d_CLight) epi =
                    new Graphic3d_CLight(Graphic3d_TypeOfLightSource_Spot);
                epi->SetPosition(posEpi);
                epi->SetDirection(gp_Dir(-Zd));
                epi->SetColor(Quantity_Color(lum, lum, lum, Quantity_TOC_RGB));
                epi->SetIntensity(1.0f);

                theViewer->AddLight(epi);
                theViewer->SetLightOn(epi);
                newLights.push_back(epi);
            }
        }
    }

    for (V3d_ListOfViewIterator vit(theViewer->DefinedViewIterator()); vit.More(); vit.Next())
    {
        Handle(V3d_View) aView = vit.Value();

        aView->InitActiveLights();
        while (aView->MoreActiveLights())
        {
            Handle(Graphic3d_CLight) L = aView->ActiveLight();
            aView->NextActiveLights();
            if (!L.IsNull())
            {
                try { aView->SetLightOff(L); }
                catch (...) {}
            }
        }

        for (const auto& L : newLights)
        {
            if (!L.IsNull())
                aView->SetLightOn(L);
        }

        Quantity_Color bgColor(bgLum, bgLum, bgLum, Quantity_TOC_RGB);
        aView->SetBackgroundColor(bgColor);
        aView->SetShadingModel(Graphic3d_TypeOfShadingModel_Phong);
    }

    theViewer->UpdateLights();
    for (V3d_ListOfViewIterator vit(theViewer->DefinedViewIterator()); vit.More(); vit.Next())
    {
        vit.Value()->Redraw();
    }
}



static int ApplyTrihedronPos(double lx, double ly, double lz)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CWinApp* pApp = AfxGetApp();
    auto* pFrame = pApp ? (CMainFrame*)pApp->m_pMainWnd : nullptr;
    COccView* pOcc = pFrame ? pFrame->m_OccView : nullptr;
    if (!pOcc) return -1;

    Handle(V3d_View) view = pOcc->GetView();
    if (view.IsNull()) return -1;

    Handle(AIS_Trihedron) trihedron = pOcc->GetTrihedron();
    if (trihedron.IsNull()) return -1;

    Handle(Geom_Axis2Placement) axisPlacement = trihedron->Component();
    if (axisPlacement.IsNull()) return -1;

    gp_Ax2 ax2 = axisPlacement->Ax2();
    if (trihedron->HasTransformation())
    {
        gp_Trsf T = trihedron->LocalTransformation();
        ax2.Transform(T);
    }

    const gp_Pnt O = ax2.Location();
    const gp_Vec Xd(ax2.XDirection());
    const gp_Vec Yd(ax2.YDirection());
    const gp_Vec Zd(ax2.Direction()); 

    gp_Pnt eye = O.Translated(Xd * lx + Yd * ly + Zd * lz);

    static Standard_Real sLookDist = 200.0;
    gp_Pnt center = eye.Translated(-Zd * sLookDist);
    gp_Dir up = gp_Dir(Yd);

    g_modelopened.SetVirtualCamera(eye, center, up);

    g_modelopened.UpdateDOFHeightFromView(view, ax2);

    gp_Pnt center2 = eye.Translated(-Zd);

    RebuildLightsFollowCenter(center2, Xd, Yd, Zd);

    std::lock_guard<std::mutex> lk(g_dataMutex);
    g_modelopened.SnapshotFromCurrentView(tempimage_width, tempimage_height);
    
    LocateCameraPos(D_index);
    return 0;
}



extern "C" MYMFCDLL1_API int PositionMotion(double* MoveArray)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (!MoveArray) return -1;

    double lx = MoveArray[0];
    double ly = MoveArray[1];
    double lz = MoveArray[2];

    {
        std::lock_guard<std::mutex> lk(g_motionMutex);
        g_motion.x = lx;
        g_motion.y = ly;
        g_motion.z = lz;
    }

    return ApplyTrihedronPos(lx, ly, lz);
}


extern "C" MYMFCDLL1_API int PositionMotionOffset(double* OffsetArray)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    if (!OffsetArray) return -1;

    double dx = OffsetArray[0];
    double dy = OffsetArray[1];
    double dz = OffsetArray[2];

    double lx, ly, lz;
    {
        std::lock_guard<std::mutex> lk(g_motionMutex);
        g_motion.x += dx;
        g_motion.y += dy;
        g_motion.z += dz;

        lx = g_motion.x;
        ly = g_motion.y;
        lz = g_motion.z;
    }

    return ApplyTrihedronPos(lx, ly, lz);
}

extern "C" MYMFCDLL1_API int SetLightBrightness(int RingCount, int RegionCount, int** light_brightness)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CWinApp* pApp = AfxGetApp();
    CMainFrame* pFrame = static_cast<CMainFrame*>(pApp->m_pMainWnd);
    if (pFrame == nullptr)
        return 0;

    CDocument* pDoc = pFrame->GetActiveDocument();
    CStl3DLasetDoc* pMyDoc = dynamic_cast<CStl3DLasetDoc*>(pDoc);
    if (pMyDoc == nullptr || pMyDoc->myViewer.IsNull())
        return 0;

    Handle(V3d_Viewer) theViewer = pMyDoc->myViewer;

    if (!pMyDoc->myAISContext.IsNull())
    {
        AIS_ListOfInteractive list;
        pMyDoc->myAISContext->DisplayedObjects(list);
        for (AIS_ListIteratorOfListOfInteractive it(list); it.More(); it.Next())
        {
            Handle(AIS_InteractiveObject) obj = it.Value();
            if (obj.IsNull()) continue;

            Handle(Prs3d_Drawer) drw = obj->Attributes();
            if (drw.IsNull())
            {
                drw = new Prs3d_Drawer();
                obj->SetAttributes(drw);
            }

            if (!drw->ShadingAspect().IsNull())
            {
                Handle(Graphic3d_AspectFillArea3d) asp = drw->ShadingAspect()->Aspect();
                if (!asp.IsNull())
                {
                    Graphic3d_MaterialAspect mat = asp->FrontMaterial();
                    mat.SetAmbientColor(Quantity_Color(0.12, 0.12, 0.12, Quantity_TOC_RGB));
                    mat.SetSpecularColor(Quantity_Color(0.20, 0.20, 0.20, Quantity_TOC_RGB));
                    mat.SetShininess(0.4f);
                    asp->SetFrontMaterial(mat);
                    asp->SetBackMaterial(mat);
                }
            }
            pMyDoc->myAISContext->Redisplay(obj, Standard_False);
        }
        pMyDoc->myAISContext->UpdateCurrentViewer();
    }

    g_lightCache.ringCount = RingCount;
    g_lightCache.regionCount = RegionCount;
    g_lightCache.data.resize(RingCount * RegionCount);
    for (int r = 0; r < RingCount; ++r)
    {
        for (int c = 0; c < RegionCount; ++c)
        {
            g_lightCache.data[r * RegionCount + c] =
                (static_cast<uint16_t>(light_brightness[r][c]))/10;
        }
    }

    COccView* pOcc = pFrame->m_OccView;
    if (pOcc)
    {
        Handle(AIS_Trihedron) trihedron = pOcc->GetTrihedron();
        if (!trihedron.IsNull())
        {
            Handle(Geom_Axis2Placement) axisPlacement = trihedron->Component();
            if (!axisPlacement.IsNull())
            {
                gp_Ax2 ax2 = axisPlacement->Ax2();
                if (trihedron->HasTransformation())
                {
                    gp_Trsf T = trihedron->LocalTransformation();
                    ax2.Transform(T);
                }

                gp_Pnt center;
                gp_Pnt eye;
                gp_Dir up;
                g_modelopened.GetVirtualCamera(eye, center, up);

                gp_Vec Xd(ax2.XDirection());
                gp_Vec Yd(ax2.YDirection());
                gp_Vec Zd(ax2.Direction());

                gp_Pnt center2 = eye.Translated(-Zd);

                RebuildLightsFollowCenter(center2, Xd, Yd, Zd);
            }
        }
    }

    g_modelopened.SnapshotFromCurrentView(tempimage_width, tempimage_height);
    return 0;
}



static int LocateCameraPos(int index)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CWinApp* pApp = AfxGetApp();
    auto* pFrame = pApp ? (CMainFrame*)pApp->m_pMainWnd : nullptr;
    COccView* pOcc = pFrame ? pFrame->m_OccView : nullptr;
    if (!pOcc) return -1;

    Handle(V3d_View) view = pOcc->GetView();
    if (view.IsNull()) return -1;

    Handle(AIS_Trihedron) trihedron = pOcc->GetTrihedron();
    Handle(Geom_Axis2Placement) axisPlacement = trihedron->Component();
    gp_Ax2 ax2 = axisPlacement->Ax2();
    if (trihedron->HasTransformation())
    {
        gp_Trsf T = trihedron->LocalTransformation();
        ax2.Transform(T);
    }

    const gp_Pnt O = ax2.Location();
    const gp_Vec Xd(ax2.XDirection());
    const gp_Vec Yd(ax2.YDirection());
    const gp_Vec Zd(ax2.Direction());

    gp_Pnt eye, center;
    gp_Dir up;
    g_modelopened.GetVirtualCamera(eye, center, up);

    gp_Vec v(O, eye);
    double lx = v.Dot(Xd.Normalized());
    double ly = v.Dot(Yd.Normalized());
    double lz = v.Dot(Zd.Normalized());

    g_positionCallback(index, lx, ly, lz);
    return 0;
}


struct ZoomPreset {
    TScalePara para;
    double     dof_mm{ 0.0 };
};

static std::unordered_map<int, ZoomPreset> g_zoomPresets;
static std::mutex g_zoomPresetsMtx;

extern "C" MYMFCDLL1_API int SetScalePara(int zoom_index, TScalePara * scale_para, double DOF_mm)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    if (zoom_index <= 0 || scale_para == nullptr) {
        return -1;
    }
    if (scale_para->calibrated_flag &&
        (scale_para->x_scale <= 0.0 || scale_para->y_scale <= 0.0))
    {
        return -1;
    }

    std::lock_guard<std::mutex> lk(g_zoomPresetsMtx);
    g_zoomPresets[zoom_index] = ZoomPreset{ *scale_para, DOF_mm };
    return 0;
}

extern "C" MYMFCDLL1_API int Zoom(int zoom_index)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    ZoomPreset preset;
    {
        std::lock_guard<std::mutex> lk(g_zoomPresetsMtx);
        auto it = g_zoomPresets.find(zoom_index);
        if (it == g_zoomPresets.end()) {
            return -1;
        }
        preset = it->second;
    }

    if (!g_modelopened.TurnToScalePara(zoom_index, preset.para, preset.dof_mm))
        return -1;

    CWinApp* pApp = AfxGetApp();
    auto* pFrame = pApp ? (CMainFrame*)pApp->m_pMainWnd : nullptr;
    COccView* pOcc = pFrame ? pFrame->m_OccView : nullptr;
    if (pOcc)
    {
        Handle(V3d_View) view = pOcc->GetView();
        if (!view.IsNull())
        {
            Handle(AIS_Trihedron) trihedron = pOcc->GetTrihedron();
            if (!trihedron.IsNull())
            {
                Handle(Geom_Axis2Placement) axisPlacement = trihedron->Component();
                if (!axisPlacement.IsNull())
                {
                    gp_Ax2 ax2 = axisPlacement->Ax2();
                    if (trihedron->HasTransformation())
                    {
                        gp_Trsf T = trihedron->LocalTransformation();
                        ax2.Transform(T);
                    }

                    g_modelopened.UpdateDOFHeightFromView(view, ax2);
                }
            }
        }
    }

    double lx, ly, lz;
    {
        std::lock_guard<std::mutex> lk(g_motionMutex);
        lx = g_motion.x;
        ly = g_motion.y;
        lz = g_motion.z;
    }
    return ApplyTrihedronPos(lx, ly, lz);
}

