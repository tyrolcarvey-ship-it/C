//// COccView.cpp: 实现文件
////


#include <stdlib.h>
#include <crtdbg.h>
#include "stdafx.h"
#include "Stl3DLaset.h"
#include"unordered_set"
//#include "Stl3DLaset.h"
#include "COccView.h"
#include"MyMemDC.h"
#include"CPDFManager.h"
#include <GProp_GProps.hxx>


// OCC特定头文件
#include <WNT_Window.hxx>
#include <BRepBuilderAPI_MakeVertex.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS_Face.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Surface.hxx>

 //COccView* g_pOccView = nullptr;
// COccView
static std::atomic<COccView*> g_cbTarget{ nullptr };
std::mutex g_mtx;
constexpr int ImgDataSize = 3200000;
BOOL g_buildflag = false;
vector<gp_Pnt> g_previewPoints;
vector<gp_Pnt> g_point = {
    /*{-33.648399, 71.983395, -0.706909 },
    { -33.648332, 25.983395, -0.703837},
    { -33.648265, -20.016604,-0.706653},
    { -33.648198, -66.016603,-0.699997},
    { 32.657226 ,72.668345,-1.010781  },
    { 32.657149 ,26.668344,-1.009245  },
    { 32.657072 ,-19.331657,-1.013085 },
    { 32.656994 ,-65.331658,-1.028957 },
    { -29.375274, -78.533209,-1.051997},
    { -1.379381 ,-78.533252, -1.004381},
    { 26.619103 ,-78.533268, -0.989789},
    { -27.531158, 77.533186,-0.683357 },
    { 0.465618,77.533152,-0.675165    },
    { 28.464262,77.533138,-0.766557   },
    { -27.531101,38.533186,-0.022877  },
    { -27.531044,-0.466813,-0.021085  },
    { -27.530987,-39.466812,-0.019549 },
    { 26.619169,-39.533267,0.011939   },
    { 26.619234,-0.533267,0.015523    },
    { 26.619300,38.466734,0.015779    }*/
};
IMPLEMENT_DYNCREATE(COccView, CView)

COccView::COccView()
{// 添加默认变倍级别
    mouseDownPT.x = 0;
    mouseDownPT.y = 0;

    m_ColorScale = new AIS_ColorScale();
    m_Pt.x = 0;
    m_Pt.y = 0;
    m_scale = 1.0;
    pdfMgr = new CPDFManager();
    m_mySignalLen = sizeof(SIGNMEMORY_T);
    m_mySignal = std::shared_ptr<char>(new char[m_mySignalLen] {0});
    //m_ipc = std::shared_ptr<CIPCBase>(GetInstance());
    m_ipc = std::shared_ptr<CIPCBase>(GetInstance(), [](CIPCBase*) {});
    m_alive.store(true, std::memory_order_relaxed);
}

COccView::~COccView()
{
    KillTimer(1); KillTimer(2); KillTimer(3);
    KillTimer(4); KillTimer(5); KillTimer(103);

    m_alive.store(false, std::memory_order_relaxed);
    g_cbTarget.store(nullptr, std::memory_order_release);

    SafeReleaseIpc();

    m_th1IsRun.store(false, std::memory_order_relaxed);
    if (th1.joinable()) {
        th1.join();
    }

    if (!m_AISContext.IsNull()) {
        for (auto& s : m_Sphere) if (!s.IsNull()) m_AISContext->Remove(s, Standard_False);
        for (auto& s : m_temporaryShapes) if (!s.IsNull()) m_AISContext->Erase(s, Standard_False);
        if (!m_ColorScale.IsNull() && m_AISContext->IsDisplayed(m_ColorScale)) {
            m_AISContext->Remove(m_ColorScale, Standard_False);
        }
        m_AISContext->UpdateCurrentViewer();
    }
    m_Sphere.clear();
    m_temporaryShapes.clear();

    delete pdfMgr; pdfMgr = nullptr;

}

void COccView::SafeReleaseIpc() noexcept
{
    if (!m_ipc) return;
    if (m_ipcReleased.exchange(true)) return;

    try { m_ipc->releaseMyThread(); }
    catch (...) {}
    try { m_ipc->releaseSDThread(); }
    catch (...) {}
}


BEGIN_MESSAGE_MAP(COccView, CView)
    ON_WM_MOUSEMOVE()
    ON_WM_MOUSEWHEEL()
    ON_WM_LBUTTONDOWN()
    //	ON_BN_CLICKED(IDC_BUTTON_PICK_TARGET, &COccView::OnBnClickedBtnTestMove)
        //	ON_BN_CLICKED(IDC_BUTTON_PICK_TARGET1, &COccView::OnBnClickedBtnTestMove1)

    ON_WM_LBUTTONUP()
    ON_WM_MBUTTONDOWN()
    ON_WM_MBUTTONUP()
    ON_WM_RBUTTONDOWN()
    ON_WM_CREATE()
    ON_WM_TIMER()
END_MESSAGE_MAP()


// COccView 绘图

void COccView::OnDraw(CDC* pDC)
{
    // TODO:  在此添加绘制代码
    if (!myView.IsNull())
    {
        myView->MustBeResized();
        myView->Update();
        myView->Redraw();
        //FitAll();
        if (m_showBias)
        {
            RedrawLableLine();
        }
    }
}


// COccView 诊断

#ifdef _DEBUG
void COccView::AssertValid() const
{
    CView::AssertValid();
}

#ifndef _WIN32_WCE
void COccView::Dump(CDumpContext& dc) const
{
    CView::Dump(dc);
}
#endif
#endif //_DEBUG

static void __stdcall MultiModelThunk(char* signal, int res) noexcept
{
    COccView* self = g_cbTarget.load(std::memory_order_acquire);
    if (!self) return;
    if (!self->m_alive.load(std::memory_order_relaxed)) return;
    self->OnMultiModel(signal, res);
}

void COccView::FitAll()
{
    if (!myView.IsNull())
    {
        myView->FitAll();
    }
    m_scale = 1.0f;
    myView->ZFitAll();
}

void COccView::selectMode(Handle(AIS_Shape) selectmode)
{
    //激活将形状分解为Any
    m_AISContext->Deactivate();
    m_AISContext->Activate(selectmode, AIS_Shape::SelectionMode(TopAbs_FACE));
    m_AISContext->Activate(selectmode, AIS_Shape::SelectionMode(TopAbs_SOLID));
    m_AISContext->Activate(selectmode, AIS_Shape::SelectionMode(TopAbs_VERTEX));
    m_AISContext->Activate(selectmode, AIS_Shape::SelectionMode(TopAbs_EDGE));

}
//激活所有选取特征模式
void COccView::selectMode()
{
    m_AISContext->Deactivate();
    m_AISContext->Activate(AIS_Shape::SelectionMode(TopAbs_FACE));
    m_AISContext->Activate(AIS_Shape::SelectionMode(TopAbs_SOLID));
    m_AISContext->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
    m_AISContext->Activate(AIS_Shape::SelectionMode(TopAbs_EDGE));
}
//设置当前上下文
BOOL COccView::SetAISContext(Handle(AIS_InteractiveContext) myAISContext)
{
    if (!myAISContext)
        return false;
    /*if (m_context.size() && m_context.size() != activeindex)
        m_context[activeindex]->EraseAll(Standard_True);*/
        //m_AISContext = myAISContext;
    myAISContext->HighlightStyle(Prs3d_TypeOfHighlight_LocalDynamic)->SetColor(Quantity_NOC_LIGHTSEAGREEN);
    myAISContext->HighlightStyle(Prs3d_TypeOfHighlight_LocalDynamic)->SetMethod(Aspect_TOHM_COLOR);
    myAISContext->HighlightStyle(Prs3d_TypeOfHighlight_LocalDynamic)->SetDisplayMode(1);
    myAISContext->HighlightStyle(Prs3d_TypeOfHighlight_LocalDynamic)->SetTransparency(0.7f);
    myAISContext->SetToHilightSelected(Standard_True);
    //myAISContext->HighlightStyle(Prs3d_TypeOfHighlight_LocalSelected)->SetColor(Quantity_NameOfColor::Quantity_NOC_YELLOW);

    //设置对象的部分选中时的绘制风格
    const Handle(Prs3d_Drawer)& aPartSelStyle = myAISContext->HighlightStyle(Prs3d_TypeOfHighlight::Prs3d_TypeOfHighlight_LocalSelected);
    aPartSelStyle->SetMethod(Aspect_TOHM_COLOR);
    aPartSelStyle->SetDisplayMode(1);
    aPartSelStyle->SetColor(Quantity_NameOfColor::Quantity_NOC_RED);
    aPartSelStyle->SetTransparency(0.5f);
    aPartSelStyle->PointAspect()->SetTypeOfMarker(Aspect_TOM_PLUS);
    aPartSelStyle->PointAspect()->SetScale(3.);

    //selectMode();
    //activeindex = m_context.size();
    m_context.push_back(myAISContext);
    //m_samplingShape = NULL;
    return SwitchAISContext(m_context.size() - 1);
}
BOOL COccView::SetViewer(Handle(V3d_Viewer) myViewer)
{
    try {
        if (myViewer.IsNull()) {
            AfxMessageBox(_T("查看器为空！"));
            return FALSE;
        }
        if (myViewer->StructureManager().IsNull()) {
            AfxMessageBox(_T("查看器的结构管理器为空！"));
            return FALSE;
        }

        // 创建视图
        myView = myViewer->CreateView();
        if (myView.IsNull()) {
            AfxMessageBox(_T("创建视图失败！"));
            return FALSE;
        }

        return TRUE;
    }
    catch (const Standard_Failure& e) {
        CString errMsg;
        errMsg.Format(_T("SetViewer中发生OCC异常: %s"), e.GetMessageString());
        AfxMessageBox(errMsg);
        return FALSE;
    }
    catch (...) {
        AfxMessageBox(_T("SetViewer中发生未知异常"));
        return FALSE;
    }
}
//根据索引切换视图上下文
//BOOL COccView::SwitchAISContext(int index)
//{
//    if (m_context.size() < index)
//    {
//        m_context.erase(m_context.begin() + m_context.size());
//        return FALSE;
//    }
//    if (m_drawBias)
//    {
//        MessageBoxA("请先停止扫描");
//        //m_context.erase(m_context.begin() + m_context.size());
//        return FALSE;
//    }
//    if (m_context.size() == 1)
//        m_AISContext = m_context[index];
//    else
//    {
//        RemoveCollectedData();
//        points.clear();
//        m_lableList.clear();
//        m_samplingShape = NULL;
//        m_AISContext->RemoveAll(Standard_True);
//        //m_AISContext->EraseAll(Standard_True);
//        m_AISContext = m_context[index];
//        if (activeindex < m_Shape.size() && m_Shape[activeindex].size()>1)
//        {
//            vector<Handle(AIS_Shape)>& ShowList = m_Shape[activeindex];
//            ShowList.erase(ShowList.begin() + 1, ShowList.end());
//        }
//        /*vector<Handle(AIS_Shape)>& ShowList = m_Shape[activeindex];
//        if (ShowList.size() > 1)
//        {
//            ShowList.erase(ShowList.begin() + 1, ShowList.end());
//        }*/
//    }
//    //m_AISContext->DisplayAll(Standard_True);
//    if (m_Shape.size() > index)
//        m_AISContext->Display(m_Shape[index][0], Standard_True);
//    activeindex = index;
//    FitAll();
//    selectMode();
//    myView->Update();
//    myView->Redraw();
//    return TRUE;
//}

BOOL COccView::SwitchAISContext(int index)//后修复越界
{
    if (index < 0 || index >= (int)m_context.size())
        return FALSE;
    if (m_drawBias) { MessageBoxA("请先停止扫描"); return FALSE; }

    if (m_context.size() == 1) {
        m_AISContext = m_context[index];
    }
    else {
        RemoveCollectedData();
        points.clear();
        m_lableList.clear();
        m_samplingShape = NULL;
        m_AISContext->RemoveAll(Standard_True);
        m_AISContext = m_context[index];
        if (activeindex < (int)m_Shape.size() && m_Shape[activeindex].size() > 1) {
            auto& ShowList = m_Shape[activeindex];
            ShowList.erase(ShowList.begin() + 1, ShowList.end());
        }
    }
    if ((int)m_Shape.size() > index && !m_Shape[index].empty())
        m_AISContext->Display(m_Shape[index][0], Standard_True);

    activeindex = index;
    FitAll();
    selectMode();
    myView->Update();
    myView->Redraw();
    return TRUE;
}


//获取当前活动上下文的模型
TopoDS_Shape COccView::GetActiveShape()
{
    TopoDS_Shape shape = m_Shape[activeindex][0]->Shape();
    return shape;
}
//清除上下文
void COccView::ClearAISContext(int index)
{
    if (m_drawBias)
    {
        MessageBoxA("请先停止扫描");
        return;
    }
    RemoveCollectedData();
    points.clear();
    //m_AISContext->RemoveAll(Standard_True);
    m_context[index]->RemoveAll(Standard_True);
    //m_context[index]->Delete();
    m_context[index].Nullify();
    m_context.erase(m_context.begin() + index);
    m_Shape.erase(m_Shape.begin() + index);
    m_samplingShape = NULL;
    if (index < activeindex)
        activeindex--;
    FitAll();
    myView->Update();
    myView->Redraw();
    //Standard::Purge();
}
//停止收点
void COccView::PauseCollecting()
{
    if (m_ifPause)
    {
        m_ifPause.store(false);
    }
    else
    {
        m_ifPause.store(true);
    }
}
//根据一个点的偏差值和整体偏差值计算该点颜色
Quantity_Color TransToClr(Standard_Real dis, Standard_Real tol)
{
    Standard_Real r, g, b; Standard_Real clrcnt = 471.0;
    Standard_Real iColStep = (8.0 * (255 - 0)) / 471.0;
    Standard_Real clrNum = (471.0 * (tol - dis) / (2 * tol));
    if (clrNum < 0)
    {
        r = 1.0;
        g = 0.0;
        b = 0.0;
    }
    else if (clrNum >= 0 && clrNum < clrcnt * 3 / 8)
    {
        //iColStep = (8.0 * (255 - 0)) / 471 / 3;
        r = 255 / 255.0;
        g = (0 + clrNum * iColStep / 3) / 255.0;
        b = 0.0;

    }
    else if (clrNum >= clrcnt * 3 / 8 && clrNum < clrcnt / 2)
    {
        //iColStep = (8.0 * (255 - 0)) / 471;
        r = (255 - (clrNum - clrcnt * 3 / 8) * iColStep) / 255.0;
        g = 1.0;
        b = 0.0;
    }
    else if (clrNum >= clrcnt / 2 && clrNum < clrcnt * 5 / 8)
    {
        //iColStep = (8.0 * (255 - 0)) / 471;
        r = 0.0;
        g = 1.0;
        b = (clrNum - clrcnt / 2) * iColStep / 255.0;
    }
    else if (clrNum >= clrcnt * 5 / 8 && clrNum < clrcnt)
    {
        //iColStep = (8.0 * (255 - 0)) / 471 / 3;
        r = 0.0;
        g = (255 - (clrNum - clrcnt * 5 / 8) * iColStep / 3) / 255.0;
        b = 1.0;
    }
    else if (clrNum >= clrcnt)
    {
        r = 0.0;
        g = 0.0;
        b = 1.0;
    }
    return Quantity_Color(r, g, b, Quantity_TOC_RGB);
}
//将计算的一个点的偏差值数据转为string形式
void ConvertToString(vector<std::string>& data, Standard_Real dis, gp_Pnt p, gp_Pnt measuredLoc)
{
    CString distance, p_x, p_y, p_z, measure_x, measure_y, measure_z;
    distance.Format("%.6f", dis);
    p_x.Format("%.6f", p.X()); p_y.Format("%.6f", p.Y()); p_z.Format("%.6f", p.Z());
    measure_x.Format("%.6f", measuredLoc.X()); measure_y.Format("%.4f", measuredLoc.Y()); measure_z.Format("%.4f", measuredLoc.Z());
    data.push_back(distance.GetString());
    data.push_back(p_x.GetString()); data.push_back(p_y.GetString()); data.push_back(p_z.GetString());
    data.push_back(measure_x.GetString()); data.push_back(measure_y.GetString()); data.push_back(measure_z.GetString());
}
//将计算的整体数据转为string形式
void ConvertToList(vector<std::string>& data, vector<Standard_Real> dataList)
{
    for (Standard_Real dt : dataList)
    {
        CString str; str.Format("%.4f", dt);
        data.push_back(str.GetString());
    }
}
//针对excel一次性输入目标位置计算偏差值
void COccView::CompareCollecting()
{
    CString path = (GetAppPath()).c_str();
    CString num1, num2;
    num1.Format(_T("\\png\\objectSnap.png"));
    num1.Format(_T("\\png\\biasSnap.png"));
    //path += num1;
    CaptureScreenPng(path + num1); CaptureScreenPng(path + num2);
    Sleep(100);
    tmpSignal.state = 0;
    memcpy_s(m_mySignal.get(), m_mySignalLen, &tmpSignal, m_mySignalLen);
    m_ipc->writeToMySignalSharedMemory(m_mySignal.get(), m_mySignalLen);
    ///////////此处转移///////////
    //auto dataBack = [this](char* signal, int res) {

    //    memcpy_s(&tmpSignal, m_mySignalLen, signal, m_mySignalLen);
    //    //如果state为1，读取数据
    //    if (tmpSignal.state == 1)
    //    {
    //        m_showBias = TRUE;
    //        //数据总长度
    //        int totalLen = tmpSignal.TotalLength;
    //        std::shared_ptr<char> m_data = std::shared_ptr<char>(new char[totalLen + 100]);
    //        res = m_ipc->readFromMyDataSharedMemory(m_data.get(), tmpSignal.TotalLength);
    //        if (res == 0)
    //        {
    //            //数据处理前
    //            tmpSignal.state = 3;
    //            memcpy_s(signal, m_mySignalLen, &tmpSignal, m_mySignalLen);
    //            m_ipc->writeToMySignalSharedMemory(signal, m_mySignalLen);
    //            //--------------------------------------数据处理----------------------------------
    //            m_dataMul.pointNum = tmpSignal.ArrayNum;

    //            //元素标识符
    //            char* idTmp = new char[tmpSignal.idLength + 1];
    //            memcpy_s(idTmp, tmpSignal.idLength, m_data.get() + sizeof(double), tmpSignal.idLength);
    //            idTmp[tmpSignal.idLength] = '\0';
    //            m_dataMul.elementId = std::string(idTmp);
    //            //3D模型文件路径
    //            char* pathTmp = new char[tmpSignal.pathLength + 1];
    //            memcpy_s(pathTmp, tmpSignal.pathLength, m_data.get() + sizeof(double) + tmpSignal.idLength, tmpSignal.pathLength);
    //            pathTmp[tmpSignal.pathLength] = '\0';
    //            m_dataMul.modelFilePath = std::string(pathTmp);
    //            //点数据
    //            vector<gp_Pnt> collectedPoints;
    //            for (int i = 0; i < tmpSignal.ArrayNum; ++i)
    //            {
    //                POINT3D_T point{ 0 };
    //                memcpy_s(&point.Xcoordinate, sizeof(double), m_data.get() + sizeof(double)
    //                    + tmpSignal.idLength + tmpSignal.pathLength + sizeof(double) * i, sizeof(double));
    //                memcpy_s(&point.Ycoordinate, sizeof(double), m_data.get() + sizeof(double)
    //                    + tmpSignal.idLength + tmpSignal.pathLength + tmpSignal.ArrayNum * sizeof(double) + sizeof(double) * i, sizeof(double));
    //                memcpy_s(&point.Zcoordinate, sizeof(double), m_data.get() + sizeof(double)
    //                    + tmpSignal.idLength + tmpSignal.pathLength + tmpSignal.ArrayNum * sizeof(double) * 2 + sizeof(double) * i, sizeof(double));

    //                collectedPoints.push_back({ point.Xcoordinate,point.Ycoordinate,point.Zcoordinate });
    //            }
    //            double tol = ComparePoints(collectedPoints);

    //            //写面轮廓度
    //            char* testValueC = new char[sizeof(double)];
    //            memcpy_s(testValueC, sizeof(double), &tol, sizeof(double));
    //            m_ipc->writeToMyDataSharedMemory(testValueC, sizeof(double));

    //            delete idTmp;
    //            idTmp = nullptr;
    //            delete pathTmp;
    //            pathTmp = nullptr;
    //            delete testValueC;
    //            testValueC = nullptr;
    //            //Invalidate(false);
    //            //SetTimer(1, 1000, NULL);

    //        }
    //        else
    //            std::cout << "读取数据失败" << std::endl;
    //    }
    //    else if (tmpSignal.state == 4)
    //    {
    //        m_position.clear();
    //        table.clear(); ptTable.clear();
    //        tmpSignal.state = 0;
    //        memcpy_s(signal, m_mySignalLen, &tmpSignal, m_mySignalLen);
    //        m_ipc->writeToMySignalSharedMemory(signal, m_mySignalLen);
    //        SetTimer(3, 1, NULL);
    //    }
    //    else if (tmpSignal.state == 5)
    //    {
    //        tmpSignal.state = 0;
    //        memcpy_s(signal, m_mySignalLen, &tmpSignal, m_mySignalLen);
    //        m_ipc->writeToMySignalSharedMemory(signal, m_mySignalLen);
    //    }
    //    };
    //m_ipc->setCallBackMultiModel(dataBack, m_mySignal.get(), m_mySignalLen);
    //m_ipc->initMyThread();

    // 回调指向当前实例
    g_cbTarget.store(this, std::memory_order_release);

    // 注册静态跳板
    m_ipc->setCallBackMultiModel(&MultiModelThunk, m_mySignal.get(), m_mySignalLen);

    // 启动库线程
    m_ipc->initMyThread();
}

void COccView::OnMultiModel(char* signal, int res) noexcept
{
    if (!m_alive.load(std::memory_order_relaxed)) return;

    memcpy_s(&tmpSignal, m_mySignalLen, signal, m_mySignalLen);

    if (tmpSignal.state == 1)
    {
        m_showBias = TRUE;
        int totalLen = tmpSignal.TotalLength;
        std::shared_ptr<char> m_data(new char[totalLen + 100]);

        res = m_ipc->readFromMyDataSharedMemory(m_data.get(), tmpSignal.TotalLength);
        if (res == 0)
        {
            // 数据处理前
            tmpSignal.state = 3;
            memcpy_s(signal, m_mySignalLen, &tmpSignal, m_mySignalLen);
            m_ipc->writeToMySignalSharedMemory(signal, m_mySignalLen);

            // ------------ 业务解析 ------------
            m_dataMul.pointNum = tmpSignal.ArrayNum;

            // 元素标识符
            char* idTmp = new char[tmpSignal.idLength + 1];
            memcpy_s(idTmp, tmpSignal.idLength, m_data.get() + sizeof(double), tmpSignal.idLength);
            idTmp[tmpSignal.idLength] = '\0';
            m_dataMul.elementId = std::string(idTmp);

            // 3D模型文件路径
            char* pathTmp = new char[tmpSignal.pathLength + 1];
            memcpy_s(pathTmp, tmpSignal.pathLength, m_data.get() + sizeof(double) + tmpSignal.idLength, tmpSignal.pathLength);
            pathTmp[tmpSignal.pathLength] = '\0';
            m_dataMul.modelFilePath = std::string(pathTmp);

            // 点数据
            std::vector<gp_Pnt> collectedPoints;
            for (int i = 0; i < tmpSignal.ArrayNum; ++i)
            {
                POINT3D_T point{ 0 };
                memcpy_s(&point.Xcoordinate, sizeof(double),
                    m_data.get() + sizeof(double) + tmpSignal.idLength + tmpSignal.pathLength + sizeof(double) * i, sizeof(double));
                memcpy_s(&point.Ycoordinate, sizeof(double),
                    m_data.get() + sizeof(double) + tmpSignal.idLength + tmpSignal.pathLength + tmpSignal.ArrayNum * sizeof(double) + sizeof(double) * i, sizeof(double));
                memcpy_s(&point.Zcoordinate, sizeof(double),
                    m_data.get() + sizeof(double) + tmpSignal.idLength + tmpSignal.pathLength + tmpSignal.ArrayNum * sizeof(double) * 2 + sizeof(double) * i, sizeof(double));

                collectedPoints.push_back({ point.Xcoordinate,point.Ycoordinate,point.Zcoordinate });
            }
            double tol = ComparePoints(collectedPoints);

            // 写面轮廓度
            char* testValueC = new char[sizeof(double)];
            memcpy_s(testValueC, sizeof(double), &tol, sizeof(double));
            m_ipc->writeToMyDataSharedMemory(testValueC, sizeof(double));

            delete[] idTmp;   idTmp = nullptr;
            delete[] pathTmp; pathTmp = nullptr;

            delete[] testValueC; testValueC = nullptr;

            //Invalidate(false);
            //SetTimer(1, 1000, NULL);
        }
        else
        {
            std::cout << "读取数据失败" << std::endl;
        }
    }
    else if (tmpSignal.state == 4)
    {
        m_position.clear();
        table.clear(); ptTable.clear();
        tmpSignal.state = 0;
        memcpy_s(signal, m_mySignalLen, &tmpSignal, m_mySignalLen);
        m_ipc->writeToMySignalSharedMemory(signal, m_mySignalLen);
        SetTimer(3, 1, NULL);
    }
    else if (tmpSignal.state == 5)
    {
        tmpSignal.state = 0;
        memcpy_s(signal, m_mySignalLen, &tmpSignal, m_mySignalLen);
        m_ipc->writeToMySignalSharedMemory(signal, m_mySignalLen);
    }
}

//以标准软件传入的测试位置与算法计算的目标位置作为输入计算偏差值
void COccView::StartCollecting()
{
    // 清除历史数据（计算前准备）
    RemoveCollectedData();
    CString path = (GetAppPath() + _T("\\png\\objectSnap.png")).c_str();
    CaptureScreenPng(path);
    m_drawBias = true;  // 标记正在扫描

    // 加锁访问点数据
    std::lock_guard<std::mutex> lock(m_profilePointMutex);

    //// 校验点是否已传完（未传完则提示）
    //if (m_tempProfilePoints.empty() || !m_isDllPointsReady)
    //{
    //    MessageBoxA("请先通过DLL接口传入所有测量点（flagBit=1标识最后一个点）");
    //    m_drawBias = false;
    //    return;
    //}

    // 执行计算（核心逻辑）
    m_position = m_tempProfilePoints;  // 复制点集到计算容器
    double tol = SearchNstPoints();    // 计算面轮廓度

    // 计算完成：更新状态和结果
    m_calculatedTolerance = tol;       // 保存结果
    m_isCalculated = true;             // 标记计算完成

    // 重置扫描状态（准备下次操作）
    m_drawBias = false;
    // 可选：保留点集供后续查看，或清除：m_tempProfilePoints.clear();

}

//停止与标准软件打点交互
void COccView::CancelCollecting()
{
    /*CString path = (GetAppPath()).c_str();
    CString num;
    num.Format(_T("\\png\\biasSnap.png"));
    path += num;
    CaptureScreenPng(path);
    Sleep(100);*/
    //KillTimer(3);
    tmpSignal.state = 5;
    memcpy_s(m_mySignal.get(), m_mySignalLen, &tmpSignal, m_mySignalLen);
    m_ipc->writeToMySignalSharedMemory(m_mySignal.get(), m_mySignalLen);
    //m_ipc->releaseMyThread();
    SafeReleaseIpc();
    RemoveCollectedData();
    Sleep(50);
    Invalidate(FALSE);
}
//清除收点数据
void COccView::RemoveCollectedData()
{
    ClearRealTimeDisplay();

    if (m_Shape.empty())
        return;
    for (auto shape : m_Sphere)
    {
        m_AISContext->Remove(shape, Standard_True);
    }
    m_position.clear(); m_Sphere.clear();
    //m_collectedCount = 0; 
    table.clear(); ptTable.clear();
    m_drawBias = false; m_showBias = false; m_ifPause.store(false);
    RemoveLableList();
    m_listRgn.clear();
}
//收点数据集
void COccView::SetSphereData(vector<gp_Pnt> position)
{
    m_position = position;
}
//清除根据收点数据位置渲染的小球
void COccView::ClearSphereData()
{
    Translate(m_position[0]);
    for (auto shape : m_Sphere) {
        m_AISContext->SetColor(shape, Quantity_NameOfColor::Quantity_NOC_RED, Standard_False);
    }
    /*for (auto shape : m_Sphere) {
        m_AISContext->Remove(shape, Standard_True);
    }
    m_position.clear(); m_Sphere.clear();*/
    Invalidate(FALSE);
}

void COccView::ApplyViewerGrid()
{
    if (myView.IsNull()) return;
    Handle(V3d_Viewer) aViewer = myView->Viewer();
    if (aViewer.IsNull()) return;

    if (m_gridMode == EGridMode::Off)
    {
        aViewer->HideGridEcho(myView);
        aViewer->DeactivateGrid();
        myView->Redraw();
        return;
    }

    gp_Pnt O(0, 0, 0);
    gp_Dir Xd(1, 0, 0);
    gp_Dir Zd(0, 0, 1);

    if (HasTrihedron())
    {
        gp_Ax2 ax2 = m_CurrentCoordinate;
        O = ax2.Location();
        Zd = ax2.Direction();
        Xd = ax2.XDirection();
    }
    gp_Dir Yd = Zd.Crossed(Xd);

    gp_Ax3 gridAx;
    switch (m_gridMode)
    {
    case EGridMode::XY: gridAx = gp_Ax3(O, Zd, Xd); break;  
    case EGridMode::XZ: gridAx = gp_Ax3(O, Yd, Xd); break;  
    case EGridMode::YZ: gridAx = gp_Ax3(O, Xd, Yd); break;  
    default:            gridAx = gp_Ax3(O, Zd, Xd); break;
    }

    aViewer->SetPrivilegedPlane(gridAx);

    const Standard_Real step = m_gridStep;
    aViewer->SetRectangularGridValues(0.0, 0.0, step, step, 0.0);

    Standard_Real size = 1000.0;
    if (!m_Shape.empty() && activeindex >= 0 && activeindex < (int)m_Shape.size() && !m_Shape[activeindex].empty())
    {
        Bnd_Box b = GetShapeBox(0);
        Standard_Real xmin, ymin, zmin, xmax, ymax, zmax;
        b.Get(xmin, ymin, zmin, xmax, ymax, zmax);
        size = Max(xmax - xmin, ymax - ymin) * 1.2;
        if (size < step * 10) size = step * 10;
    }

    aViewer->SetRectangularGridGraphicValues(size, size, 0.0);

    aViewer->DeactivateGrid();
    aViewer->ActivateGrid(Aspect_GT_Rectangular, Aspect_GDM_Lines);

    myView->Redraw();
}

void COccView::CycleViewerGrid(Standard_Real step)
{
    if (step > 0) m_gridStep = step;

    switch (m_gridMode)
    {
    case EGridMode::Off: m_gridMode = EGridMode::XY; break;
    case EGridMode::XY:  m_gridMode = EGridMode::XZ; break;
    case EGridMode::XZ:  m_gridMode = EGridMode::YZ; break;
    case EGridMode::YZ:  m_gridMode = EGridMode::Off; break;
    default:             m_gridMode = EGridMode::Off; break;
    }

    ApplyViewerGrid();
}

//添加特征
void COccView::Add(Handle(AIS_Shape) shape)
{
    if (activeindex < m_Shape.size())
        m_Shape[activeindex].push_back(shape);
    else
        m_Shape.push_back({ shape });
    m_AISContext->SetColor(shape, Quantity_NameOfColor::Quantity_NOC_GRAY, Standard_True);
    m_AISContext->Display(shape, Standard_True);
    selectMode();
}
//根据特征的不同类型，添加特征
void COccView::Add(Handle(AIS_Shape) shape, FeatureType type)
{
    if (activeindex < m_Shape.size())
    {
        m_Shape[activeindex].push_back(shape);
        CString str;
        addCallBack(type, str);
        //pView->AddItem(type, str);
        gp_Pnt Lablepos;
        Standard_Real lFirst = 0, lLast = 100;
        Handle(Geom_Surface) surface;
        Handle(Geom_Curve) curve;
        Standard_Real umin, umax, vmin, vmax;
        switch (type)
        {
        case FeatureType::PLANE:
        case FeatureType::CURVED_SURFACE:
        case FeatureType::CONE:case FeatureType::CYLINDER:
            surface = BRep_Tool::Surface(TopoDS::Face(shape->Shape()));
            BRepTools::UVBounds(TopoDS::Face(shape->Shape()), umin, umax, vmin, vmax);
            //surface->Bounds(umin, umax, vmin, vmax);
            surface->D0(umin, vmin, Lablepos);
            break;
        case FeatureType::LINE:
        case FeatureType::CIRCLE:
        case FeatureType::CURVE:
        case FeatureType::ARC:
        case FeatureType::ELLIPSE:
            curve = BRep_Tool::Curve(TopoDS::Edge(shape->Shape()), lFirst, lLast);
            curve->D0(lFirst, Lablepos);
            break;
        case FeatureType::VERTEX:
            Lablepos = GetPosition(m_Shape[activeindex].size() - 1);
            break;
        default:
            break;
        }
        AddLable(Lablepos, str);
    }
    else
    {
        m_Shape.push_back({ shape });
    }
    m_AISContext->SetColor(shape, Quantity_NameOfColor::Quantity_NOC_RED, Standard_True);
    m_AISContext->Display(shape, Standard_True);
    //m_AISContext->DefaultDrawer()->SetFaceBoundaryDraw(true);
    //m_AISContext->DefaultDrawer()->SetFaceBoundaryAspect(
    //	new Prs3d_LineAspect(Quantity_NOC_CYAN, Aspect_TOL_SOLID, 1.0));
    //m_AISContext->DefaultDrawer()->SetIsoOnTriangulation(true);    //（显示线框）
    selectMode();
    //m_AISContext->UpdateCurrentViewer();
}
//点选特征的lable显示
void COccView::AddLable(gp_Pnt position,
    Standard_CString str,
    bool isSmallBlack)
{
    if (isSmallBlack)
    {
        // 1. 小黑球
        // 创建小黑球
        TopoDS_Shape sphere = BRepPrimAPI_MakeSphere(position, 0.5);
        Handle(AIS_Shape) aisSphere = new AIS_Shape(sphere);
        aisSphere->SetColor(Quantity_NOC_RED);
        m_AISContext->Display(aisSphere, Standard_True);
        m_temporaryShapes.push_back(aisSphere); // 添加到临时列表

        // 创建标签
        Handle(AIS_TextLabel) lbl = new AIS_TextLabel();
        TCollection_ExtendedString ext(str);
        lbl->SetText(ext);
        lbl->SetPosition(position.Translated(gp_Vec(0, 0, 13)));
        lbl->SetFont("Courier");
        lbl->SetHeight(13);
        lbl->SetColor(Quantity_NOC_GREEN);
        lbl->SetZoomable(false);
        m_AISContext->Display(lbl, Standard_True);
        m_temporaryShapes.push_back(lbl); // 添加到临时列表

        SetTimer(103, 8000, NULL); // 3秒后自动隐藏
        return;
    }

    // 3. 原默认大标签
    Handle(AIS_TextLabel) aLable = new AIS_TextLabel();
    TCollection_ExtendedString tostr;
    Resource_Unicode::ConvertGBToUnicode(str, tostr);
    aLable->SetText(tostr);
    aLable->SetPosition(position);
    aLable->SetZoomable(false);
    aLable->SetFont("SimHei");
    m_lableList.push_back(aLable);
    m_AISContext->Display(aLable, Standard_True);
}
//根据索引删除特征元素
// OccView的删除函数 - 修复版
void COccView::Delete(int index)
{
    if (index >= m_Shape[activeindex].size())
        return;
    m_AISContext->Remove(m_Shape[activeindex][index], Standard_True);
    m_AISContext->Remove(m_lableList[index - 1], Standard_True);
    m_Shape[activeindex].erase(m_Shape[activeindex].begin() + index);
    m_lableList.erase(m_lableList.begin() + index - 1);
    m_AISContext->UpdateCurrentViewer();
    myView->Redraw();
}

//根据索引隐藏特征元素
void COccView::Hide(int index)
{
    if (index >= m_Shape[activeindex].size())
        return;
    m_AISContext->Erase(m_Shape[activeindex][index], Standard_True);
    m_AISContext->Remove(m_lableList[index - 1], Standard_True);
    m_AISContext->UpdateCurrentViewer();
    myView->Redraw();
}
//根据索引显示特征元素
void COccView::Show(int index)
{
    if (index >= m_Shape[activeindex].size())
        return;
    m_AISContext->Display(m_Shape[activeindex][index], Standard_True);
    m_AISContext->Display(m_lableList[index - 1], Standard_True);
    m_AISContext->UpdateCurrentViewer();
    myView->Redraw();
}
void COccView::HighLight(int index)
{
    // 清除之前的选择
    m_AISContext->ClearSelected(Standard_False);

    // 恢复之前高亮形状的原始属性
    if (!m_prevHighlightedShape.IsNull()) {
        m_AISContext->SetColor(m_prevHighlightedShape, m_prevColor, Standard_False);
        m_prevHighlightedShape->SetTransparency(m_prevTransparency);
        m_prevHighlightedShape->SetDisplayMode(m_prevDisplayMode);
        m_prevHighlightedShape->SetMaterial(m_prevMaterial);
        m_AISContext->Redisplay(m_prevHighlightedShape, Standard_True);
        m_prevHighlightedShape.Nullify();
    }

    if (index >= 0 && index < m_Shape[activeindex].size()) {
        Handle(AIS_Shape) shape = m_Shape[activeindex][index];

        // 保存当前形状的原始属性
        m_prevHighlightedShape = shape;

        // 正确获取当前颜色 - 使用交互上下文的方法
        m_AISContext->Color(shape, m_prevColor);
        m_prevTransparency = shape->Transparency();
        m_prevDisplayMode = shape->DisplayMode();
        m_prevMaterial = shape->Material();

        // 直接使用RGB值定义紫色，避免常量问题
        Quantity_Color purpleColor(0.5, 0.0, 0.5, Quantity_TOC_RGB); // RGB紫色

        // 设置新的高亮属性
        m_AISContext->SetColor(shape, purpleColor, Standard_False);
        shape->SetTransparency(0.3f);
        shape->SetDisplayMode(1); // 填充模式
        shape->SetMaterial(Graphic3d_NOM_PLASTIC);
        shape->SetWidth(2.0);

        // 强制重绘
        m_AISContext->Redisplay(shape, Standard_True);
        m_AISContext->UpdateCurrentViewer();
        myView->RedrawImmediate();

        // 同时选择对象
        m_AISContext->AddOrRemoveSelected(shape, Standard_True);
    }
}
//根据索引获取对应上下文中模型的包围盒
Bnd_Box COccView::GetShapeBox(const int index)
{
    Bnd_Box box;
    BRepBndLib::Add(m_Shape[activeindex][index]->Shape(), box);
    return box;
}
//显示更新模型
void COccView::DisplayShape(Handle(AIS_Shape) aisShape)
{
    m_AISContext->Display(aisShape, Standard_True);
    m_AISContext->UpdateCurrentViewer();
    myView->Redraw();
}
//清除模型显示
void COccView::RemoveShape(Handle(AIS_Shape) aisShape)
{
    m_AISContext->Remove(aisShape, Standard_True);
    m_AISContext->UpdateCurrentViewer();
    myView->Redraw();
}
//面特征类型
FeatureType COccView::GetFaceType(TopoDS_Shape shape)
{
    if (shape.ShapeType() == TopAbs_FACE)
    {
        Handle(Geom_Surface) surface = BRep_Tool::Surface(TopoDS::Face(shape));
        ////if (surface->DynamicType() == STANDARD_TYPE(Geom_Plane))
        //if (surface->IsKind(STANDARD_TYPE(Geom_Plane)))
        //{
        //	return TRUE;
        //}
        BRepAdaptor_Surface BS(TopoDS::Face(shape), Standard_True);
        GeomAdaptor_Surface AdpSurf = BS.Surface(); GeomAbs_SurfaceType type = AdpSurf.GetType();
        switch (type)
        {
        case GeomAbs_Plane:
            return FeatureType::PLANE;
        case GeomAbs_Cylinder:
            return FeatureType::CYLINDER;
        case GeomAbs_Cone:
            return FeatureType::CONE;
        default:
            break;
        }
        Handle(Geom_BSplineSurface) bsplineSurface = Handle(Geom_BSplineSurface)::DownCast(surface);
        if (!bsplineSurface.IsNull()) {
            // 提取控制点

            TColgp_Array2OfPnt controlPoints = bsplineSurface->Poles();
        }
        if (AdpSurf.GetType() == GeomAbs_BSplineSurface)
        {
            //Handle(Geom_BSplineSurface) bsplineSurface = Handle(Geom_BSplineSurface)::DownCast(surface);
            BRepAdaptor_Surface adaptor(TopoDS::Face(shape));
            Standard_Real u = 0.5 * (adaptor.FirstUParameter() + adaptor.LastUParameter());
            Standard_Real v = 0.5 * (adaptor.FirstVParameter() + adaptor.LastVParameter());
            GeomLProp_SLProps props(surface, 2, 1e-5);
            props.SetParameters(u, v);
            Standard_Real max = props.MaxCurvature();
            Standard_Real min = props.MinCurvature();
            if (max == min && max == 0.0)
                return FeatureType::PLANE;
            //if()//圆柱面

        }
    }
    return FeatureType::CURVED_SURFACE;
}
//线特征类型
FeatureType COccView::GetEdgeType(TopoDS_Shape shape)
{
    BRepAdaptor_Curve BC(TopoDS::Edge(shape)); GeomAdaptor_Curve AdpCurve = BC.Curve(); GeomAbs_CurveType type = AdpCurve.GetType();
    TopoDS_Edge edge = TopoDS::Edge(shape);
    switch (type)
    {
    case GeomAbs_Line:
        return FeatureType::LINE;
    case GeomAbs_Circle:
        if (edge.Closed())
            return FeatureType::CIRCLE;
        else
            return FeatureType::ARC;
        break;
    case GeomAbs_Ellipse:
        return FeatureType::ELLIPSE;
    default:
        return FeatureType::CURVE;
    }

    /*if (shape.ShapeType() == TopAbs_EDGE)
    {
        Standard_Real first = 0; Standard_Real last = 100;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(TopoDS::Edge(shape), first, last);
        if (curve->DynamicType() == STANDARD_TYPE(Geom_Line))
            return TRUE;
    }*/
}
//设置当前只允许选择的特征类型
void COccView::ActivateFeature(FeatureBaseType type)
{
    m_AISContext->Deactivate();
    switch (type)
    {
    case FeatureBaseType::FACE:
        m_AISContext->Activate(AIS_Shape::SelectionMode(TopAbs_FACE));
        break;
    case FeatureBaseType::EDGE:
        m_AISContext->Activate(AIS_Shape::SelectionMode(TopAbs_EDGE));
        break;
    case FeatureBaseType::VERTEX:
        m_AISContext->Activate(AIS_Shape::SelectionMode(TopAbs_VERTEX));
        break;
    default:
        break;
    }
    ActiveSelect.SetActiveBaseSelect(type);
}
//调整视图角度
void COccView::SetTopView()
{
    myView->SetProj(V3d_Zpos); 
    myView->Redraw();
    RedrawLableLine();
}

void COccView::SetSideView()
{
    myView->SetProj(V3d_Xneg); 
    myView->Redraw();
    RedrawLableLine();
}
void COccView::SetFrontView()
{
    myView->SetProj(V3d_Yneg); 
    myView->Redraw();
    RedrawLableLine();
}
//将计算和收集的数据生成pdf
void COccView::SetPDF(bool bOpen)
{
    if (!m_isCalculated) return;
    CString path = (GetAppPath()).c_str();
    CString num;
    num.Format(_T("\\png\\biasSnap.png"));
    path += num;
    CaptureScreenPng(path);
    Sleep(100);

    pdfMgr->CreatePDF(table, ptTable, bOpen);
    table.clear(); ptTable.clear();


    //MessageBox("请在结束一轮扫描后，按暂停生成报告！");
}
//屏幕截图并存储
//屏幕截图并存储 - 只截取软件窗口
bool COccView::CaptureScreenPng(CString Path)
{
    CRect clientRect;
    GetClientRect(&clientRect);  // 获取客户区矩形

    // 创建兼容DC
    CClientDC clientDC(this);
    CDC memDC;
    memDC.CreateCompatibleDC(&clientDC);

    // 创建兼容位图
    CBitmap bitmap;
    bitmap.CreateCompatibleBitmap(&clientDC, clientRect.Width(), clientRect.Height());

    // 选择位图到内存DC
    CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

    // 将客户区内容复制到内存DC
    memDC.BitBlt(0, 0, clientRect.Width(), clientRect.Height(), &clientDC, 0, 0, SRCCOPY);

    // 创建CImage并保存
    CImage image;
    image.Create(clientRect.Width(), clientRect.Height(), 32);  // 32位色深

    // 将位图数据复制到CImage
    BITMAP bmpInfo;
    bitmap.GetBitmap(&bmpInfo);

    // 获取位图数据
    BYTE* pBits = new BYTE[bmpInfo.bmWidthBytes * bmpInfo.bmHeight];
    bitmap.GetBitmapBits(bmpInfo.bmWidthBytes * bmpInfo.bmHeight, pBits);

    // 将数据复制到CImage
    for (int y = 0; y < clientRect.Height(); y++)
    {
        for (int x = 0; x < clientRect.Width(); x++)
        {
            BYTE* pPixel = pBits + (y * bmpInfo.bmWidthBytes) + (x * 4);
            image.SetPixel(x, y, RGB(pPixel[2], pPixel[1], pPixel[0]));  // BGR转RGB
        }
    }

    // 保存图片
    image.Save(Path, Gdiplus::ImageFormatPNG);

    // 清理资源
    delete[] pBits;
    memDC.SelectObject(pOldBitmap);
    image.Destroy();

    return true;
}

void COccView::MakeBackgroundTransparent(CImage& img, COLORREF bgColor)
{
    for (int y = 0; y < img.GetHeight(); y++)
    {
        for (int x = 0; x < img.GetWidth(); x++)
        {
            COLORREF pixelColor = img.GetPixel(x, y);
            if (pixelColor == bgColor)
            {
                //img.SetTransparentColor(pixelColor);
                img.SetPixel(x, y, CLR_INVALID); // CLR_INVALID makes the pixel transparent
            }
        }
    }
}
//探针复位，现停用
void COccView::Translate(gp_Pnt newpos)
{
    newpos.SetZ(newpos.Z() + 20.0);
    //gp_Vec dir(oldpos, newpos);
    //gp_Trsf translate; translate.SetTranslation(dir);
    //BRepBuilderAPI_Transform transform(m_Sphere.back()->Shape(), translate);
    //Handle(AIS_Shape) newshape = new AIS_Shape(transform.Shape());
    TopoDS_Shape shape = BRepPrimAPI_MakeSphere(newpos, 1.0);
    m_AISContext->Remove(m_Sphere.back(), Standard_True);
    m_Sphere.back().Nullify(); m_Sphere.pop_back();
    Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
    m_Sphere.push_back(aisShape);
    //m_Sphere.back()->SetShape(shape);

    //m_AISContext->Update(movesphere, Standard_True);
    m_AISContext->Display(m_Sphere.back(), Standard_True);
    shape.Nullify(); aisShape.Nullify();
    Standard::Purge();
}
//屏幕点转三维点
gp_Pnt COccView::Get3dPos(double x, double y)
{
    gp_Pnt point = { 0,0,0 };
    gp_Pnt p0;
    gp_Dir v0;
    double X, Y, Z, VX, VY, VZ;
    myView->Convert((int)x, (int)y, X, Y, Z);
    myView->Proj(VX, VY, VZ);
    p0.SetCoord(X, Y, Z); v0.SetCoord(-VX, -VY, -VZ);
    gp_Lin lin(p0, v0);
    //Handle(Geom_TrimmedCurve) curve = GC_MakeSegment(lin, -10000, 10000);
    IntCurvesFace_ShapeIntersector dist;
    dist.Load(m_Shape[activeindex][0]->Shape(), 0.1); dist.Perform(lin, -10000, 10000);
    if (dist.IsDone() && dist.NbPnt() > 0)
    {
        point = dist.Pnt(1);
        /*Handle(AIS_Shape) clickpos = new AIS_Shape(BRepBuilderAPI_MakeVertex(point));
        clickpos->SetColor(Quantity_NameOfColor::Quantity_NOC_BLACK);
        m_AISContext->Display(clickpos, Standard_True);*/
    }
    return point;
}
//计算偏差值，返回面轮廓度
double COccView::ComparePoints(vector<gp_Pnt> collectedPoints)
{
    Standard_Real rms = 0.0; Standard_Real average = 0.0; Standard_Real dev = 0.0;
    Standard_Real discrete = 0.0; Standard_Real positive_ave = 0.0; Standard_Real negative_ave = 0.0;
    Standard_Integer negative_count = 0; Standard_Real minBias = 1, maxBias = -1;
    int pCount = 0, posSize = m_position.size();
    vector<Standard_Real> biaslist;
    for (int i = 0; i < posSize; i++)
    {
        gp_Pnt point = m_position[i];
        gp_Pnt collectedPnt = collectedPoints[i];
        Standard_Real bias = collectedPnt.Distance(point);
        if (point.Z() - collectedPnt.Z() < 0)
        {
            bias = -bias;
            negative_ave += bias;
            negative_count++;
        }
        else
            positive_ave += bias;
        vector<std::string> data;
        ConvertToString(data, bias, point, collectedPnt);
        ptTable.push_back(data);
        average += bias; minBias = min(bias, minBias); maxBias = max(bias, maxBias);
        biaslist.push_back(bias);
    }
    vector<Standard_Real> dataList; dataList.reserve(8);
    average /= posSize; positive_ave /= (posSize - negative_count); negative_ave /= negative_count;
    Standard_Real tol = max(abs(maxBias), abs(minBias));
    {
        std::lock_guard<std::mutex> lock(g_mtx);
        for (int i = 0; i < biaslist.size(); i++)
        {
            Standard_Real distance = biaslist[i];
            rms += distance * distance; dev += (distance - average) * (distance - average);
        }
        rms = sqrt(rms) / posSize; dev = sqrt(dev) / posSize;
    }
    dataList.push_back(minBias); dataList.push_back(maxBias); dataList.push_back(average);
    dataList.push_back(rms); dataList.push_back(dev); dataList.push_back(0.0); dataList.push_back(positive_ave); dataList.push_back(negative_ave);
    ConvertToList(table, dataList);
    m_ifPause = true;
    if (table.size())
    {
        pdfMgr->CreatePDF(table, ptTable);
        //table.clear(); ptTable.clear();
    }
    m_ifPause = false; table.clear(); ptTable.clear();
    return 2 * tol;
}
//以寻找点集最近点的方式计算点集误差并返回面轮廓度
double COccView::SearchNstPoints()
{
	if (m_listRgn.size())
	{
		m_listRgn.clear();
	}

	static Standard_Real rms = 0.0; static Standard_Real average = 0.0; static Standard_Real dev = 0.0;
	static Standard_Real discrete = 0.0; static Standard_Real positive_ave = 0.0; static Standard_Real negative_ave = 0.0;
	static Standard_Integer negative_count = 0; static Standard_Real minBias = 1, maxBias = -1;
	int pCount = 0;
	vector<vector<gp_Pnt>> pos = SortLocation(m_position);
	Standard_Integer index; m_listRgn.reserve(m_position.size());
	int posSize = pos.size();
	for (int i = 0; i < posSize; i++)
	{
		for (int j = 0; j < pos[i].size(); j++)
		{
			gp_Pnt point = pos[i][j];

			gp_Pnt nst_point = SearchNearestPoint(point);

			TopoDS_Shape shape = BRepPrimAPI_MakeSphere(nst_point, 1.0);
			Handle(AIS_Shape) aisshape = new AIS_Shape(shape);
			m_Sphere.push_back(aisshape);
			m_AISContext->Display(aisshape, Standard_True);

			RgnBox rgn; rgn.Set3dLoc(nst_point);
			CreateLable(rgn, i, j, posSize);
			m_listRgn.push_back(rgn);
			Standard_Real bias = nst_point.Distance(point);
			if (point.Z() - nst_point.Z() < 0)
			{
				bias = -bias;
				negative_ave += bias;
				negative_count++;
			}
			else
				positive_ave += bias;
			vector<std::string> data;
			ConvertToString(data, bias, nst_point, point);
			ptTable.push_back(data);
			average += bias; minBias = min(bias, minBias); maxBias = max(bias, maxBias);
			m_listRgn.back().SetText(++pCount, bias);
		}
	}
	vector<Standard_Real> dataList; dataList.reserve(8);
	average /= posSize; positive_ave /= (posSize - negative_count); negative_ave /= negative_count;
	Standard_Real tol = max(abs(maxBias), abs(minBias));
	{
		std::lock_guard<std::mutex> lock(g_mtx);
		for (int i = 0; i < m_listRgn.size(); i++)
		{
			Standard_Real distance = m_listRgn[i].GetDistance();
			rms += distance * distance; dev += (distance - average) * (distance - average);
			m_listRgn[i].SetFillColor(TransToClr(distance, tol));
			m_listRgn[i].DisplayAll(m_AISContext);
			//m_listRgn[i].Remove(m_AISContext);
		}
		rms = sqrt(rms) / posSize; dev = sqrt(dev) / posSize;
		ShowColorScale("U(mm)", -tol, tol, true);
	}
	dataList.push_back(minBias); dataList.push_back(maxBias); dataList.push_back(average);
	dataList.push_back(rms); dataList.push_back(dev); dataList.push_back(0.0); dataList.push_back(positive_ave); dataList.push_back(negative_ave);
	ConvertToList(table, dataList);
	rms = 0.0; average = 0.0; dev = 0.0; discrete = 0.0; positive_ave = 0.0; negative_ave = 0.0; negative_count = 0;
	minBias = 1, maxBias = -1;
    //自动生成PDF报告
    //if (m_bAutoGeneratePDF && !table.empty() && !ptTable.empty())
    //{
    //    // 静默生成PDF，不打开
    //    CString path = (GetAppPath()).c_str();
    //    CString num;
    //    num.Format(_T("\\png\\biasSnap.png"));
    //    path += num;
    //    CaptureScreenPng(path);
    //    Sleep(100);

        // 判断是否是第一次生成
     /*   bool bOpenPDF = m_bFirstTimeGeneratePDF;*/
    CString path = (GetAppPath()).c_str();
    CString num;
    num.Format(_T("\\png\\biasSnap.png"));
    path += num;
    CaptureScreenPng(path);
        // 调用PDF生成，第一次弹出，后续不弹出
        pdfMgr->CreatePDF(table, ptTable, false);

        //// 更新第一次标志
        //if (m_bFirstTimeGeneratePDF) {
        //    m_bFirstTimeGeneratePDF = false;
        //}
   /* }*/

    return 2 * tol;
}
//输入点集的排序
// 修改 SortLocation 函数，返回带原始索引的点集
vector<vector<gp_Pnt>> COccView::SortLocation(vector<gp_Pnt> pos)
{
    vector<vector<gp_Pnt>> loc;
    sort(pos.begin(), pos.end(), [](gp_Pnt p1, gp_Pnt p2) {return p1.Y() > p2.Y(); });
    int row = 0;
    for (auto pt : pos)
    {
        if (loc.size() > row && loc[row].size())
        {
            if (abs(pt.Y() - loc[row].back().Y()) > 1.0)
            {
                sort(loc[row].begin(), loc[row].end(), [](gp_Pnt p1, gp_Pnt p2) {return p1.X() > p2.X(); });
                ++row;
            }
        }
        if (loc.size() <= row)
        {
            loc.resize(row + 1);
        }
        loc[row].push_back(pt);
    }
    if (loc[row].size())
    {
        sort(loc[row].begin(), loc[row].end(), [](gp_Pnt p1, gp_Pnt p2) {return p1.X() > p2.X(); });
    }
    return loc;
}
//获取面上点对应的uv值
void COccView::GetPointsUV(Handle(Geom_Surface) surface, gp_Pnt point, Standard_Real& u, Standard_Real& v)
{
    float maxDistance = Precision::Confusion();
    GeomLib_Tool parameterTool;
    parameterTool.Parameters(surface, point, maxDistance, u, v);
}
//获取点在模型面上的投影点位置
void COccView::PointOnFace(gp_Pnt& point, gp_Dir dir)
{
    IntCurvesFace_ShapeIntersector dist;
    gp_Lin line(point, -dir);
    dist.Load(m_Shape[activeindex][0]->Shape(), 0.1); dist.Perform(line, -10000, 10000);
    if (dist.IsDone() && dist.NbPnt() > 0)
    {
        point = dist.Pnt(1);
    }
}
//获取鼠标选中的面特征
TopoDS_Face COccView::GetSelectFace(int x, int y)
{
    TopoDS_Face face;
    gp_Pnt p0; gp_Dir v0;
    double X, Y, Z, VX, VY, VZ;
    myView->Convert((int)x, (int)y, X, Y, Z);
    myView->Proj(VX, VY, VZ);
    p0.SetCoord(X, Y, Z); v0.SetCoord(-VX, -VY, -VZ);
    gp_Lin lin(p0, v0);
    IntCurvesFace_ShapeIntersector dist;
    dist.Load(m_Shape[activeindex][0]->Shape(), 0.1); dist.Perform(lin, -10000, 10000);
    if (dist.IsDone() && dist.NbPnt() > 0)
    {
        face = dist.Face(1);
    }
    return face;
}
//删除采样点显示
//面上的均匀采样
void COccView::FaceSampling(TopoDS_Face face)
{
    RemoveSamplingShape();
    Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
    Standard_Real umin, umax, vmin, vmax;
    Standard_Real u0, u1, v0, v1;
    surface->Bounds(u0, u1, v0, v1);
    BRepTools::UVBounds(face, umin, umax, vmin, vmax);
    Standard_Real ustep = (umax - umin) / 15, vstep = (vmax - vmin) / 15;
    //Standard_Real ustep = 0.1, vstep = 0.1;
    //TColgp_Array2OfPnt points(1, (umax - umin) / ustep + 1, 1, (vmax - vmin) / vstep + 1);
    BRepClass3d_SolidClassifier classifier(face);
    TopoDS_Compound compound; BRep_Builder builder; builder.MakeCompound(compound);
    for (Standard_Real u = umin; u <= umax; u += ustep)
    {
        for (Standard_Real v = vmin; v <= vmax; v += vstep)
        {
            gp_Pnt pt = surface->Value(u, v);
            classifier.Perform(pt, 1e-5);
            if (classifier.State() == TopAbs_ON)
            {
                TopoDS_Shape shape = BRepBuilderAPI_MakeVertex(pt);
                builder.Add(compound, shape);
            }
            //points.SetValue((u - umin) / ustep, (v - vmin) / vstep, pt);
        }
    }
    m_samplingShape = new AIS_Shape(compound);
    m_AISContext->Display(m_samplingShape, Standard_True);
    /*IMeshData::Array1OfVertexOfDelaun vertices(1, 121);
    for (int i = 1; i <= 11; i++)
    {
        for (int j = 1; j <= 11; j++)
        {
            vertices.SetValue((i - 1) * 11 + j, BRepMesh_Vertex(umin + (i - 1) * ustep, vmin + (j - 1) * vstep, BRepMesh_OnSurface));
        }
    }
    BRepMesh_Delaun triangulation(vertices);
    Handle(BRepMesh_DataStructureOfDelaun) meshData = triangulation.Result();
    for (Standard_Integer i = 1; i < meshData.get()->NbNodes(); i += 3)
    {
        Standard_Integer index1, index2, index3;
        gp_XY p1 = triangulation.GetVertex(i).Coord(); gp_XY p2 = triangulation.GetVertex(i + 1).Coord(); gp_XY p3 = triangulation.GetVertex(i + 2).Coord();
        gp_Pnt point1 = surface->Value(p1.X(), p1.Y()); gp_Pnt point2 = surface->Value(p2.X(), p2.Y()); gp_Pnt point3 = surface->Value(p3.X(), p3.Y());
        TopoDS_Shape triangle = BRepBuilderAPI_MakePolygon(point1, point2, point3, true);
        Handle(AIS_Shape) aisTriangle = new AIS_Shape(triangle);
        m_AISContext->Display(aisTriangle, Standard_False);
        TopoDS_Shape shape1 = BRepBuilderAPI_MakeVertex(point1);
        Handle(AIS_Shape) aisPoint1 = new AIS_Shape(shape1);
        m_AISContext->Display(aisPoint1, Standard_False);
        TopoDS_Shape shape2 = BRepBuilderAPI_MakeVertex(point2);
        Handle(AIS_Shape) aisPoint2 = new AIS_Shape(shape2);
        m_AISContext->Display(aisPoint2, Standard_False);
        TopoDS_Shape shape3 = BRepBuilderAPI_MakeVertex(point3);
        Handle(AIS_Shape) aisPoint3 = new AIS_Shape(shape3);
        m_AISContext->Display(aisPoint3, Standard_False);
    }*/
}

//TopoDS_Compound COccView::MeasureCone(int index, Standard_Boolean isClockWise, Standard_Real startDepth, Standard_Real endDepth, Standard_Real angle, Standard_Integer pointCount, Standard_Integer pathCount)
//{
//	RemoveSamplingShape();
//	TopoDS_Shape shape = m_Shape[activeindex][index]->Shape();
//	TopoDS_Face face = TopoDS::Face(shape);
//	Handle(Geom_ConicalSurface) cone = Handle(Geom_ConicalSurface)::DownCast(BRep_Tool::Surface(face));
//	
//	Standard_Real ufirst, ulast, vfirst, vlast;
//	BRepTools::UVBounds(face, ufirst, ulast, vfirst, vlast);
//	vlast -= endDepth; vfirst += startDepth;
//	//gp_Dir normal = cylinder->Axis().Direction();
//	Standard_Real uStep = (ulast - ufirst) / (pointCount - 1);
//	Standard_Real vStep = (vlast - vfirst) / (pathCount - 1);
//	TopoDS_Compound compound; BRep_Builder builder; builder.MakeCompound(compound);
//	if (isClockWise)//顺时针
//	{
//		for (int j = 0; j < pathCount; j++)
//		{
//			for (int i = 0; i < pointCount; i++)
//			{
//				gp_Pnt pt;
//				if (isClockWise)
//					cylinder->D0(ulast - i * uStep, vfirst + vStep * j, pt);
//				TopoDS_Shape shape = BRepBuilderAPI_MakeVertex(pt);
//				builder.Add(compound, shape);
//			}
//		}
//	}
//	else
//	{
//		for (int j = 0; j < pathCount; j++)
//		{
//			for (int i = 0; i < pointCount; i++)
//			{
//				gp_Pnt pt;
//				cylinder->D0(ufirst + i * uStep, vfirst + vStep * j, pt);
//				//pt.SetX(pt.X() + normal.X() * depth); pt.SetY(pt.Y() + normal.Y() * depth); pt.SetZ(pt.Z() + normal.Z() * depth);
//				TopoDS_Shape shape = BRepBuilderAPI_MakeVertex(pt);
//				builder.Add(compound, shape);
//			}
//		}
//	}
//	return compound;
//}
//圆柱面测量采样
void COccView::MeasureCYL(int index, Standard_Boolean isClockWise, Standard_Real startDepth, Standard_Real endDepth, Standard_Real angle, Standard_Integer pointCount, Standard_Integer pathCount)
{
    RemoveSamplingShape();
    g_previewPoints.clear();
    TopoDS_Shape shape = m_Shape[activeindex][index]->Shape();
    TopoDS_Face face = TopoDS::Face(shape);
    //Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
    Handle(Geom_CylindricalSurface) cylinder = Handle(Geom_CylindricalSurface)::DownCast(BRep_Tool::Surface(face));
    Standard_Real ufirst, ulast, vfirst, vlast;
    BRepTools::UVBounds(face, ufirst, ulast, vfirst, vlast);
    vlast -= endDepth; vfirst += startDepth;
    Standard_Real uStep = (ulast - ufirst) / (pointCount - 1);
    Standard_Real vStep = (vlast - vfirst) / (pathCount - 1);
    TopoDS_Compound compound; BRep_Builder builder; builder.MakeCompound(compound);
    if (isClockWise)//顺时针
    {
        for (int j = 0; j < pathCount; j++)
        {
            for (int i = 0; i < pointCount; i++)
            {
                gp_Pnt pt;
                cylinder->D0(ulast - i * uStep, vfirst + vStep * j, pt);
                g_previewPoints.push_back(pt);
                TopoDS_Shape shape = BRepBuilderAPI_MakeVertex(pt);
                builder.Add(compound, shape);
            }
        }
    }
    else
    {
        for (int j = 0; j < pathCount; j++)
        {
            for (int i = 0; i < pointCount; i++)
            {
                gp_Pnt pt;
                cylinder->D0(ufirst + i * uStep, vfirst + vStep * j, pt);
                g_previewPoints.push_back(pt);
                //pt.SetX(pt.X() + normal.X() * depth); pt.SetY(pt.Y() + normal.Y() * depth); pt.SetZ(pt.Z() + normal.Z() * depth);
                TopoDS_Shape shape = BRepBuilderAPI_MakeVertex(pt);
                builder.Add(compound, shape);
            }
        }
    }
    m_samplingShape = new AIS_Shape(compound);
    m_AISContext->Display(m_samplingShape, Standard_True);
}
//平面测量采样
void COccView::MeasurePln(int index, Standard_Integer uCount, Standard_Integer vCount, Standard_Real ustart, Standard_Real vstart, Standard_Real uend, Standard_Real vend)
{
    RemoveSamplingShape();
    g_previewPoints.clear();
    TopoDS_Shape shape = m_Shape[activeindex][index]->Shape();
    TopoDS_Face face = TopoDS::Face(shape);
    Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
    Standard_Real umin, umax, vmin, vmax;
    Standard_Real u0, u1, v0, v1;
    surface->Bounds(u0, u1, v0, v1);
    BRepTools::UVBounds(face, umin, umax, vmin, vmax);
    umin += ustart; umax -= uend; vmin += vstart; vmax -= vend;
    Standard_Real ustep = (umax - umin) / (uCount - 1), vstep = (vmax - vmin) / (vCount - 1);

    BRepClass3d_SolidClassifier classifier(face);
    TopoDS_Compound compound; BRep_Builder builder; builder.MakeCompound(compound);
    for (Standard_Real u = umin; u <= umax; u += ustep)
    {
        for (Standard_Real v = vmin; v <= vmax; v += vstep)
        {
            gp_Pnt pt = surface->Value(u, v);
            classifier.Perform(pt, 1e-5);
            if (classifier.State() == TopAbs_ON)
            {
                TopoDS_Shape shape = BRepBuilderAPI_MakeVertex(pt);
                g_previewPoints.push_back(pt);
                builder.Add(compound, shape);
            }
        }
    }
    m_samplingShape = new AIS_Shape(compound);
    m_AISContext->Display(m_samplingShape, Standard_True);

}
//圆形特征测量采样
void COccView::MeasureCir(int index, Standard_Real start, int pointCount, Standard_Boolean isClockWise, Standard_Real depth)
{
    RemoveSamplingShape();
    g_previewPoints.clear();
    TopoDS_Shape shape = m_Shape[activeindex][index]->Shape();
    TopoDS_Edge edge = TopoDS::Edge(shape); Standard_Real first = 0; Standard_Real last = 1;
    //Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
    Handle(Geom_Circle) cir = Handle(Geom_Circle)::DownCast(BRep_Tool::Curve(edge, first, last));
    gp_Dir normal = cir->Axis().Direction();
    Standard_Real ustep = (last - first) / (pointCount - 1);
    TopoDS_Compound compound; BRep_Builder builder; builder.MakeCompound(compound);
    if (isClockWise)
    {
        for (int i = 0; i < pointCount; i++)
        {
            gp_Pnt pt;
            cir->D0(last - i * ustep, pt);
            pt.SetX(pt.X() + normal.X() * depth); pt.SetY(pt.Y() + normal.Y() * depth); pt.SetZ(pt.Z() + normal.Z() * depth);
            g_previewPoints.push_back(pt);
            TopoDS_Shape shape = BRepBuilderAPI_MakeVertex(pt);
            builder.Add(compound, shape);
        }
    }
    else
    {
        for (int i = 0; i < pointCount; i++)
        {
            gp_Pnt pt;
            cir->D0(first + i * ustep, pt);
            pt.SetX(pt.X() + normal.X() * depth); pt.SetY(pt.Y() + normal.Y() * depth); pt.SetZ(pt.Z() + normal.Z() * depth);
            g_previewPoints.push_back(pt);
            TopoDS_Shape shape = BRepBuilderAPI_MakeVertex(pt);
            builder.Add(compound, shape);
        }
    }
    m_samplingShape = new AIS_Shape(compound);
    m_AISContext->Display(m_samplingShape, Standard_True);
}
//弧形特征测量采样
void COccView::MeasureARC(int index, int pointCount, Standard_Real start, Standard_Real end, Standard_Boolean isClockWise, Standard_Real depth)
{
    RemoveSamplingShape();
    g_previewPoints.clear();
    TopoDS_Shape shape = m_Shape[activeindex][index]->Shape();
    TopoDS_Edge edge = TopoDS::Edge(shape); Standard_Real first = 0; Standard_Real last = 1;
    //Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
    Handle(Geom_Circle) cir = Handle(Geom_Circle)::DownCast(BRep_Tool::Curve(edge, first, last));
    int num = start / (last - first); start -= num * (last - first);
    num = end / (last - first); end -= num * (last - first);
    first += start; last -= end;
    gp_Dir normal = cir->Axis().Direction();
    Standard_Real ustep = (last - first) / (pointCount - 1);
    TopoDS_Compound compound; BRep_Builder builder; builder.MakeCompound(compound);
    if (isClockWise)
    {
        for (int i = 0; i < pointCount; i++)
        {
            gp_Pnt pt;
            cir->D0(start + 2 * PI - i * ustep, pt);
            pt.SetX(pt.X() + normal.X() * depth); pt.SetY(pt.Y() + normal.Y() * depth); pt.SetZ(pt.Z() + normal.Z() * depth);
            g_previewPoints.push_back(pt);
            TopoDS_Shape shape = BRepBuilderAPI_MakeVertex(pt);
            builder.Add(compound, shape);
        }
    }
    else
    {
        for (int i = 0; i < pointCount; i++)
        {
            gp_Pnt pt;
            cir->D0(start + i * ustep, pt);
            pt.SetX(pt.X() + normal.X() * depth); pt.SetY(pt.Y() + normal.Y() * depth); pt.SetZ(pt.Z() + normal.Z() * depth);
            g_previewPoints.push_back(pt);
            TopoDS_Shape shape = BRepBuilderAPI_MakeVertex(pt);
            builder.Add(compound, shape);
        }
    }
    m_samplingShape = new AIS_Shape(compound);
    m_AISContext->Display(m_samplingShape, Standard_True);
}
//线型测量采样
void COccView::MeasureLine(int index, Standard_Real start, Standard_Real end, int pointCount, gp_Dir deepDir, Standard_Real depth)
{
    RemoveSamplingShape();
    g_previewPoints.clear();
    TopoDS_Shape shape = m_Shape[activeindex][index]->Shape();
    TopoDS_Edge edge = TopoDS::Edge(shape); Standard_Real first = 0; Standard_Real last = 1;
    Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
    first += start; last -= end;
    Standard_Real ustep = (last - first) / (pointCount - 1);
    TopoDS_Compound compound; BRep_Builder builder; builder.MakeCompound(compound);
    for (int i = 0; i < pointCount; i++)
    {
        gp_Pnt pt;
        curve->D0(first + i * ustep, pt);
        pt.SetX(pt.X() + deepDir.X() * depth); pt.SetY(pt.Y() + deepDir.Y() * depth); pt.SetZ(pt.Z() + deepDir.Z() * depth);
        g_previewPoints.push_back(pt);
        TopoDS_Shape shape = BRepBuilderAPI_MakeVertex(pt);
        builder.Add(compound, shape);
    }
    m_samplingShape = new AIS_Shape(compound);
    m_AISContext->Display(m_samplingShape, Standard_True);
}
//添加确认测量采样的点集
void COccView::AddSamplingPoints()
{
    m_samplingPoints.push_back(g_previewPoints);
    g_previewPoints.clear();
}
//取消测量，清除测量点集
void COccView::RemoveSamplingShape()
{
    if (m_samplingShape)
    {
        m_AISContext->Remove(m_samplingShape, Standard_True);
        m_samplingShape.Nullify();
    }
    Invalidate(false);
}
//面特征三角化
void COccView::FaceTriangulation(TopoDS_Face face)
{
    Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
    Handle(Standard_Type) type = surface->DynamicType();
    /*Handle(Geom_BSplineSurface) plane = GeomConvert::SurfaceToBSplineSurface(surface);
    Handle(Adaptor3d_Surface) adaptorsurface = new GeomAdaptor_Surface(surface);*/
    Standard_Real umin, umax, vmin, vmax;
    BRepTools::UVBounds(face, umin, umax, vmin, vmax);
    Handle(Adaptor3d_Surface) adaptorsurface = new GeomAdaptor_Surface(surface);
    Standard_Real ustep = 0.05, vstep = 0.05;

    //IntPatch_Polyhedron polyface(adaptorsurface,(umax-umin)/ustep, (vmax - vmin) / vstep);	
    IntPatch_Polyhedron polyface(adaptorsurface, 20, 12);
    for (Standard_Integer i = 1; i <= polyface.NbTriangles(); i++)
    {
        Standard_Integer index1, index2, index3;
        polyface.Triangle(i, index1, index2, index3);
        gp_Pnt point1 = polyface.Point(index1); gp_Pnt point2 = polyface.Point(index2); gp_Pnt point3 = polyface.Point(index3);
        TopoDS_Shape triangle = BRepBuilderAPI_MakePolygon(point1, point2, point3, true);
        Handle(AIS_Shape) aisTriangle = new AIS_Shape(triangle);
        m_AISContext->Display(aisTriangle, Standard_False);
        TopoDS_Shape shape1 = BRepBuilderAPI_MakeVertex(point1);
        Handle(AIS_Shape) aisPoint1 = new AIS_Shape(shape1);
        m_AISContext->Display(aisPoint1, Standard_False);
        TopoDS_Shape shape2 = BRepBuilderAPI_MakeVertex(point2);
        Handle(AIS_Shape) aisPoint2 = new AIS_Shape(shape2);
        m_AISContext->Display(aisPoint2, Standard_False);
        TopoDS_Shape shape3 = BRepBuilderAPI_MakeVertex(point3);
        Handle(AIS_Shape) aisPoint3 = new AIS_Shape(shape3);
        m_AISContext->Display(aisPoint3, Standard_False);
    }
}
//面型特征网格化
void COccView::FaceMesh(TopoDS_Face face)
{
    TopExp_Explorer explore(m_Shape[activeindex][0]->Shape(), TopAbs_FACE);
    for (; explore.More(); explore.Next())
    {
        TopoDS_Face face = TopoDS::Face(explore.Current());
        FaceTriangulation(face);
    }
}
//设置透明度
void COccView::SetTransparent()
{
    if (m_Shape.size() && m_Shape[activeindex].size())
    {
        m_Shape[activeindex][0]->SetTransparency(0.5);
        myView->Redraw();
    }
}

void COccView::SetSolid()
{
    if (m_Shape.size() && m_Shape[activeindex].size())
    {
        m_Shape[activeindex][0]->SetTransparency(0.0);
        ActiveSelect.SetActiveBaseSelect(FeatureBaseType::NONE);
        myView->Redraw();
    }
}
//根据输入建立坐标系
void COccView::CreateCoordinate(gp_Ax2 anAxis)
{
    if (!m_Trihedron.IsNull() && m_AISContext->IsDisplayed(m_Trihedron))
        m_AISContext->Remove(m_Trihedron, Standard_True);

    m_trihedronAxis = new Geom_Axis2Placement(anAxis);
    m_Trihedron = new AIS_Trihedron(m_trihedronAxis);

    Standard_Real size = 200.0f;
    Bnd_Box box = GetShapeBox();
    Standard_Real xmin, xmax, ymin, ymax, zmin, zmax;
    box.Get(xmin, ymin, zmin, xmax, ymax, zmax);
    Standard_Real maxborder = Max(Max(xmax - xmin, ymax - ymin), zmax - zmin);
    m_Trihedron->SetSize(size * maxborder / 4000);

    m_Trihedron->SetXAxisColor(Quantity_NOC_RED);
    m_Trihedron->SetYAxisColor(Quantity_NOC_GREEN);
    m_Trihedron->SetAxisColor(Quantity_NOC_BLUE1);
    m_Trihedron->SetDatumDisplayMode(Prs3d_DM_Shaded);
    m_AISContext->Display(m_Trihedron, Standard_True);
    m_CurrentCoordinate = anAxis;


}

// 删除当前坐标系（视图 + 树控件 + 映射）
void COccView::DeleteCoordinate()
{
    // 1. 从OCC视图中移除坐标系
    if (!m_Trihedron.IsNull() && m_AISContext->IsDisplayed(m_Trihedron))
    {
        m_AISContext->Remove(m_Trihedron, Standard_True);
        m_Trihedron.Nullify();
    }

}
//坐标系重建
void COccView::CoordConversion(gp_Ax2 anAxis)
{
    TopoDS_Shape oldshape = m_Shape[activeindex][0]->Shape();
    gp_Ax3 oldcoord(gp_Pnt(0, 0, 0), gp_Dir(0, 0, 1), gp_Dir(1, 0, 0));
    gp_Ax3 newcoord(anAxis);
    m_AISContext->RemoveAll(Standard_True);
    CreateCoordinate(oldcoord.Ax2());
    g_buildflag = TRUE;
    m_Shape[activeindex].clear(); m_lableList.clear();
    //gp_Ax1 rotateAxis1(anAxis.Axis());
    gp_Trsf transform = gp_Trsf();
    transform.SetDisplacement(newcoord, oldcoord);
    BRepBuilderAPI_Transform trans(oldshape, transform);
    TopoDS_Shape newshape = trans.Shape();
    Handle(AIS_Shape) aisShape = new AIS_Shape(newshape);
    aisShape->SetTransparency(0.9);
    //m_AISContext->Remove(m_Shape[activeindex][0], Standard_True);
    Add(aisShape);
    FitAll();
}


//面型特征参数获取
void COccView::GetPlaneParameter(int index, gp_Pnt& location, gp_Dir& direction)
{
    TopoDS_Shape shape = m_Shape[activeindex][index]->Shape();
    TopoDS_Face face = TopoDS::Face(shape);
    Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
    Standard_Real umin, umax, vmin, vmax;
    BRepTools::UVBounds(face, umin, umax, vmin, vmax);
    gp_Vec du, dv;
    surface->D1((umin + umax) / 2, (vmin + vmax) / 2, location, du, dv);//surface->Axis().Direction();
    direction = du ^ dv;
    if (face.Orientation() == TopAbs_REVERSED)
        direction.Reverse();
}
//根据索引获取线型元素的方向
gp_Dir COccView::GetLineDir(int index)
{
    TopoDS_Shape shape = m_Shape[activeindex][index]->Shape();
    if (shape.ShapeType() != TopAbs_EDGE)
        return gp_Dir(0.0, 0.0, 0.0);
    TopoDS_Edge curve = TopoDS::Edge(shape);
    Standard_Real first = 0; Standard_Real last = 100;
    Handle(Geom_Line) line = Handle(Geom_Line)::DownCast(BRep_Tool::Curve(curve, first, last));
    gp_Dir dir = line->Lin().Direction();
    return dir;
}

//根据索引获取点型元素的位置
gp_Pnt COccView::GetPosition(int index)
{
    TopoDS_Shape shape = m_Shape[activeindex][index]->Shape();
    TopoDS_Vertex point = TopoDS::Vertex(shape);
    gp_Pnt pos = BRep_Tool::Pnt(point);
    return pos;
}
//根据索引获取上下文的模型
TopoDS_Shape COccView::GetShape(int index)
{
    TopoDS_Shape shape;
    if (index < m_Shape[activeindex].size())
        shape = m_Shape[activeindex][index]->Shape();
    return shape;
}
//检查上下文是否为空
Standard_Boolean COccView::isEmpty()
{
    if (activeindex >= 0 && m_Shape.size() > activeindex)
        return FALSE;
    return TRUE;
}
//允许鼠标点选特征
void COccView::SwitchToMousePicking()
{
    m_MousePicking = TRUE;
}
//禁止鼠标点选特征
void COccView::PreventMousePicking()
{
    m_MousePicking = FALSE;
}
//鼠标点选回调函数
void COccView::SetMouseCallBack(MouseButtonCallBack callback)
{
    mouseCallBack = callback;
}
//添加特征元素回调函数
void COccView::SetAddItemCallBack(AddItemCallBack callback)
{
    addCallBack = callback;
}
//标准软件和3d切换显示回调函数
void COccView::SetSwitchCallBack(SwitchCallBack callback)
{
    switchCallBack = callback;
}
//菜单按钮灰化及恢复的回调函数
void COccView::SetButtonCallBack(ButtonCallBack callback)
{
    btCallBack = callback;
}
//计时清除后按钮恢复回调函数
void COccView::SetClearCallBack(ClearCallBack callback)
{
    clrCallBack = callback;
}


// COccView 消息处理程序

gp_Ax2 COccView::GetCurrentCoordinate() const
{
    return m_CurrentCoordinate;
}

void COccView::SetFeatureColor(int index, Quantity_NameOfColor color)
{
    if (index < 0 || index >= m_Shape[activeindex].size())
        return;

    Handle(AIS_Shape) shape = m_Shape[activeindex][index];
    if (shape.IsNull())
        return;

    // 设置元素颜色
    m_AISContext->SetColor(shape, color, Standard_True);
    // 更新显示
    m_AISContext->Update(shape, Standard_True);
    myView->RedrawImmediate();
}

Handle(V3d_Viewer) COccView::GetViewer() const
{
    if (myView.IsNull())
    {
        // 如果视图未初始化，返回空
        return Handle(V3d_Viewer)();
    }
    // 通过V3d_View的Viewer()方法获取所属的V3d_Viewer
    return myView->Viewer();
}

void COccView::OnInitialUpdate()
{
    CView::OnInitialUpdate();
    if (myView.IsNull()) {
        return;
    }
    // TODO: 在此添加专用代码和/或调用基类
    //myView = GetDocument()->GetViewer()->CreateView();
    myView->SetShadingModel(V3d_GOURAUD);
    //Handle(Graphic3d_GraphicDriver) theGraphicDriver = ((COCCTestApp*)AfxGetApp())->GetGraphicDriver();
    //HWND hwnd = GetSafeHwnd();
    //Aspect_Handle aWindowHandle = (Aspect_Handle)GetSafeHwnd();
    Handle(WNT_Window) aWntWindow = new WNT_Window(GetSafeHwnd());
    myView->SetWindow(aWntWindow);
    if (!aWntWindow->IsMapped()) {
        aWntWindow->Map();
    }
    Standard_Integer w = 100;
    Standard_Integer h = 100;
    aWntWindow->Size(w, h);
    ::PostMessage(GetSafeHwnd(), WM_SIZE, SIZE_RESTORED, w + h * 65536);
    //g_pOccView = this;

    // 在类成员中添加
    bool m_cameraInitialized = false;
    // 默认显示1x
    myView->FitAll();

    myView->ZBufferTriedronSetup(Quantity_NOC_RED, Quantity_NOC_GREEN, Quantity_NOC_BLUE1, 0.8, 0.05, 12);
    myView->TriedronDisplay(Aspect_TOTP_LEFT_LOWER, Quantity_NOC_PURPLE, 0.1, V3d_ZBUFFER);
    ////myView->SetBgGradientColors(background_color, Quantity_NOC_WHITE, Aspect_GFM_VER);
    //m_AISContext->UpdateCurrentViewer();
}


struct TreeNode {
    int val;
    TreeNode* left;
    TreeNode* right;
    TreeNode() : val(0), left(nullptr), right(nullptr) {}
    TreeNode(int x) : val(x), left(nullptr), right(nullptr) {}
    TreeNode(int x, TreeNode* left, TreeNode* right) : val(x), left(left), right(right) {}
};
void COccView::OnMouseMove(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    //if (m_drawBias && !m_ifPause)
    if (!m_Shape.size() || (m_showBias && !m_ifPause))
        return;
    CRect rect;
    GetClientRect(&rect);
    switch (m_current_mode)
    {
    case COccView::CurAction3d_DynamicPanning:
        //执行平移
        myView->Pan(point.x - m_x_max, m_y_max - point.y);
        m_x_max = point.x;
        m_y_max = point.y;
        RedrawLableLine();
        break;

    case COccView::CurAction3d_DynamicRotation:
        //执行旋转
        myView->Rotation(point.x, point.y);
        RedrawLableLine();
        // 触发位置回调
        break;
    case COccView::CurAction3d_DynamicTranslation:
        m_listRgn[m_translationIndex].Remove(m_AISContext);
        //m_AISContext->RemoveAll(Standard_True);
        m_listRgn[m_translationIndex].SetPosition(point.x, rect.bottom - point.y, m_listRgn[m_translationIndex].Get2dLoc());
        points[m_translationIndex] = gp_Pnt2d(point.x, rect.bottom - point.y);
        m_listRgn[m_translationIndex].DisplayAll(m_AISContext);
        //m_AISContext->Display(m_Shape[activeindex][0], Standard_True);
        myView->Redraw();
        //m_listRgn[m_translationIndex].ReDisplayLine(m_AISContext);
        break;
    case COccView::CurAction3d_Nothing:
        //执行高亮显示
        if (!m_showBias)
            m_AISContext->MoveTo(point.x, point.y, myView, true);
        break;
    }
    CView::OnMouseMove(nFlags, point);

}

BOOL COccView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
    if (!m_Shape.size() || (m_showBias && !m_ifPause))
        return CView::OnMouseWheel(nFlags, zDelta, pt);

    if (myView.IsNull())
        return FALSE;

    ScreenToClient(&pt);

    int dy = (zDelta > 0) ? 10 : -10;

    myView->StartZoomAtPoint(pt.x, pt.y);
    myView->ZoomAtPoint(pt.x, pt.y, pt.x, pt.y + dy);

    if (m_listRgn.size())
        RedrawLableLine();

    myView->Redraw();
    return TRUE;
}



void COccView::OnLButtonDown(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    if (!m_Shape.size())
        return;

    // 如果鼠标移动点击到报表框上 → 仅平移报表框，不旋转（保留return）
    if (m_showBias)
    {
        CRect rc; GetClientRect(rc);
        for (int i = 0; i < m_listRgn.size(); i++)
        {
            RgnBox rgn = m_listRgn[i];
            CRect rect = CRect(rgn.GetScreenPos().X(), rc.bottom - rgn.GetScreenPos().Y(), rgn.GetScreenPos().X() + 80, rc.bottom - rgn.GetScreenPos().Y() + 40);
            if (point.x<rect.right && point.x>rect.left && point.y > rect.top && point.y < rect.bottom)
            {
                m_current_mode = CurAction3d_DynamicTranslation;
                m_translationIndex = i;
                return; // 报表框平移时，跳过旋转逻辑（保留）
            }
        }
    }

    // 2. VERTEX 模式下点击已建点 → 显示坐标（核心修改：移除return，不锁住旋转）
    bool isVertexClicked = false; // 标记是否点击了顶点
    if (ActiveSelect.GetActiveBaseSelect() == FeatureBaseType::VERTEX)
    {
        AIS_StatusOfPick state = m_AISContext->Select(true);
        if (state != AIS_SOP_NothingSelected)
        {
            /* ---------- 关键：先过滤标签 ---------- */
            Handle(AIS_InteractiveObject) pickObj;
            m_AISContext->InitSelected();
            if (m_AISContext->MoreSelected())
                pickObj = m_AISContext->SelectedInteractive();

            Handle(AIS_Trihedron) trihedronObj = Handle(AIS_Trihedron)::DownCast(pickObj);
            if (!trihedronObj.IsNull())
            {
                return;
            }

            Handle(AIS_TextLabel) textLbl = Handle(AIS_TextLabel)::DownCast(pickObj);
            if (textLbl.IsNull()) // 未选中标签时，处理顶点逻辑
            {
                TopoDS_Shape picked = m_AISContext->DetectedShape();
                if (!picked.IsNull() && picked.ShapeType() == TopAbs_VERTEX)
                {
                    gp_Pnt pt = BRep_Tool::Pnt(TopoDS::Vertex(picked));
                    gp_Pnt displayPt;
                    if (HasTrihedron())
                    {
                        gp_Trsf trf;
                        trf.SetTransformation(m_CurrentCoordinate);
                        displayPt = pt.Transformed(trf);
                    }
                    else
                    {
                        displayPt = pt;
                    }
                    std::lock_guard<std::mutex> lock(m_profilePointMutex);
                    m_hasValidClickedPoint = true;
                    m_clickedPointX = displayPt.X();
                    m_clickedPointY = displayPt.Y();
                    m_clickedPointZ = displayPt.Z();
                    temparr[0] = m_clickedPointX, temparr[1] = m_clickedPointY, temparr[2] = m_clickedPointZ;
                    PositionMotion(temparr);
                    // 显示坐标标签
                    CString txt;
                    txt.Format(_T("X:%.2f\nY:%.2f\nZ:%.2f"),
                        displayPt.X(), displayPt.Y(), displayPt.Z());
                    //AddLable(displayPt, txt, true);

                    //// 想永久显示标签则删除下面这行
                    //SetTimer(103, 3000, NULL);

                    isVertexClicked = true; // 标记：点击了顶点
                }
            }
        }
    }

    // ===== 核心修改：无论是否点击顶点，都执行旋转逻辑 =====
    m_current_mode = CurAction3d_DynamicRotation;
    mouseDownPT = point;
    myView->StartRotation(point.x, point.y);
    leftMouseBtn = true;

    // 调用基类函数（保留原有逻辑）
    CView::OnLButtonDown(nFlags, point);
}

//鼠标选中状态复原
void COccView::OnLButtonUp(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    if (!m_Shape.size())
        return;
    CView::OnLButtonUp(nFlags, point);

    m_current_mode = CurAction3d_Nothing;
    if (leftMouseBtn == true)
    {
        leftMouseBtn = false;
    }
}


void COccView::OnMButtonDown(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    if (!m_Shape.size())
        return;
    m_current_mode = CurAction3d_DynamicPanning;
    mouseDownPT = point;
    m_x_max = point.x; //记录平移时起始X位置
    m_y_max = point.y; //记录平移时起始Y位置
    midMouseBtn = true;

    CView::OnMButtonDown(nFlags, point);
}


void COccView::OnMButtonUp(UINT nFlags, CPoint point)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值
    if (!m_Shape.size())
        return;
    m_current_mode = CurAction3d_Nothing;
    if (midMouseBtn == true)
    {
        midMouseBtn = false;
    }
    CView::OnMButtonUp(nFlags, point);
}

//鼠标点选模型特征
void COccView::OnRButtonDown(UINT nFlags, CPoint point)
{
    if (!m_Shape.size())
        return;

    /* 0. 先拾取 */
    AIS_StatusOfPick pick = m_AISContext->Select(true);
    if (pick == AIS_SOP_NothingSelected)
        return;

    /* 1. 过滤掉坐标系（AIS_Trihedron） */
// 0. 过滤掉坐标系（AIS_Trihedron）
    Handle(AIS_InteractiveObject) detectedIO = m_AISContext->DetectedInteractive();
    if (!detectedIO.IsNull() &&
        detectedIO->IsKind(STANDARD_TYPE(AIS_Trihedron)))   // 老式 RTTI
    {
        return;   // 坐标系不参与任何右键逻辑
    }

    /* 2. 后面就是原来的 FACE/EDGE/VERTEX 逻辑 */
    TopoDS_Shape shape = m_AISContext->DetectedShape();
    if (shape.IsNull())
        return;

    Handle(AIS_Shape) aisShape = new AIS_Shape(shape);
    TopAbs_ShapeEnum st = shape.ShapeType();

    switch (st)
    {
    case TopAbs_FACE:
        if (ActiveSelect.GetActiveBaseSelect() == FeatureBaseType::FACE)
            Add(aisShape, GetFaceType(shape));
        break;
    case TopAbs_EDGE:
        if (ActiveSelect.GetActiveBaseSelect() == FeatureBaseType::EDGE)
            Add(aisShape, GetEdgeType(shape));
        break;
    case TopAbs_VERTEX:
        if (ActiveSelect.GetActiveBaseSelect() == FeatureBaseType::VERTEX)
            Add(aisShape, FeatureType::VERTEX);
        break;
    default:
        break;
    }

    CView::OnRButtonDown(nFlags, point);
}


void COccView::ShowPointCoordinate(const gp_Pnt& pt)
{
    CString txt;
    txt.Format(_T("X:%.2f\nY:%.2f\nZ:%.2f"), pt.X(), pt.Y(), pt.Z());
    AddLable(pt, txt, true);   // true → 小黑球 + 细字
}

int COccView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if (CView::OnCreate(lpCreateStruct) == -1)
        return -1;

    // TODO:  在此添加您专用的创建代码
    //m_staticBias.Create("bias", WS_CHILD | WS_VISIBLE, CRect(20, 30, 60, 50), this);
    return 0;
}

Quantity_Color COccView::RGBToQuantityClr(int r, int g, int b)
{
    Quantity_Color color;
    Standard_Real red = static_cast<float>(r) / 255.0f;
    Standard_Real green = static_cast<float>(g) / 255.0f;
    Standard_Real blue = static_cast<float>(b) / 255.0f;

    // 创建 Quantity_Color 对象
    color = Quantity_Color(red, green, blue, Quantity_TOC_RGB);
    return color;
}

//搜索模型上距离该点最近的特征点
gp_Pnt COccView::SearchNearestPoint(gp_Pnt pt)
{
    TopoDS_Vertex shapePoint = BRepBuilderAPI_MakeVertex(pt);
    TopExp_Explorer explore(m_Shape[activeindex][0]->Shape(), TopAbs_FACE);
    gp_Pnt point; Standard_Real min_dis = 100;
    for (; explore.More(); explore.Next())
    {
        TopoDS_Face face = TopoDS::Face(explore.Current());
        BRepExtrema_DistShapeShape extrema(shapePoint, face);
        if (extrema.IsDone() && min_dis > extrema.Value())
        {
            min_dis = extrema.Value();
            point = extrema.PointOnShape2(1);
        }
    }
    return point;
}
//根据点在点集排序后的行列位置来排版报表框位置
void COccView::CreateLable(RgnBox& rgn, int row, int col, int rowCount)
{
    CRect rect;
    GetClientRect(rect);
    // 核心1：右侧预留空间从120→50（多释放70像素给报表框）
    rect.right -= 50;

    // 核心2：适当增大报表框尺寸（从70→90，保证可见性）
    int width = 90;
    // 垂直预留空间从100→150，避免高度过小导致框体挤压
    int height = (rect.bottom - rect.top - 150) / rowCount;

    int length = m_position.size() / rowCount;
    Standard_Integer screen_x, screen_y;
    gp_Pnt realLoc = rgn.Get3dLoc();
    myView->Convert(realLoc.X(), realLoc.Y(), realLoc.Z(), screen_x, screen_y);
    gp_Pnt2d screenpos;
    screenpos.SetX(screen_x);
    screenpos.SetY(rect.bottom - screen_y);

    int size = m_listRgn.size();
    if (points.size() > size)
    {
        rgn.SetPosition(points[size].X(), points[size].Y(), screenpos);
        return;
    }

    // 水平间距适当增加（从7→10，避免框体过挤但保证多放几个）
    const int hGap = 10;
    // 垂直间距保持紧凑（2→3，轻微增大避免文字重叠）
    const int vGap = 3;

    Standard_Real startX, startY;

    // 首行（底部）：右移更多，利用右侧释放的空间
    if (row == 0)
    {
        // 右侧偏移从300→150（右移150像素，靠近右侧边缘）
        startX = rect.right - col * (width + hGap) - 200;
        startY = rect.bottom - 25;  // 底部偏移略增，避免贴边
    }
    // 末行（顶部）：同理右移，增加右侧显示数量
    else if (row == rowCount - 1)
    {
        startX = rect.right - col * (width + hGap) - 200;  // 右移150像素
        startY = rect.top + 25;  // 顶部偏移略增
    }
    // 中间行（右半部分）：左移更少，让更多框体靠右
    else if (col > length / 2 - 1)
    {
        col = length - col;
        // 左侧偏移从40→20（左移减少，释放右侧空间）
        startX = rect.left + (width + hGap) * col + 70;
        // 垂直方向减少列偏移影响，避免下移过多
        startY = rect.bottom - (height + vGap) * row - col * 5 + 10;
    }
    // 中间行（左半部分）：右移，减少右侧浪费空间
    else
    {
        // 右侧偏移从150→100（再右移50像素）
        startX = rect.right - (width + hGap) * col - 200;
        // 垂直方向减少列偏移，避免上移过多
        startY = rect.bottom - (height + vGap) * row - col * 5 - 80;
    }

    // 最终位置设置
    rgn.SetPosition(startX, startY, screenpos);
    points.push_back(gp_Pnt2d(startX, startY));
}

//void COccView::DrawLableList()
//{	
//	if (m_listRgn.size())
//	{
//		for (auto& rgn : m_listRgn)
//		{
//			rgn.DisplayAll(m_AISContext);
//		}
//		if (!m_ColorScale.IsNull())
//			m_AISContext->Display(m_ColorScale, Standard_True);
//		m_showBias = TRUE;
//		myView->Redraw();
//	}
//}
//清除报表
void COccView::RemoveLableList()
{
    for (auto& rgn : m_listRgn)
    {
        rgn.Remove(m_AISContext);
    }
    if (m_AISContext->IsDisplayed(m_ColorScale))
    {
        m_AISContext->Remove(m_ColorScale, Standard_True);
    }
    Standard::Purge();
    //m_listRgn.clear();
    m_showBias = FALSE;
    myView->Redraw();
}
//重绘连接报表框和模型上点的连线
void COccView::RedrawLableLine()
{
    Standard_Integer screen_x, screen_y;
    CRect rect; GetClientRect(rect);
    for (auto& rgn : m_listRgn)
    {
        gp_Pnt point = rgn.Get3dLoc();
        myView->Convert(point.X(), point.Y(), point.Z(), screen_x, screen_y);
        gp_Pnt2d screenpos; screenpos.SetX(screen_x); screenpos.SetY(rect.bottom - screen_y);
        rgn.Set2dLoc(screenpos);
        rgn.ReDisplayLine(m_AISContext);
    }
    myView->Redraw();
}
//重绘单个索引指定的连接报表框和模型上点的连线
void COccView::RedrawSingleLine(int index)
{
    Standard_Integer screen_x, screen_y;
    CRect rect; GetClientRect(rect);
    gp_Pnt point = m_listRgn[index].Get3dLoc();
    myView->Convert(point.X(), point.Y(), point.Z(), screen_x, screen_y);
    gp_Pnt2d screenpos; screenpos.SetX(screen_x); screenpos.SetY(rect.bottom - screen_y);
    m_listRgn[index].Set2dLoc(screenpos);
    m_listRgn[index].DrawLableLine();

    myView->Redraw();
}
//绘制颜色条
Handle(AIS_ColorScale) COccView::ShowColorScale(Standard_CString str, double min, double max, bool isBlueToRed)
{
    TCollection_ExtendedString tostr(str, true);
    /*if (!m_ColorScale.IsNull())
    {
        m_AISContext->Remove(m_ColorScale, Standard_False);
        m_ColorScale.Nullify();
    }*/
    //m_ColorScale->SetFormat(TCollection_AsciiString("%E"));
    m_ColorScale->SetSize(200, 500);
    m_ColorScale->SetRange(min, max);//设置颜色条区间
    //    aColorScale->SetColorType(Aspect_TOCSD_AUTO);
    m_ColorScale->SetNumberOfIntervals(9);
    m_ColorScale->SetSmoothTransition(Standard_True);//颜色平衡过度
    m_ColorScale->SetLabelPosition(Aspect_TOCSP_RIGHT);
    m_ColorScale->SetTextHeight(14);
    m_ColorScale->SetColor(Quantity_Color(Quantity_NOC_BLACK));
    m_ColorScale->SetTitle(tostr);
    if (isBlueToRed) {
        m_ColorScale->SetColorRange(Quantity_Color(Quantity_NOC_BLUE1), Quantity_Color(Quantity_NOC_RED));
    }
    else
    {
        m_ColorScale->SetColorRange(Quantity_Color(Quantity_NOC_RED), Quantity_Color(Quantity_NOC_BLUE1));
    }

    m_ColorScale->SetLabelType(Aspect_TOCSD_AUTO);
    m_ColorScale->SetZLayer(Graphic3d_ZLayerId_TopOSD);
    Graphic3d_Vec2i anoffset(120, Standard_Integer(500));
    //aColorScale->SetTransformPersistence(Graphic3d_TMF_2d,gp_Pnt(-1,0,0));
    m_AISContext->SetTransformPersistence(m_ColorScale, new Graphic3d_TransformPers(Graphic3d_TMF_2d, Aspect_TOTP_RIGHT_UPPER, anoffset));
    m_AISContext->SetDisplayMode(m_ColorScale, 1, Standard_False);
    m_AISContext->Display(m_ColorScale, Standard_True);
    //m_AISContext->Remove(m_ColorScale, Standard_False);
    return m_ColorScale;
}

void COccView::OnTimer(UINT_PTR nIDEvent)
{
    // TODO: 在此添加消息处理程序代码和/或调用默认值

    if (nIDEvent == 1)
    {
        static int count = 0;
        if (count == 0)
            switchCallBack(true);
        count++;
        //3d结束收点后，按钮要及时复原
        if (!m_drawBias)
        {
            KillTimer(1);
            //SetTimer(3, 1, NULL);
            count = 0;
            switchCallBack(true);
            Invalidate(FALSE);
        }
        //停留10s或取消暂停恢复常态后要清除报表绘制
        if (count >= 10 && !m_ifPause)
        {
            KillTimer(1);
            //SetTimer(3, 1, NULL);
            RemoveLableList();
            /*for (auto shape : m_Sphere) {
                m_AISContext->SetColor(shape, Quantity_NameOfColor::Quantity_NOC_RED, Standard_False);
            }*/
            for (auto shape : m_Sphere)
            {
                m_AISContext->Remove(shape, Standard_True);
            }
            m_Sphere.clear();
            count = 0;// m_collectedCount = 0;
            table.clear(); ptTable.clear(); m_position.clear();
            m_showBias = false;
            //数据处理后
            tmpSignal.state = 2;
            memcpy_s(m_mySignal.get(), m_mySignalLen, &tmpSignal, m_mySignalLen);
            m_ipc->writeToMySignalSharedMemory(m_mySignal.get(), m_mySignalLen);
            //m_dataMul.points.clear();
            switchCallBack(true);
            Invalidate(FALSE);
        }
    }
    else if (nIDEvent == 2)
    {
        KillTimer(2);
        btCallBack();
    }
    else if (nIDEvent == 3)
    {
        //m_ipc->releaseMyThread();
        SafeReleaseIpc();
        KillTimer(3);
        btCallBack();
        //SetTimer(2, 1, NULL);
    }
    else if (nIDEvent == 4)
    {
        KillTimer(4); m_ifPause.store(true);
        //m_ipc->releaseSDThread();
        btCallBack();
        clrCallBack();
    }
    else if (nIDEvent == 5)
    {
        KillTimer(5);
        switchCallBack(false);
    }
    else if (nIDEvent == 103)
    {
        KillTimer(103);
        for (auto& shape : m_temporaryShapes)
        {
            if (!shape.IsNull())
            {
                m_AISContext->Erase(shape, Standard_False); // 隐藏但不删除
            }
        }
        myView->Redraw();
    }
    CView::OnTimer(nIDEvent);
}
// COccView.h 中添加声明

// COccView.cpp 中实现
gp_Pnt COccView::GetSelectedElementPosition(int index)
{
    if (activeindex < 0 || activeindex >= m_Shape.size() || index < 0 || index >= m_Shape[activeindex].size())
        return gp_Pnt(0, 0, 0);

    TopoDS_Shape shape = m_Shape[activeindex][index]->Shape();
    TopAbs_ShapeEnum shapeType = shape.ShapeType();

    if (shapeType == TopAbs_VERTEX) {
        TopoDS_Vertex vertex = TopoDS::Vertex(shape);
        return BRep_Tool::Pnt(vertex);
    }
    else if (shapeType == TopAbs_EDGE) {
        TopoDS_Edge edge = TopoDS::Edge(shape);
        Standard_Real first, last;
        Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
        gp_Pnt midPoint;
        curve->D0((first + last) / 2, midPoint);
        return midPoint;
    }
    else if (shapeType == TopAbs_FACE) {
        TopoDS_Face face = TopoDS::Face(shape);
        Handle(Geom_Surface) surface = BRep_Tool::Surface(face);
        Standard_Real u, v;
        BRepTools::UVBounds(face, u, v, u, v); // 获取UV范围的中点
        gp_Pnt centerPoint;
        surface->D0(u, v, centerPoint);
        return centerPoint;
    }

    return gp_Pnt(0, 0, 0);
}
// 计算选中元素的面积
// 
//
// 计算选中元素的面积
#include <BRepGProp.hxx> // Ensure the correct header is included for BRepGProp

double COccView::GetSelectedElementArea(int index)
{
    if (activeindex < 0 || activeindex >= m_Shape.size() || index < 0 || index >= m_Shape[activeindex].size())
        return 0.0;

    TopoDS_Shape shape = m_Shape[activeindex][index]->Shape();
    TopAbs_ShapeEnum shapeType = shape.ShapeType();

    // 仅对面进行面积计算
    if (shapeType == TopAbs_FACE) {
        TopoDS_Face face = TopoDS::Face(shape);

        // 使用BRepGProp计算面积
        GProp_GProps props;
        BRepGProp::SurfaceProperties(face, props);

        // 返回计算得到的面积
        return props.Mass();
    }
    // 对于边，返回长度
    else if (shapeType == TopAbs_EDGE) {
        TopoDS_Edge edge = TopoDS::Edge(shape);

        // 检查边是否有效
        if (edge.IsNull()) {
            return 0.0;
        }
        // 预处理边（构建3D曲线）
        BRepLib::BuildCurves3d(edge);

        // 使用BRepGProp计算长度
        GProp_GProps props;
        try {
            BRepGProp::LinearProperties(edge, props);

            // 检查计算结果是否有效
            if (props.Mass() <= 0) {
                // 尝试其他方法或返回默认值
                return 0.0;
            }

            // 返回计算得到的长度
            return props.Mass();
        }
        catch (...) {
            // 捕获所有异常并返回0
            return 0.0;
        }
    }
    // 对于顶点，返回0
    else if (shapeType == TopAbs_VERTEX) {
        return 0.0;
    }

    return 0.0;
}
#include <Quantity_Color.hxx>
#include <Prs3d_DatumParts.hxx>
// 设置坐标系三个轴的颜色
void COccView::SetTrihedronColors(Quantity_Color xColor, Quantity_Color yColor, Quantity_Color zColor)
{
    // 检查坐标系对象是否存在（关键！）
    if (m_Trihedron.IsNull())
    {
        AfxMessageBox(_T("坐标系未创建，无法设置颜色")); // 调试用，可删除
        return;
    }

    // 设置轴线颜色
    m_Trihedron->SetXAxisColor(xColor);
    m_Trihedron->SetYAxisColor(yColor);
    m_Trihedron->SetAxisColor(zColor); // Z轴颜色

    // 设置箭头颜色
    m_Trihedron->SetArrowColor(Prs3d_DP_XArrow, xColor);
    m_Trihedron->SetArrowColor(Prs3d_DP_YArrow, yColor);
    m_Trihedron->SetArrowColor(Prs3d_DP_ZArrow, zColor);

    // 强制刷新视图（修正判断条件）
    if (!m_AISContext.IsNull())  // 替换 m_AISContext.IsNotNull()
    {
        m_AISContext->Redisplay(m_Trihedron, Standard_True);
        m_AISContext->UpdateCurrentViewer();
    }
    if (!myView.IsNull())  // 替换 myView.IsNotNull()
    {
        myView->Redraw();
    }
}void COccView::GetTrihedronColors(Quantity_Color& xCol,
    Quantity_Color& yCol,
    Quantity_Color& zCol) const
{
    xCol = m_triColorX;
    yCol = m_triColorY;
    zCol = m_triColorZ;
}
// 在 COccView.cpp 中实现
void COccView::SetTransparency(int shapeIndex, double transparency)
{
    if (shapeIndex < 0 || shapeIndex >= m_Shape[activeindex].size())
        return;

    // 获取形状的AIS_InteractiveObject
    Handle(AIS_Shape) obj = m_Shape[activeindex][shapeIndex];

    // 设置透明度 (0.0-1.0, 0=完全透明, 1=完全不透明)
    obj
        ->SetTransparency(transparency);

    // 更新显示
    m_AISContext
        ->SetTransparency(obj, transparency, Standard_True);
    m_AISContext
        ->UpdateCurrentViewer();
}
// 获取AIS形状指针
Handle(AIS_Shape) COccView::GetAISShape(int index)
{
    if (activeindex >= 0 && activeindex < m_Shape.size() &&
        index >= 0 && index < m_Shape[activeindex].size()) {
        return m_Shape[activeindex][index];
    }
    return NULL;
}
void COccView::ToggleTransparency(int shapeIndex)
{
    if (shapeIndex < 0 || shapeIndex >= m_Shape[activeindex].size())
        return;

    Handle(AIS_Shape) obj = m_Shape[activeindex][shapeIndex];
    double currentTransparency = obj->Transparency();

    // 切换透明度状态
    if (currentTransparency > 0.5) {
        SetTransparency(shapeIndex, 0.0); // 设置为不透明//
    }
    else {
        SetTransparency(shapeIndex, 0.1); // 设置为半透明
    }
}int COccView::GetShapeCount()
{
    if (activeindex < 0 || activeindex >= m_Shape.size())
        return 0;
    return m_Shape[activeindex].size();
}
bool COccView::HasTrihedron()
{
    return !m_Trihedron.IsNull() && m_AISContext->IsDisplayed(m_Trihedron);
}
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <TDocStd_Document.hxx>
#include <TDataStd_Name.hxx>
#include <set>
// XCAF文档相关核心头文件
#include <TDocStd_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <TDocStd_Document.hxx>
#include <TCollection_ExtendedString.hxx>

TopoDS_Shape COccView::GetTrihedronAsShape()
{
    // 替换m_trihedronAxis为之前定义的m_CurrentCoordinate（确保坐标系有效）
    gp_Ax2 currentAx = m_CurrentCoordinate;

    // 创建复合形状存储三轴
    TopoDS_Compound triCompound;
    BRep_Builder builder;
    builder.MakeCompound(triCompound);

    // 轴长度（可根据模型大小调整）
    Standard_Real axisLength = 10.0;
    gp_Pnt origin = currentAx.Location();

    // 1. 创建X轴（红色）
    gp_Dir xDir = currentAx.XDirection();
    gp_Pnt xEnd = origin.Translated(gp_Vec(xDir) * axisLength);
    TopoDS_Edge xAxis = BRepBuilderAPI_MakeEdge(origin, xEnd);
    builder.Add(triCompound, xAxis);

    // 2. 创建Y轴（绿色）
    gp_Dir yDir = currentAx.YDirection();
    gp_Pnt yEnd = origin.Translated(gp_Vec(yDir) * axisLength);
    TopoDS_Edge yAxis = BRepBuilderAPI_MakeEdge(origin, yEnd);
    builder.Add(triCompound, yAxis);

    // 3. 创建Z轴（蓝色）
    gp_Dir zDir = currentAx.Direction();//
    gp_Pnt zEnd = origin.Translated(gp_Vec(zDir) * axisLength);//这里使用derection获取z轴方向
    TopoDS_Edge zAxis = BRepBuilderAPI_MakeEdge(origin, zEnd);//
    builder.Add(triCompound, zAxis);

    if (m_XCAFDoc.IsNull())
    {
        // 初始化XCAF文档（用于管理形状与颜色的关联）
        Handle(TDocStd_Application) app = new TDocStd_Application;
        app->NewDocument("MDTV-XCAF", m_XCAFDoc);
    }

    Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_ShapeTool::Set(m_XCAFDoc->Main());
    Handle(XCAFDoc_ColorTool) colorTool = XCAFDoc_ColorTool::Set(m_XCAFDoc->Main());

    TDF_Label triLabel = shapeTool->AddShape(triCompound);
    TDataStd_Name::Set(triLabel, TCollection_ExtendedString("Trihedron"));

    TDF_Label xLabel = shapeTool->AddSubShape(triLabel, xAxis);
    if (!xLabel.IsNull()) {
        TDataStd_Name::Set(xLabel, TCollection_ExtendedString("XAxis"));
        colorTool->SetColor(xLabel, Quantity_NOC_RED, XCAFDoc_ColorGen);
    }

    TDF_Label yLabel = shapeTool->AddSubShape(triLabel, yAxis);
    if (!yLabel.IsNull()) {
        TDataStd_Name::Set(yLabel, TCollection_ExtendedString("YAxis"));
        colorTool->SetColor(yLabel, Quantity_NOC_GREEN, XCAFDoc_ColorGen);
    }
    TDF_Label zLabel = shapeTool->AddSubShape(triLabel, zAxis);
    if (!zLabel.IsNull()) {
        TDataStd_Name::Set(zLabel, TCollection_ExtendedString("ZAxis"));
        colorTool->SetColor(zLabel, Quantity_NOC_BLUE1, XCAFDoc_ColorGen);
    }
    return triCompound;

}// 计算XY点在激活曲面上的Z坐标
//gp_Pnt COccView::CalculateZFromXY(const gp_Pnt2d& xyPoint)
//{
//    // 确保激活曲面有效
//    if (m_ActiveSurface.IsNull()) {
//        //UpdateActiveFace();  // 尝试更新曲面
//        if (m_ActiveSurface.IsNull()) {
//            TRACE("无有效曲面，无法计算Z坐标！");
//            return gp_Pnt(xyPoint.X(), xyPoint.Y(), 0.0);  // 默认Z=0
//        }
//    }
//
//    gp_Pnt resultPoint;
//    Standard_Real u, v;
//    Standard_Real maxDist = Precision::Confusion();  // 精度阈值
//
//    // 1. 尝试用GeomLib_Tool计算UV参数
//    gp_Pnt temp3dPoint(xyPoint.X(), xyPoint.Y(), 0.0);  // 临时3D点（Z=0）
//    Standard_Boolean isFound = GeomLib_Tool::Parameters(m_ActiveSurface, temp3dPoint, maxDist, u, v);
//
//    if (isFound) {
//        m_ActiveSurface->D0(u, v, resultPoint);  // 通过UV获取Z坐标
//        return resultPoint;
//    }
//
//    // 2. 备选方案：沿Z轴投影求交
//    gp_Lin zLine(temp3dPoint, gp_Dir(0, 0, 1));  // 沿Z轴方向的直线
//    IntCurvesFace_ShapeIntersector intersector;
//    intersector.Load(m_ActiveFace, maxDist);  // 加载激活的面
//    intersector.Perform(zLine, -10000, 10000);  // 搜索交点
//
//    if (intersector.IsDone() && intersector.NbPnt() > 0) {
//        return intersector.Pnt(1);  // 返回第一个交点
//    }
//    else {
//        TRACE("投影失败，使用默认Z=0！");
//        return gp_Pnt(xyPoint.X(), xyPoint.Y(), 0.0);
//    }
//}void COccView::DisplayProjectionPoint(const gp_Pnt2d& measuredXY)
//{
//    // 1. 确保激活曲面有效
//    if (m_ActiveSurface.IsNull()) {
//        /*   UpdateActiveFace();*/
//        if (m_ActiveSurface.IsNull()) {
//            TRACE("无有效曲面，无法显示投影点！");
//            return;
//        }
//    }
//
//    // 2. 计算Z坐标，得到3D点
//    gp_Pnt measuredPoint = CalculateZFromXY(measuredXY);
//
//    // 3. 查找投影点（使用现有逻辑）
//    gp_Pnt projectedPoint = SearchNearestPoint(measuredPoint);
//    double distance = measuredPoint.Distance(projectedPoint);
//
//    // 4. 显示测量点和投影点
//    TopoDS_Shape measSphere = BRepPrimAPI_MakeSphere(measuredPoint, 0.6);
//    Handle(AIS_Shape) aisMeasSphere = new AIS_Shape(measSphere);
//    aisMeasSphere->SetColor(Quantity_NOC_RED);
//
//    TopoDS_Shape projSphere = BRepPrimAPI_MakeSphere(projectedPoint, 0.50);
//    Handle(AIS_Shape) aisProjSphere = new AIS_Shape(projSphere);
//    aisProjSphere->SetColor(Quantity_NOC_BLUE);
//
//    // 存储并显示
//    m_projectionSpheres.push_back(aisProjSphere);
//    m_measurementSpheres.push_back(aisMeasSphere);
//    m_AISContext->Display(aisProjSphere, Standard_True);
//    m_AISContext->Display(aisMeasSphere, Standard_True);
//
//    // 输出调试信息
//    TRACE("投影显示: 测量点(%.4f,%.4f,%.4f) -> 投影点(%.4f,%.4f,%.4f), 距离=%.6f\n",
//        measuredPoint.X(), measuredPoint.Y(), measuredPoint.Z(),
//        projectedPoint.X(), projectedPoint.Y(), projectedPoint.Z(),
//        distance);
//
//    // 刷新视图
//    m_AISContext->UpdateCurrentViewer();
//    myView->Redraw();
//}
void COccView::ClearRealTimeDisplay()
{
    // 清理实时显示的小球
    for (auto& sphere : m_projectionSpheres) {
        m_AISContext->Remove(sphere, Standard_True);
    }
    for (auto& sphere : m_measurementSpheres) {
        m_AISContext->Remove(sphere, Standard_True);
    }

    m_projectionSpheres.clear();
    m_measurementSpheres.clear();

    m_AISContext->UpdateCurrentViewer();
    myView->Redraw();
}
// 导出函数：传入轮廓点，flagBit=0表示普通点，=1表示最后一个点
