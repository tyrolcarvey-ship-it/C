#include "ModelImport.h"
#include <algorithm>
#include <limits>
#include <BRep_Builder.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>
#include <Precision.hxx>
#include <mutex>
ModelImport::ModelImport()
    : m_maxCache(10)                                    // 初始化最大缓存数量
    , m_generateImages(true)                            // 初始化图像生成标志
    , m_usePerformanceMode(false)                       // 初始化性能模式标志
    , m_lastImageWidth(0)                               // 初始化最后图像宽度
    , m_lastImageHeight(0)                              // 初始化最后图像高度
    , m_boundsCalculated(false)                         // 初始化边界框计算标志
    , m_modelMin(0.0, 0.0, 0.0)                         // 初始化模型最小点
    , m_modelMax(0.0, 0.0, 0.0)                         // 初始化模型最大点
    , m_modelCenter(0.0, 0.0, 0.0)                      // 初始化模型中心点
    , m_modelDiameter(0.0)                              // 初始化模型直径
    , m_imageWidth(640)                                 // 宽
    , m_imageHeight(480)                                // 高
    , m_focalLength(50.0)                               // 焦距
    , m_viewConfig()                                    // 初始化视图配置（使用默认构造函数）
    , m_camHeightFactor(1.0)

{
    // 初始化Handle对象为空状态（它们默认为空，但显式初始化更安全）
    m_displayConnection.Nullify();
    m_viewer.Nullify();
    m_context.Nullify();
    m_currentDoc.Nullify();

    // 清空容器（虽然默认构造时已为空，但显式清空更安全）
    m_cacheOrder.clear();
    m_loadedModelsCache.clear();
    m_imageDataBuffers.clear();
    m_filePath.clear();

}


ModelImport::~ModelImport()
{
    ClearModelCache();

    if (m_ownsViewer)
    {
        if (!m_context.IsNull())
        {
            m_context->RemoveAll(Standard_False);
            m_context.Nullify();
        }

        if (!m_viewer.IsNull())
        {
            m_viewer->InitDefinedLights();
            while (m_viewer->MoreDefinedLights())
            {
                m_viewer->SetLightOff(m_viewer->DefinedLight());
                m_viewer->NextDefinedLights();
            }
            m_viewer.Nullify();
        }

        if (!m_displayConnection.IsNull())
        {
            m_displayConnection.Nullify();
        }
    }

    m_imageDataBuffers.clear();
    m_imageDataBuffers.shrink_to_fit();
}

void ModelImport::ClearOffscreenExcludes()
{
    m_offscreenExcludes.clear();
}

void ModelImport::AddOffscreenExclude(const Handle(AIS_InteractiveObject)& obj)
{
    if (obj.IsNull())
        return;
    auto it = std::find_if(m_offscreenExcludes.begin(), m_offscreenExcludes.end(),
        [&obj](const Handle(AIS_InteractiveObject)& existing) { return existing == obj; });
    if (it == m_offscreenExcludes.end())
        m_offscreenExcludes.push_back(obj);
}

void ModelImport::SetOffscreenExcludes(const std::vector<Handle(AIS_InteractiveObject)>& objs)
{
    m_offscreenExcludes = objs;
}


// 主函数，支持焦距控制
bool ModelImport::ImportModelAndExportViews(const std::wstring& filePath,
    const int w, const int h,
    bool generateImages,
    double focalLength)
{
    // 首先检查文件是否存在
    if (!FileExists(filePath))
    {
        std::wcerr << L"Error: File does not exist: " << filePath << std::endl;
        return false;
    }

    // 验证分辨率参数
    if (!IsValidResolution(w, h))
    {
        std::wcerr << L"Error: Invalid resolution " << w << L"x" << h << std::endl;
        return false;
    }

    // 验证焦距参数
    if (focalLength <= 0.0 || focalLength > 500.0)
    {
        std::wcerr << L"Error: Invalid focal length " << focalLength
            << L"mm (valid range: 0.1-500)" << std::endl;
        return false;
    }

    // 同步更新成员变量
    m_imageWidth = w;
    m_imageHeight = h;
    m_focalLength = focalLength;

    // 设置标志位和配置
    m_generateImages = generateImages;
    m_viewConfig.focalLength = focalLength;
    m_viewConfig.aspectRatio = static_cast<double>(w) / h;

    // 性能模式判断
    m_usePerformanceMode = !ShouldUseHighQuality(w, h);
    m_lastImageWidth = w;
    m_lastImageHeight = h;

    // 清空之前的图像数据
    m_imageDataBuffers.clear();

    if (!InitializeDisplay() || !InitializeViewer() || !InitializeContext())
        return false;

    m_filePath = filePath;

    std::vector<Handle(AIS_InteractiveObject)> loadedObjects = LoadModelWithCache(filePath);

    if (loadedObjects.empty())
    {
        std::wcerr << L"Failed to load shapes from file: " << filePath << std::endl;
        return false;
    }
    std::wcout << L"Model loaded successfully." << std::endl;

    // 显示对象
    DisplayShapesWithShadedMode(loadedObjects);

    // 计算模型包围盒
    CalculateModelBounds();

    // 直接进入try块
    try
    {
        if (m_preserveExternalViewState) {
            ExportCurrentView(m_viewer, m_context, filePath, m_imageWidth, m_imageHeight, m_viewConfig);
        }
        else {
            ExportViewsToMemoryAndFile(m_viewer, m_context, filePath, m_imageWidth, m_imageHeight, m_viewConfig);
        }
        std::wcout << L"Views generated and saved to memory successfully (focal length: "
            << m_focalLength << L"mm)." << std::endl;
    }
    catch (...)
    {
        std::wcout << L"Model loaded without generating views (generateImages = false)." << std::endl;
    }

    return true;
}

//bool ModelImport::ImportExportViews(Handle(V3d_Viewer)& viewer, Handle(AIS_InteractiveContext)& context,
//    const int w, const int h, bool generateImages, double focalLength)
//{
//    // 验证传入的对象
//    if (viewer.IsNull())
//    {
//        std::wcerr << L"Error: Invalid viewer object" << std::endl;
//        return false;
//    }
//
//    if (context.IsNull())
//    {
//        std::wcerr << L"Error: Invalid context object" << std::endl;
//        return false;
//    }
//
//    // 验证分辨率参数
//    if (!IsValidResolution(w, h))
//    {
//        std::wcerr << L"Error: Invalid resolution " << w << L"x" << h << std::endl;
//        return false;
//    }
//
//    // 验证焦距参数
//    if (focalLength <= 0.0 || focalLength > 500.0)
//    {
//        std::wcerr << L"Error: Invalid focal length " << focalLength
//            << L"mm (valid range: 0.1-500)" << std::endl;
//        return false;
//    }
//
//    // 同步更新成员变量
//    m_imageWidth = w;
//    m_imageHeight = h;
//    m_focalLength = focalLength;
//
//    // 设置标志位和配置
//    m_generateImages = generateImages;
//    m_viewConfig.focalLength = focalLength;
//    m_viewConfig.aspectRatio = static_cast<double>(w) / h;
//
//    // 性能模式判断
//    m_usePerformanceMode = !ShouldUseHighQuality(w, h);
//    m_lastImageWidth = w;
//    m_lastImageHeight = h;
//
//    // 清空之前的图像数据
//    m_imageDataBuffers.clear();
//
//    // 直接使用传入的 viewer 和 context
//    m_viewer = viewer;
//    m_context = context;
//
//    // 初始化显示连接（如果需要）
//    if (!InitializeDisplay())
//    {
//        std::wcerr << L"Failed to initialize display" << std::endl;
//        return false;
//    }
//
//    // 从传入的 context 中解析几何模型
//    std::vector<Handle(AIS_InteractiveObject)> loadedObjects = ParseObjectsFromContext(context);
//
//    if (loadedObjects.empty())
//    {
//        std::wcerr << L"No objects found in the provided context" << std::endl;
//        return false;
//    }
//
//    std::wcout << L"Successfully parsed " << loadedObjects.size()
//        << L" objects from context" << std::endl;
//
//    ///////////////////////////////////////////
//    std::vector<Handle(AIS_InteractiveObject)> internalObjects;
//    internalObjects.reserve(loadedObjects.size());
//
//    for (auto& obj : loadedObjects)
//    {
//        if (obj.IsNull()) continue;
//
//        // 只针对几何对象复制
//        Handle(AIS_Shape) shapeObj = Handle(AIS_Shape)::DownCast(obj);
//        if (!shapeObj.IsNull())
//        {
//            Handle(AIS_Shape) newShape = new AIS_Shape(shapeObj->Shape());
//
//            CopyBasicAttributes(shapeObj, newShape);
//            SetGeometrySpecificAttributes(newShape, shapeObj->Shape().ShapeType());
//
//            internalObjects.push_back(newShape);
//        }
//        else
//        {
//            internalObjects.push_back(obj);
//        }
//    }
//
//    DisplayShapesWithShadedMode(internalObjects);
//
//    CalculateModelBounds();
//
//    //// 确保所有对象都在 context 中显示
//    //EnsureObjectsDisplayed(context, loadedObjects);
//
//    //// 计算模型包围盒
//    //CalculateModelBoundsFromContext(context);
//
//    // 根据标志位决定是否生成图片
//    try
//    {
//        if (generateImages)
//        {
//            //ExportViewsToMemoryAndFile(m_viewer, m_context, L"", m_imageWidth, m_imageHeight, m_viewConfig);
//            ExportCurrentView(m_viewer, m_context, L"", m_imageWidth, m_imageHeight, m_viewConfig);
//            std::wcout << L"Views generated and saved to memory successfully (focal length: "
//                << m_focalLength << L"mm)." << std::endl;
//        }
//        else
//        {
//            std::wcout << L"Model parsed without generating views (generateImages = false)." << std::endl;
//        }
//    }
//    catch (const std::exception& e)
//    {
//        std::wcerr << L"Error generating views: " << e.what() << std::endl;
//        return false;
//    }
//    catch (...)
//    {
//        std::wcerr << L"Unknown error occurred while generating views" << std::endl;
//        return false;
//    }
//
//    return true;
//}

bool ModelImport::ImportExportViews(Handle(V3d_Viewer)& viewer,
    Handle(AIS_InteractiveContext)& context,
    const int w, const int h,
    bool generateImages,
    double focalLength)
{
    if (viewer.IsNull() || context.IsNull())
        return false;

    // 保存外部 viewer/context 供后续用，但不去动它们的 view
    m_viewer = viewer;
    m_context = context;

    m_imageWidth = w;
    m_imageHeight = h;
    m_focalLength = focalLength;
    m_generateImages = generateImages;
    m_viewConfig.focalLength = focalLength;
    m_viewConfig.aspectRatio = (double)w / h;
    m_usePerformanceMode = !ShouldUseHighQuality(w, h);
    m_lastImageWidth = w;
    m_lastImageHeight = h;
    m_imageDataBuffers.clear();

    Handle(V3d_View) imgView = GetOrCreateImageView(w, h);

    for (const auto& obj : m_offscreenExcludes)
    {
        if (!obj.IsNull())
        {
            obj->SetViewAffinity(imgView, false);
        }
    }

    CalculateModelBounds();

    try
    {
        if (generateImages)
        {
            RenderOneViewToBuffer(imgView, L"", w, h, m_viewConfig);
        }
    }
    catch (...)
    {
        return false;
    }

    return true;
}


void ModelImport::RenderOneViewToBuffer(const Handle(V3d_View)& view,
    const std::wstring& baseFileName,
    const int w, const int h,
    const ViewConfig& config)
{
    if (view.IsNull())
        throw std::runtime_error("RenderOneViewToBuffer: view is null");
    if (w <= 0 || h <= 0)
        throw std::runtime_error("RenderOneViewToBuffer: invalid image size");

    if (!m_preserveExternalViewState)
    {
        ConfigureRendering(view, config);

        if (m_scaleCalibrated)
        {
            ApplyScaleCalibration(view, w, h, /*keepCenter=*/true);
        }
        else if (m_boundsCalculated)
        {
            view->FitAll(0.01, Standard_False);
            view->ZFitAll();
        }
    }

    view->Redraw();

    Image_AlienPixMap pixmap;
    if (!view->ToPixMap(pixmap, w, h))
        throw std::runtime_error("ToPixMap failed in RenderOneViewToBuffer");

    const size_t bpp = 3;
    const size_t imageSize = (size_t)w * (size_t)h * bpp;
    m_imageDataBuffers.clear();
    m_imageDataBuffers.emplace_back(imageSize);

    unsigned char* dst = m_imageDataBuffers.back().data();
    const Standard_Byte* src = pixmap.Data();
    const Standard_Size  srcStep = pixmap.SizeRowBytes();

    for (int y = 0; y < h; ++y)
    {
        const Standard_Byte* srcRow =
            src + (ptrdiff_t)y * srcStep;
        unsigned char* dstRow =
            dst + (ptrdiff_t)(h - 1 - y) * w * bpp;

        for (int x = 0; x < w; ++x)
        {
            const int si = x * bpp;
            const int di = x * bpp;

            dstRow[di + 0] = srcRow[si + 2]; // B
            dstRow[di + 1] = srcRow[si + 1]; // G
            dstRow[di + 2] = srcRow[si + 0]; // R
        }
    }

    // DOF 后处理仍然只作用在图片 buffer 上
    if (m_enableDOF)
    {
        std::vector<float> depth;
        bool topDown = false;
        if (GrabDepth(view, w, h, depth, topDown))
        {
            PostProcessDOF_Basic(m_imageDataBuffers.back(), depth, w, h, view);
        }
    }

    if (m_generateImages && !baseFileName.empty())
    {
        std::wstring fileName = baseFileName + L"_current.png";
        TCollection_AsciiString fn(fileName.c_str());
        pixmap.Save(fn);
    }
}



// 找主窗口的可见 view（只读，用来拷贝初始相机）
Handle(V3d_View) ModelImport::FindMainView(const Handle(V3d_Viewer)& viewer)
{
    if (viewer.IsNull()) return Handle(V3d_View)();

    V3d_ListOfView defined = viewer->DefinedViews();
    for (V3d_ListOfViewIterator it(defined); it.More(); it.Next())
    {
        Handle(V3d_View) v = it.Value();
        Handle(Aspect_Window) w = v->Window();
        Handle(WNT_Window) ww = Handle(WNT_Window)::DownCast(w);
        if (!ww.IsNull() && !ww->IsVirtual())
        {
            return v;   // 主界面的 COccView 用的 view
        }
    }
    return Handle(V3d_View)();
}

Handle(V3d_View) ModelImport::GetOrCreateImageView(int w, int h)
{
    if (m_viewer.IsNull())
        throw std::runtime_error("GetOrCreateImageView: m_viewer is null");

    if (m_ImageView.IsNull())
    {
        m_ImageView = new V3d_View(m_viewer);
    }

    Graphic3d_Vec2i winSize(w, h);

    static int s_imgClassId = 0;
    ++s_imgClassId;
    std::string cls = "VirtualImgClass_" + std::to_string(s_imgClassId);
    std::string name = "VirtualImgWindow_" + std::to_string(s_imgClassId);

    TCollection_AsciiString clsName(cls.c_str());
    TCollection_AsciiString winName(name.c_str());

    Handle(WNT_WClass) wc =
        new WNT_WClass(clsName.ToCString(), nullptr, 0);
    Handle(WNT_Window) ww =
        new WNT_Window(winName.ToCString(), wc, WS_POPUP,
            64, 64, winSize.x(), winSize.y(),
            Quantity_NOC_BLACK);
    ww->SetVirtual(true);
    m_ImageView->SetWindow(ww);
    m_ImageView->MustBeResized();

    // 第一次创建时把主视图相机拷贝过来一次，之后就互不影响
    static bool s_camInited = false;
    if (!s_camInited)
    {
        Handle(V3d_View) mainView = FindMainView(m_viewer);
        if (!mainView.IsNull())
        {
            Handle(Graphic3d_Camera) src = mainView->Camera();
            Handle(Graphic3d_Camera) dst = m_ImageView->Camera();
            if (!src.IsNull() && !dst.IsNull())
            {
#if (OCC_VERSION_HEX >= 0x070500)
                dst->Copy(src);
#else
                // 旧版本可以手动拷贝
                dst->SetProjectionType(src->ProjectionType());
                dst->SetEye(src->Eye());
                dst->SetCenter(src->Center());
                dst->SetUp(src->Up());
                dst->SetDirection(src->Direction());
                dst->SetScale(src->Scale());
                dst->SetAspect(src->Aspect());
#endif
            }
        }
        s_camInited = true;
    }

    return m_ImageView;
}


void ModelImport::ExportCurrentView(const Handle(V3d_Viewer)& viewer,
    const Handle(AIS_InteractiveContext)& context,
    const std::wstring& baseFileName,
    const int w, const int h,
    const ViewConfig& config)
{
    if (viewer.IsNull() || context.IsNull())
        throw std::runtime_error("Invalid viewer/context in ExportCurrentView");
    if (w <= 0 || h <= 0)
        throw std::runtime_error("Invalid image size in ExportCurrentView");

    Handle(V3d_View) srcView;
    V3d_ListOfView defined = viewer->DefinedViews();
    for (V3d_ListOfViewIterator it(defined); it.More(); it.Next())
    {
        Handle(V3d_View) v = it.Value();
        Handle(Aspect_Window) aw = v->Window();
        Handle(WNT_Window) ww = Handle(WNT_Window)::DownCast(aw);
        if (!ww.IsNull() && !ww->IsVirtual())
        {
            srcView = v;
            break;
        }
    }
    if (srcView.IsNull())
        throw std::runtime_error("No non-virtual view found");

    struct OffscreenCache
    {
        Handle(V3d_Viewer) ownerViewer;
        Handle(V3d_View)   offView;
        Handle(WNT_WClass) winClass;
        Handle(WNT_Window) win;
        int W = 0;
        int H = 0;
        std::mutex mtx;
    };
    static OffscreenCache s;

    auto resizeWntWindow = [](const Handle(WNT_Window)& win, int newW, int newH)
    {
        if (win.IsNull()) return;
        HWND hwnd = (HWND)win->HWindow();
        if (hwnd != nullptr)
        {
            ::SetWindowPos(hwnd, NULL, 0, 0, newW, newH,
                SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        win->DoResize();
    };

    Handle(V3d_View) offView;
    {
        std::lock_guard<std::mutex> lk(s.mtx);

        const bool viewerChanged = (s.ownerViewer != viewer);
        if (viewerChanged || s.offView.IsNull())
        {
            s.ownerViewer = viewer;
            s.offView = new V3d_View(viewer);
            s.win.Nullify();
            s.winClass.Nullify();
            s.W = 0; s.H = 0;
        }

        if (s.winClass.IsNull())
        {
            std::string cls =
                "OffscreenClass_ECV_Cached_" +
                std::to_string(GetCurrentProcessId());
            TCollection_AsciiString className(cls.c_str());
            s.winClass = new WNT_WClass(className.ToCString(), nullptr, 0);
        }

        if (s.win.IsNull())
        {
            std::string nm =
                "OffscreenWindow_ECV_Cached_" +
                std::to_string(GetCurrentProcessId());
            TCollection_AsciiString windowName(nm.c_str());

            s.win = new WNT_Window(windowName.ToCString(),
                s.winClass,
                WS_POPUP,
                64, 64,
                w, h,
                Quantity_NOC_BLACK);
            s.win->SetVirtual(true);

            s.offView->SetWindow(s.win);
            s.offView->MustBeResized();

            s.W = w; s.H = h;
        }
        else if (s.W != w || s.H != h)
        {
            resizeWntWindow(s.win, w, h);
            s.offView->MustBeResized();
            s.W = w; s.H = h;
        }

        offView = s.offView;
    }


    Handle(Graphic3d_Camera) dstCam = offView->Camera();
    if (dstCam.IsNull())
        throw std::runtime_error("offView camera is null");

    if (m_hasVirtualCamera)
    {
        dstCam->SetProjectionType(Graphic3d_Camera::Projection_Perspective);
        dstCam->SetEye(m_camEye);
        dstCam->SetCenter(m_camCenter);
        dstCam->SetUp(m_camUp);
        dstCam->SetAspect(static_cast<Standard_Real>(w) / static_cast<Standard_Real>(h));
    }
    else
    {
        Handle(Graphic3d_Camera) srcCam = srcView->Camera();
        if (!srcCam.IsNull())
        {
#if (OCC_VERSION_HEX >= 0x070500)
            dstCam->Copy(srcCam);
#else
            dstCam->SetProjectionType(srcCam->ProjectionType());
            dstCam->SetEye(srcCam->Eye());
            dstCam->SetCenter(srcCam->Center());
            dstCam->SetUp(srcCam->Up());
            dstCam->SetDirection(srcCam->Direction());
            dstCam->SetScale(srcCam->Scale());
            dstCam->SetAspect(srcCam->Aspect());
#endif
            dstCam->SetAspect(static_cast<Standard_Real>(w) / static_cast<Standard_Real>(h));
        }
    }

    ConfigureRendering(offView, config);

    Quantity_Color bg = srcView->BackgroundColor();
    offView->SetBackgroundColor(bg);

    if (m_scaleCalibrated)
    {
        ApplyScaleCalibration(offView, w, h, /*keepCenter=*/true);
    }
    else if (m_boundsCalculated && !m_preserveExternalViewState)
    {
        offView->FitAll(0.01, Standard_False);
        offView->ZFitAll();
    }

    offView->Redraw();

    Image_AlienPixMap pixmap;
    if (!offView->ToPixMap(pixmap, w, h))
        throw std::runtime_error("ToPixMap failed in ExportCurrentView");

    const int bpp = 3;
    const size_t imageSize = (size_t)w * (size_t)h * (size_t)bpp;

    if (m_imageDataBuffers.size() != 1)
        m_imageDataBuffers.resize(1);

    std::vector<unsigned char>& out = m_imageDataBuffers[0];
    out.resize(imageSize);

    unsigned char* dst = out.data();
    const Standard_Byte* src = pixmap.Data();
    const Standard_Size srcStep = pixmap.SizeRowBytes();

    for (int y = 0; y < h; ++y)
    {
        const Standard_Byte* srcRow = src + (ptrdiff_t)y * (ptrdiff_t)srcStep;
        unsigned char* dstRow = dst + (ptrdiff_t)(h - 1 - y) * (ptrdiff_t)w * bpp;

        for (int x = 0; x < w; ++x)
        {
            const int si = x * bpp;
            const int di = x * bpp;
            dstRow[di + 0] = srcRow[si + 2]; // B
            dstRow[di + 1] = srcRow[si + 1]; // G
            dstRow[di + 2] = srcRow[si + 0]; // R
        }
    }

    // DOF
    if (m_enableDOF)
    {
        std::vector<float> depth;
        bool topDown = false;
        if (GrabDepth(offView, w, h, depth, topDown))
        {
            PostProcessDOF_Basic(out, depth, w, h, offView);
        }
    }

    if (m_generateImages && !baseFileName.empty())
    {
        std::wstring fileName = baseFileName + L"_current.png";
        TCollection_AsciiString fn(fileName.c_str());
        pixmap.Save(fn);
    }
}





// 视图导出方法
void ModelImport::ExportViewsToMemoryAndFile(const Handle(V3d_Viewer)& viewer,
    const Handle(AIS_InteractiveContext)& context,
    const std::wstring& baseFileName,
    const int w, const int h,
    const ViewConfig& config)
{
    Graphic3d_Vec2i windowSize(w, h);

    static int classCounter = 0;
    classCounter++;
    std::string uniqueClassName = "OffscreenClass_" + std::to_string(classCounter) +
        "_" + std::to_string(GetCurrentProcessId()) +
        "_" + std::to_string(GetCurrentThreadId());
    const TCollection_AsciiString className(uniqueClassName.c_str());
    std::string uniqueWindowsName = "OffscreenWindows_" + std::to_string(classCounter) +
        "_" + std::to_string(GetCurrentProcessId()) +
        "_" + std::to_string(GetCurrentThreadId());
    const TCollection_AsciiString windowName(uniqueWindowsName.c_str());

    Handle(WNT_WClass) windowClass = new WNT_WClass(className.ToCString(), nullptr, 0);
    Handle(WNT_Window) offscreenWindow = new WNT_Window(windowName.ToCString(),
        windowClass, WS_POPUP,
        64, 64, windowSize.x(), windowSize.y(),
        Quantity_NOC_BLACK);
    offscreenWindow->SetVirtual(true);

    Handle(V3d_View) view = new V3d_View(viewer);
    view->SetWindow(offscreenWindow);
    view->MustBeResized();

    if (!m_preserveExternalViewState) {
        ConfigureRendering(view, config);
    }

    // 生成所有视图
    GenerateAllViews(view, baseFileName, windowSize, config);
}



// 优化的生成所有视图方法
void ModelImport::GenerateAllViews(const Handle(V3d_View)& view,
    const std::wstring& baseFileName,
    const Graphic3d_Vec2i& windowSize,
    const ViewConfig& config)
{

    if (m_preserveExternalViewState) {
        ExportCurrentView(view->Viewer(), m_context, baseFileName, windowSize.x(), windowSize.y(), config);
        return;
    }

    struct ViewInfo
    {
        V3d_TypeOfOrientation orientation;
        std::wstring suffix;
    };

    const std::vector<ViewInfo> views = {
        {V3d_Ypos, L"_Front.png"},
        {V3d_Zpos, L"_Top.png"},
        {V3d_Xpos, L"_Right.png"},
        {V3d_XposYposZpos, L"_Incline.png"}
    };

    std::wstring baseName = baseFileName;
    size_t dotPos = baseName.rfind(L'.');
    if (dotPos != std::wstring::npos)
        baseName = baseName.substr(0, dotPos);

    m_imageDataBuffers.clear();
    m_imageDataBuffers.reserve(views.size());

    // 计算统一的缩放值
    double unifiedScale = 0.0;
    gp_Pnt unifiedCenter = m_modelCenter;

    // 使用正视图计算基准参数
    if (m_boundsCalculated)
    {
        SetupUnifiedViewParameters(view, config, unifiedScale, unifiedCenter);
        std::wcout << L"Unified scale calculated: " << unifiedScale
            << L" for focal length: " << config.focalLength << L"mm" << std::endl;
    }

    // 预分配单个图像的内存
    const size_t bytesPerPixel = 3;
    const size_t singleImageSize = windowSize.x() * windowSize.y() * bytesPerPixel;

    // 为每个视图生成图像
    for (const auto& v : views)
    {
        std::wcout << L"\nGenerating view: " << v.suffix << std::endl;

        // 设置视图
        SetupView(view, v.orientation, config, unifiedScale, unifiedCenter);

        // 生成图像
        Image_AlienPixMap image;
        bool imageGenerated = view->ToPixMap(image, windowSize.x(), windowSize.y());

        if (!imageGenerated)
        {
            std::wcerr << L"Failed to generate pixmap for view: " << v.suffix << std::endl;
            m_imageDataBuffers.emplace_back(); // 添加空缓冲区
            continue;
        }

        // 直接在vector中构造新元素，避免临时对象
        m_imageDataBuffers.emplace_back(singleImageSize);
        auto& currentBuffer = m_imageDataBuffers.back();

        // 复制数据并转换RGB到BGR
        const Standard_Byte* srcData = image.Data();
        unsigned char* destData = currentBuffer.data();

        for (size_t i = 0; i < windowSize.x() * windowSize.y(); ++i)
        {
            size_t offset = i * bytesPerPixel;
            destData[offset] = srcData[offset + 2];     // B
            destData[offset + 1] = srcData[offset + 1]; // G
            destData[offset + 2] = srcData[offset];     // R
        }

        // 根据标志位决定是否保存文件
        if (m_generateImages)
        {
            std::wstring focalStr = std::to_wstring(static_cast<int>(config.focalLength));
            std::string savePath = ConvertWStringToUtf8(baseName + L"_" +
                std::to_wstring(windowSize.x()) + L"x" +
                std::to_wstring(windowSize.y()) +
                L"_f" + focalStr + L"mm" + v.suffix);

            // 创建BGR格式的图像用于保存
            Image_AlienPixMap bgrImage;
            if (bgrImage.InitTrash(Image_Format_BGR, windowSize.x(), windowSize.y()))
            {
                // 复制BGR数据到新图像
                Standard_Byte* bgrData = bgrImage.ChangeData();
                std::memcpy(bgrData, currentBuffer.data(), singleImageSize);

                if (!bgrImage.Save(savePath.c_str()))
                {
                    std::wcerr << L"Failed to save image file: " << v.suffix << std::endl;
                }
                else
                {
                    std::wcout << L"Saved image file: " << v.suffix << std::endl;
                }
            }
        }
    }
}

// 优化的设置统一视图参数
void ModelImport::SetupUnifiedViewParameters(const Handle(V3d_View)& view,
    const ViewConfig& config,
    double& outScale,
    gp_Pnt& outCenter)
{
    if (m_preserveExternalViewState) { outScale = 0.0; outCenter = m_modelCenter; return; }
    if (!m_boundsCalculated)
    {
        view->FitAll(0.01, Standard_True);
        outScale = view->Scale();
        outCenter = gp_Pnt(0, 0, 0);
        return;
    }

    // 使用模型中心
    outCenter = config.useCustomFocus ? config.focusPoint : m_modelCenter;

    // 设置正视图
    view->SetProj(V3d_Ypos);

    // 使用FitAll自动调整
    view->FitAll(0.01, Standard_True);
    view->ZFitAll();

    // 确保视图中心对准模型中心
    view->SetAt(outCenter.X(), outCenter.Y(), outCenter.Z());

    // 获取FitAll后的缩放值作为基准
    double baseScale = view->Scale();

    // 根据焦距计算缩放调整
    double focalAdjustment = config.focalLength / 50.0;
    outScale = baseScale * focalAdjustment;

    std::wcout << L"Base scale: " << baseScale
        << L", Focal adjustment: " << focalAdjustment
        << L", Final scale: " << outScale << std::endl;
}

// 优化的设置视图方法
void ModelImport::SetupView(const Handle(V3d_View)& view,
    V3d_TypeOfOrientation orientation,
    const ViewConfig& config,
    double scale,
    const gp_Pnt& center)
{
    view->SetImmediateUpdate(Standard_False);

    if (!m_preserveExternalViewState) {
        view->SetProj(orientation);
        if (!m_scaleCalibrated) {
            view->FitAll(0.01, Standard_False);
            view->ZFitAll();
            view->SetAt(center.X(), center.Y(), center.Z());
            if (scale > 0) view->SetScale(scale);
        }
        else {
            if (m_viewConfig.useCustomFocus) {
                view->SetAt(m_viewConfig.focusPoint.X(), m_viewConfig.focusPoint.Y(), m_viewConfig.focusPoint.Z());
            }
            else if (m_boundsCalculated) {
                view->SetAt(m_modelCenter.X(), m_modelCenter.Y(), m_modelCenter.Z());
            }
            ApplyScaleCalibration(view, m_imageWidth, m_imageHeight);
        }
    }
    else {
        if (m_scaleCalibrated) {
            ApplyScaleCalibration(view, m_imageWidth, m_imageHeight, /*keepCenter=*/true);
        }
    }

    view->SetImmediateUpdate(Standard_True);
    view->Update();
}

// 优化的计算模型包围盒-> 边界线采样点，曲线取点采样
void ModelImport::CalculateModelBounds()
{
    if (m_context.IsNull())
    {
        m_boundsCalculated = false;
        return;
    }

    // 使用顶点索引映射来避免重复计算同一顶点
    TopTools_IndexedMapOfShape allVertices;
    int shapeCount = 0;

    // 处理XCAF文档中的形状
    if (!m_currentDoc.IsNull())
    {
        Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(m_currentDoc->Main());
        TDF_LabelSequence freeShapes;
        shapeTool->GetFreeShapes(freeShapes);

        for (Standard_Integer i = 1; i <= freeShapes.Length(); ++i)
        {
            TDF_Label label = freeShapes.Value(i);
            TopoDS_Shape shape = shapeTool->GetShape(label);

            if (!shape.IsNull())
            {
                // 将形状的所有顶点添加到映射中（自动去重）
                TopExp::MapShapes(shape, TopAbs_VERTEX, allVertices);
                shapeCount++;
            }
        }
    }

    // 处理AIS对象
    AIS_ListOfInteractive aObjects;
    m_context->ObjectsInside(aObjects, AIS_KindOfInteractive_None, -1);

    for (AIS_ListIteratorOfListOfInteractive it(aObjects); it.More(); it.Next())
    {
        Handle(AIS_InteractiveObject) obj = it.Value();
        if (obj.IsNull() || !m_context->IsDisplayed(obj)) continue;

        Handle(AIS_Shape) shapeObj = Handle(AIS_Shape)::DownCast(obj);
        if (!shapeObj.IsNull())
        {
            TopoDS_Shape shape = shapeObj->Shape();
            if (!shape.IsNull())
            {
                // 将形状的所有顶点添加到映射中（自动去重）
                TopExp::MapShapes(shape, TopAbs_VERTEX, allVertices);
                shapeCount++;
            }
        }
    }

    // 计算边界
    if (allVertices.Extent() > 0)
    {
        double xmin = DBL_MAX, ymin = DBL_MAX, zmin = DBL_MAX;
        double xmax = -DBL_MAX, ymax = -DBL_MAX, zmax = -DBL_MAX;

        // 遍历所有唯一顶点
        for (Standard_Integer i = 1; i <= allVertices.Extent(); ++i)
        {
            TopoDS_Vertex vertex = TopoDS::Vertex(allVertices(i));
            gp_Pnt point = BRep_Tool::Pnt(vertex);

            xmin = (xmin < point.X()) ? xmin : point.X();
            ymin = (ymin < point.Y()) ? ymin : point.Y();
            zmin = (zmin < point.Z()) ? zmin : point.Z();
            xmax = (xmax > point.X()) ? xmax : point.X();
            ymax = (ymax > point.Y()) ? ymax : point.Y();
            zmax = (zmax > point.Z()) ? zmax : point.Z();
        }

        // 对于曲线和曲面，添加关键点采样以提高精度
        // 处理边上的中点和参数点
        TopTools_IndexedMapOfShape allEdges;

        // 从XCAF文档中获取所有边
        if (!m_currentDoc.IsNull())
        {
            Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(m_currentDoc->Main());
            TDF_LabelSequence freeShapes;
            shapeTool->GetFreeShapes(freeShapes);

            for (Standard_Integer i = 1; i <= freeShapes.Length(); ++i)
            {
                TDF_Label label = freeShapes.Value(i);
                TopoDS_Shape shape = shapeTool->GetShape(label);
                if (!shape.IsNull())
                {
                    TopExp::MapShapes(shape, TopAbs_EDGE, allEdges);
                }
            }
        }

        // 从AIS对象中获取边
        for (AIS_ListIteratorOfListOfInteractive it(aObjects); it.More(); it.Next())
        {
            Handle(AIS_InteractiveObject) obj = it.Value();
            if (obj.IsNull() || !m_context->IsDisplayed(obj)) continue;

            Handle(AIS_Shape) shapeObj = Handle(AIS_Shape)::DownCast(obj);
            if (!shapeObj.IsNull())
            {
                TopoDS_Shape shape = shapeObj->Shape();
                if (!shape.IsNull())
                {
                    TopExp::MapShapes(shape, TopAbs_EDGE, allEdges);
                }
            }
        }

        // 对每条边采样几个点
        for (Standard_Integer i = 1; i <= allEdges.Extent(); ++i)
        {
            TopoDS_Edge edge = TopoDS::Edge(allEdges(i));
            if (!BRep_Tool::Degenerated(edge))
            {
                Standard_Real first, last;
                Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, first, last);
                if (!curve.IsNull())
                {
                    // 采样5个点：起点、终点已在顶点中，额外采样3个中间点
                    for (int j = 1; j <= 3; ++j)
                    {
                        Standard_Real param = first + (last - first) * j / 4.0;
                        gp_Pnt point = curve->Value(param);

                        xmin = (xmin < point.X()) ? xmin : point.X();
                        ymin = (ymin < point.Y()) ? ymin : point.Y();
                        zmin = (zmin < point.Z()) ? zmin : point.Z();
                        xmax = (xmax > point.X()) ? xmax : point.X();
                        ymax = (ymax > point.Y()) ? ymax : point.Y();
                        zmax = (zmax > point.Z()) ? zmax : point.Z();
                    }
                }
            }
        }

        m_modelMin = gp_Pnt(xmin, ymin, zmin);
        m_modelMax = gp_Pnt(xmax, ymax, zmax);
        m_modelCenter = gp_Pnt((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);

        m_modelDiameter = std::sqrt(std::pow(xmax - xmin, 2) +
            std::pow(ymax - ymin, 2) +
            std::pow(zmax - zmin, 2));

        m_modelDiameter *= 1.05;

        m_boundsCalculated = true;

        std::wcout << L"Model bounds calculated: " << shapeCount << L" shapes, "
            << allVertices.Extent() << L" vertices, "
            << L"Center: [" << m_modelCenter.X() << L", "
            << m_modelCenter.Y() << L", " << m_modelCenter.Z() << L"], "
            << L"Diameter: " << m_modelDiameter << std::endl;

        RebuildHeightIntersectorFromScene();
    }
    else
    {
        m_boundsCalculated = false;
        std::wcerr << L"Warning: Failed to calculate model bounds." << std::endl;
    }
}

// 配置渲染
void ModelImport::ConfigureRendering(const Handle(V3d_View)& view, const ViewConfig& config)
{
    if (m_preserveExternalViewState) {
        return;
    }
    // 设置着色模型为标准Phong
    view->SetShadingModel(Graphic3d_TOSM_FRAGMENT);

    // 设置纯白背景
    view->SetBackgroundColor(Quantity_NOC_WHITE);

    // 获取渲染参数引用
    Graphic3d_RenderingParams& params = view->ChangeRenderingParams();

    if (m_usePerformanceMode)
    {
        // 性能优先模式
        params.NbMsaaSamples = 2;
        params.RenderResolutionScale = 0.8f;
        params.ToEnableDepthPrepass = Standard_False;
    }
    else
    {
        // 质量优先模式  
        params.NbMsaaSamples = 4;
        params.RenderResolutionScale = 1.0f;
        params.ToEnableDepthPrepass = Standard_True; // 启用深度预处理
    }

    // 通用设置
    params.TransparencyMethod = Graphic3d_RTM_BLEND_UNORDERED; // 使用无序混合透明度
    params.ToShowStats = Standard_False;  // 不显示统计信息
    params.IsAntialiasingEnabled = Standard_True;  // 开启抗锯齿
    params.IsShadowEnabled = Standard_False;
    params.IsReflectionEnabled = Standard_False;
    params.ToneMappingMethod = Graphic3d_ToneMappingMethod_Disabled;
    params.Exposure = 0.0f;
    params.WhitePoint = 1.0f;

    // 根据焦距调整某些参数
    if (config.focalLength < 24.0) // 广角镜头
    {
        // 开启阴影和抗锯齿
        params.IsAntialiasingEnabled = Standard_True;
    }
    else if (config.focalLength > 85.0) // 长焦镜头
    {
        // 
        params.IsShadowEnabled = Standard_False;
    }

    view->SetComputedMode(Standard_False);
}

// 判断是否使用高质量模式
bool ModelImport::ShouldUseHighQuality(int width, int height) const
{
    const int highQualityThreshold = 3840 * 2160; // 4k
    int totalPixels = width * height;
    return totalPixels < highQualityThreshold;
}

// 视图配置方法
void ModelImport::SetViewScale(double scale)
{
    if (scale > 0.1 && scale <= 10.0)
    {
        m_viewConfig.viewScale = scale;
    }
}

void ModelImport::SetCustomFocusPoint(const gp_Pnt& point)
{
    m_viewConfig.useCustomFocus = true;
    m_viewConfig.focusPoint = point;
}

void ModelImport::ResetToAutoFocus()
{
    m_viewConfig.useCustomFocus = false;
}

// 获取模型包围盒信息
bool ModelImport::GetModelBounds(gp_Pnt& minPoint, gp_Pnt& maxPoint, gp_Pnt& center) const
{
    if (!m_boundsCalculated)
        return false;

    minPoint = m_modelMin;
    maxPoint = m_modelMax;
    center = m_modelCenter;
    return true;
}

// 加载模型
std::vector<Handle(AIS_InteractiveObject)> ModelImport::LoadModelFromFile(const std::wstring& filePath,
    const std::wstring& fileType)
{
    std::vector<Handle(AIS_InteractiveObject)> aisObjects;
    std::string file = ConvertWStringToUtf8(filePath);

    // 创建 XCAF 文档
    Handle(TDocStd_Application) app = XCAFApp_Application::GetApplication();
    Handle(TDocStd_Document) doc;
    app->NewDocument("MDTV-XCAF", doc);

    // 保存到成员变量
    m_currentDoc = doc;

    bool readSuccess = false;
    bool transferSuccess = false;
    if (fileType == L"stl")
    {
        try
        {
            TopoDS_Shape aShape;
            StlAPI_Reader stlReader;
            stlReader.Read(aShape, file.c_str());  // 抛异常/返回空都进入 catch
            if (aShape.IsNull())
            {
                std::cerr << "STL read returned null shape." << std::endl;
                return aisObjects;
            }

            Handle(AIS_Shape) ais = new AIS_Shape(aShape);
            // 和 step/iges 保持一致的可视属性
            Handle(Prs3d_Drawer) attributes = ais->Attributes();
            if (!attributes.IsNull())
            {
                attributes->SetFaceBoundaryDraw(Standard_True);
                attributes->SetDisplayMode(AIS_Shaded);
                Handle(Prs3d_LineAspect) prl =
                    new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 0.5);
                attributes->SetFaceBoundaryAspect(prl);
                attributes->SetIsoOnTriangulation(Standard_False);
                attributes->SetMaximalParameterValue(100000);
            }

            aisObjects.push_back(ais);
            std::wcout << L"Model loaded as STL (" << filePath << L")" << std::endl;
            return aisObjects; // STL 直接返回，不走 XCAF 分支
        }
        catch (...)
        {
            std::cerr << "Failed to read STL: " << file << std::endl;
            return aisObjects;
        }
    }

    // ====== 新增：PCD（ASCII）======
    if (fileType == L"pcd")
    {
        Handle(AIS_InteractiveObject) pcObj = LoadPCD_AsPointCloud(filePath);
        if (!pcObj.IsNull())
        {
            aisObjects.push_back(pcObj);
            std::wcout << L"Model loaded as PCD (" << filePath << L")" << std::endl;
            return aisObjects; // PCD 直接返回
        }
        else
        {
            std::cerr << "Failed to read PCD: " << file << std::endl;
            return aisObjects;
        }
    }

    if (fileType == L"step" || fileType == L"stp")
    {
        // 读取 STEP 文件
        STEPCAFControl_Reader reader;
        reader.SetColorMode(true);
        reader.SetNameMode(true);
        reader.SetMatMode(true);
        reader.SetLayerMode(true);
        reader.SetPropsMode(true);

        readSuccess = (reader.ReadFile(file.c_str()) == IFSelect_RetDone);
        if (readSuccess)
        {
            transferSuccess = reader.Transfer(doc);
        }
    }
    else if (fileType == L"iges" || fileType == L"igs")
    {
        // 读取 IGES 文件
        IGESCAFControl_Reader reader;
        reader.SetColorMode(true);
        reader.SetNameMode(true);
        reader.SetLayerMode(true);

        readSuccess = (reader.ReadFile(file.c_str()) == IFSelect_RetDone);
        if (readSuccess)
        {
            transferSuccess = reader.Transfer(doc);
        }
    }

    if (!readSuccess)
    {
        std::cerr << "Read failed: " << file << std::endl;
        return aisObjects;
    }

    if (!transferSuccess)
    {
        std::cerr << "Transfer failed." << std::endl;
        return aisObjects;
    }

    // 获取工具
    Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(doc->Main());

    TDF_LabelSequence freeShapes;
    shapeTool->GetFreeShapes(freeShapes);

    // 预分配vector大小
    aisObjects.resize(freeShapes.Length());

    // 移除多线程处理，改为单线程
    std::wcout << L"Creating AIS objects for " << freeShapes.Length() << L" shapes" << std::endl;

    // 单线程处理每个形状
    for (Standard_Integer i = 1; i <= freeShapes.Length(); ++i)
    {
        TDF_Label shapeLabel = freeShapes.Value(i);

        // 创建自动处理装配的对象
        Handle(XCAFPrs_AISObject) xcafObject = new XCAFPrs_AISObject(shapeLabel);
        xcafObject->SetDisplayMode(AIS_Shaded);

        // 设置属性
        Handle(Prs3d_Drawer) attributes = xcafObject->Attributes();
        if (!attributes.IsNull())
        {
            attributes->SetFaceBoundaryDraw(Standard_True);
            attributes->SetDisplayMode(AIS_Shaded);

            Handle(Prs3d_LineAspect) prl = new Prs3d_LineAspect(Quantity_NOC_BLACK, Aspect_TOL_SOLID, 0.5);
            attributes->SetFaceBoundaryAspect(prl);
            attributes->SetIsoOnTriangulation(Standard_False);
            attributes->SetMaximalParameterValue(100000);
        }

        aisObjects[i - 1] = xcafObject;
    }

    std::wcout << L"Model loaded with " << aisObjects.size() << L" top-level assemblies." << std::endl;
    return aisObjects;
}

// 读取 ASCII PCD: FIELDS x y z [rgb]，生成 AIS_PointCloud；失败返回 Null
Handle(AIS_InteractiveObject) ModelImport::LoadPCD_AsPointCloud(const std::wstring& wfile)
{
    // 读取文本
    std::ifstream ifs;
#ifdef _WIN32
    // MSVC 的 libstdc++ 支持用 wstring 打开（Windows 平台）
    ifs.open(wfile);
#else
    ifs.open(ConvertWStringToUtf8(wfile));
#endif
    if (!ifs.is_open()) return Handle(AIS_InteractiveObject)();

    // 解析头
    bool hasX = false, hasY = false, hasZ = false, hasRGB = false, isAscii = false;
    int  declaredPoints = -1;
    std::string line;
    while (std::getline(ifs, line))
    {
        if (line.empty()) continue;
        if (line[0] == '#') continue;

        // 大写头关键字
        auto up = [](std::string s) { for (auto& c : s) c = (char)std::toupper((unsigned char)c); return s; };
        std::string u = up(line);

        if (u.rfind("FIELDS", 0) == 0)
        {
            // 例：FIELDS x y z rgb
            std::istringstream iss(line.substr(6));
            std::string f;
            while (iss >> f)
            {
                if (f == "x") hasX = true;
                else if (f == "y") hasY = true;
                else if (f == "z") hasZ = true;
                else if (f == "rgb" || f == "rgba") hasRGB = true;
            }
        }
        else if (u.rfind("POINTS", 0) == 0)
        {
            declaredPoints = std::atoi(line.substr(6).c_str());
        }
        else if (u.rfind("DATA", 0) == 0)
        {
            isAscii = (u.find("ASCII") != std::string::npos);
            break; // 到 DATA 行结束头部
        }
    }

    if (!isAscii || !hasX || !hasY || !hasZ)
    {
        std::cerr << "PCD must be DATA ascii and contain x y z fields." << std::endl;
        return Handle(AIS_InteractiveObject)();
    }

    // 读数据
    std::vector<gp_Pnt> points;
    points.reserve(declaredPoints > 0 ? declaredPoints : 100000);

    std::vector<Quantity_Color> colors;
    if (hasRGB) colors.reserve(points.capacity());

    double xmin = DBL_MAX, ymin = DBL_MAX, zmin = DBL_MAX;
    double xmax = -DBL_MAX, ymax = -DBL_MAX, zmax = -DBL_MAX;

    std::streampos dataPos = ifs.tellg();
    while (std::getline(ifs, line))
    {
        if (line.empty() || line[0] == '#') continue;
        std::istringstream iss(line);
        double x, y, z;
        if (!(iss >> x >> y >> z)) continue;

        points.emplace_back(x, y, z);
        xmin = (std::min)(xmin, x); ymin = (std::min)(ymin, y); zmin = (std::min)(zmin, z);
        xmax = (std::max)(xmax, x); ymax = (std::max)(ymax, y); zmax = (std::max)(zmax, z);

        if (hasRGB)
        {
            // PCD 的 rgb 可能是 float（打包）或 int，这里按 float 读取再 reinterpret
            float rgbF = 0.0f;
            if (iss >> rgbF)
            {
                uint32_t rgb = *reinterpret_cast<uint32_t*>(&rgbF);
                int r = (rgb >> 16) & 0xFF;
                int g = (rgb >> 8) & 0xFF;
                int b = (rgb) & 0xFF;
                colors.emplace_back((double)r / 255.0, (double)g / 255.0, (double)b / 255.0, Quantity_TOC_RGB);
            }
        }
    }

    if (points.empty())
    {
        std::cerr << "PCD contains no points." << std::endl;
        return Handle(AIS_InteractiveObject)();
    }

    // 构建点数组（可携带顶点颜色）
#if (OCC_VERSION_HEX >= 0x070600)
    Handle(Graphic3d_ArrayOfPoints) arr =
        new Graphic3d_ArrayOfPoints((Standard_Integer)points.size(),
            hasRGB ? Graphic3d_ArrayFlags_VertexColor : Graphic3d_ArrayFlags_None);
#else
    Handle(Graphic3d_ArrayOfPoints) arr =
        new Graphic3d_ArrayOfPoints((Standard_Integer)points.size(),
            hasRGB ? Standard_True : Standard_False); // 老版本接口
#endif

    for (size_t i = 0; i < points.size(); ++i)
    {
        if (hasRGB && i < colors.size())
            arr->AddVertex(points[i], colors[i]);
        else
            arr->AddVertex(points[i]);
    }

    Handle(AIS_PointCloud) pc = new AIS_PointCloud();
    pc->SetPoints(arr);
    // 点大小/颜色可按需设置
    // Handle(Prs3d_PointAspect) pa = new Prs3d_PointAspect(Aspect_TOM_POINT, Quantity_NOC_WHITE, 2.0);
    // pc->Attributes()->SetPointAspect(pa);

    // 直接把点云的包围盒写入成员（让后续视图/缩放好用）
    m_modelMin = gp_Pnt(xmin, ymin, zmin);
    m_modelMax = gp_Pnt(xmax, ymax, zmax);
    m_modelCenter = gp_Pnt((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);
    m_modelDiameter = std::sqrt(std::pow(xmax - xmin, 2) + std::pow(ymax - ymin, 2) + std::pow(zmax - zmin, 2)) * 1.05;
    m_boundsCalculated = true;

    return Handle(AIS_InteractiveObject)::DownCast(pc);
}


// 显示形状
void ModelImport::DisplayShapesWithShadedMode(const std::vector<Handle(AIS_InteractiveObject)>& objects)
{
    if (m_context.IsNull())
        return;

    m_context->RemoveAll(Standard_False);

    for (const auto& object : objects) {
        m_context->Display(object, Standard_False);
        if (!m_preserveExternalViewState) {
            m_context->SetDisplayMode(object, AIS_Shaded, Standard_False);
        }
    }
    m_context->UpdateCurrentViewer();
}

// 获取所有视图数据
const std::vector<std::vector<unsigned char>>& ModelImport::GetAllViewData() const
{
    return m_imageDataBuffers;
}

// 获取指定视图数据
const std::vector<unsigned char>* ModelImport::GetViewData(size_t index) const
{
    if (index >= m_imageDataBuffers.size())
        return nullptr;
    return &m_imageDataBuffers[index];
}

// 加载模型（带缓存）
std::vector<Handle(AIS_InteractiveObject)> ModelImport::LoadModelWithCache(const std::wstring& filePath)
{
    // 检查缓存
    auto it = m_loadedModelsCache.find(filePath);
    if (it != m_loadedModelsCache.end())
    {
        UpdateCacheOrder(filePath);
        return it->second;
    }

    // 加载模型
    std::wstring ext = GetFileExtensionLower(filePath);
    std::vector<Handle(AIS_InteractiveObject)> loadedObjects = LoadModelFromFile(filePath, ext);

    // 存入缓存
    if (!loadedObjects.empty())
    {
        m_loadedModelsCache[filePath] = loadedObjects;
        UpdateCacheOrder(filePath);
        MaintainCacheSize();
    }

    return loadedObjects;
}

// 字符串转换
std::string ModelImport::ConvertWStringToUtf8(const std::wstring& wstr)
{
    if (wstr.empty()) return std::string();
    try {
        std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t> converter;
        return converter.to_bytes(wstr);
    }
    catch (const std::range_error&) {
        return std::string();
    }
}

// 获取文件扩展名（小写）
std::wstring ModelImport::GetFileExtensionLower(const std::wstring& filename)
{
    size_t pos = filename.rfind(L'.');
    if (pos == std::wstring::npos)
        return L"";
    std::wstring ext = filename.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);
    return ext;
}

// 初始化显示
bool ModelImport::InitializeDisplay()
{
    m_displayConnection = new Aspect_DisplayConnection();
    if (m_displayConnection.IsNull())
    {
        std::wcerr << L"Failed to create display connection." << std::endl;
        return false;
    }
    return true;
}

// 初始化查看器
bool ModelImport::InitializeViewer()
{
    Handle(Graphic3d_GraphicDriver) graphicDriver = new OpenGl_GraphicDriver(m_displayConnection);
    if (graphicDriver.IsNull())
    {
        std::wcerr << L"Failed to initialize OpenGl graphic driver." << std::endl;
        return false;
    }

    m_viewer = new V3d_Viewer(graphicDriver);
    if (m_viewer.IsNull())
    {
        std::wcerr << L"Failed to create V3d_Viewer." << std::endl;
        return false;
    }

    SetupLights();
    return true;
}

// 初始化上下文
bool ModelImport::InitializeContext()
{
    m_context = new AIS_InteractiveContext(m_viewer);
    if (m_context.IsNull())
    {
        std::wcerr << L"Failed to create AIS_InteractiveContext." << std::endl;
        return false;
    }
    return true;
}

// 设置光照
void ModelImport::SetupLights()
{
    if (m_viewer.IsNull())
        return;

    if (m_preserveExternalViewState) {
        return;
    }

    // 环境光
    Handle(Graphic3d_CLight) ambientLight = new Graphic3d_CLight(Graphic3d_TypeOfLightSource_Ambient);
    ambientLight->SetColor(Quantity_Color(0.8, 0.8, 0.8, Quantity_TOC_RGB)); // 白色环境光
    ambientLight->SetIntensity(0.9);
    m_viewer->AddLight(ambientLight);
    m_viewer->SetLightOn(ambientLight);

    // 基础方向光
    Quantity_Color c(Quantity_NOC_WHITE);
    Standard_Real intensities[4] = { 1.2, 1.0, 0.8, 0.6 };
    gp_Dir directions[4] = {
        gp_Dir(-1.0, 0.0, 0.0),
        gp_Dir(0.0, -1.0, 0.0),
        gp_Dir(0.0, 0.0, -1.0),
    };

    for (int i = 0; i < 3; ++i)
    {
        Handle(Graphic3d_CLight) dirLight = new Graphic3d_CLight(Graphic3d_TypeOfLightSource_Directional);
        dirLight->SetColor(c);
        dirLight->SetIntensity(intensities[i]);
        dirLight->SetDirection(directions[i]);
        m_viewer->AddLight(dirLight);
        m_viewer->SetLightOn(dirLight);
    }

    m_viewer->UpdateLights();
}

// 清理缓存
void ModelImport::ClearModelCache()
{
    size_t cacheSize = m_loadedModelsCache.size();

    // 首先从AIS上下文中移除所有显示的对象
    if (!m_context.IsNull())
    {
        for (auto& pair : m_loadedModelsCache)
        {
            for (auto& object : pair.second)
            {
                if (!object.IsNull() && m_context->IsDisplayed(object))
                {
                    m_context->Remove(object, Standard_False);
                }
            }
        }
        m_context->UpdateCurrentViewer();
    }

    // 然后清理Handle对象
    for (auto& pair : m_loadedModelsCache)
    {
        for (auto& object : pair.second)
        {
            if (!object.IsNull())
            {
                object.Nullify();
            }
        }
        pair.second.clear();
    }

    m_loadedModelsCache.clear();
    m_cacheOrder.clear();
    InvalidateHeightIntersector();
    m_boundsCalculated = false;

    std::wcout << L"Model cache cleared. Removed " << cacheSize << L" cached models." << std::endl;
}

// 设置最大缓存数量
void ModelImport::SetMaxCache(size_t maxCache)
{
    m_maxCache = maxCache;
    ClearModelCache();
    std::wcout << L"Max cache size set to: " << m_maxCache << L", cache cleared." << std::endl;
}

// 获取最大缓存数量
size_t ModelImport::GetMaxCache() const
{
    return m_maxCache;
}

// 获取当前缓存大小
size_t ModelImport::GetCurrentCacheSize() const
{
    return m_loadedModelsCache.size();
}

// 维护缓存大小
void ModelImport::MaintainCacheSize()
{
    while (m_loadedModelsCache.size() > m_maxCache && !m_cacheOrder.empty())
    {
        std::wstring oldestPath = m_cacheOrder.back();
        m_cacheOrder.pop_back();

        auto it = m_loadedModelsCache.find(oldestPath);
        if (it != m_loadedModelsCache.end())
        {
            if (!m_context.IsNull())
            {
                for (auto& object : it->second)
                {
                    if (!object.IsNull() && m_context->IsDisplayed(object))
                    {
                        m_context->Remove(object, Standard_False);
                    }
                }
            }

            for (auto& object : it->second)
            {
                if (!object.IsNull())
                {
                    object.Nullify();
                }
            }

            m_loadedModelsCache.erase(it);
            std::wcout << L"Cache evicted: " << oldestPath << std::endl;
        }
    }

    if (!m_context.IsNull())
    {
        m_context->UpdateCurrentViewer();
    }
}

// 更新缓存访问顺序
void ModelImport::UpdateCacheOrder(const std::wstring& filePath)
{
    auto it = std::find(m_cacheOrder.begin(), m_cacheOrder.end(), filePath);
    if (it != m_cacheOrder.end())
    {
        m_cacheOrder.erase(it);
    }
    m_cacheOrder.push_front(filePath);
}

// 检查文件是否存在
bool ModelImport::FileExists(const std::wstring& filePath)
{
#ifdef _WIN32
    DWORD dwAttrib = GetFileAttributesW(filePath.c_str());
    return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
        !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
#else
    std::string filePathStr = ConvertWStringToUtf8(filePath);
    struct stat buffer;
    return (stat(filePathStr.c_str(), &buffer) == 0);
#endif
}

// 验证分辨率
bool ModelImport::IsValidResolution(int width, int height)
{
    // 基本范围检查
    if (width <= 0 || height <= 0)
    {
        std::wcerr << L"Error: Resolution must be positive" << std::endl;
        return false;
    }

    // 最小分辨率限制
    const int MIN_SIZE = 100;
    if (width < MIN_SIZE || height < MIN_SIZE)
    {
        std::wcerr << L"Error: Resolution too small (minimum " << MIN_SIZE << L"x" << MIN_SIZE << L")" << std::endl;
        return false;
    }

    // 最大分辨率限制
    const int MAX_WIDTH = 7680;   // 8K width
    const int MAX_HEIGHT = 4320;  // 8K height
    if (width > MAX_WIDTH || height > MAX_HEIGHT)
    {
        std::wcerr << L"Error: Resolution too large (maximum " << MAX_WIDTH << L"x" << MAX_HEIGHT << L")" << std::endl;
        return false;
    }

    // 检查像素总数
    const long long MAX_PIXELS = 33177600; // 7680x4320 = 33MP
    long long totalPixels = static_cast<long long>(width) * height;
    if (totalPixels > MAX_PIXELS)
    {
        std::wcerr << L"Error: Too many pixels" << std::endl;
        return false;
    }

    // 估算内存使用量
    const long long BYTES_PER_PIXEL = 3;
    const long long VIEWS_COUNT = 4;
    long long estimatedMemory = totalPixels * BYTES_PER_PIXEL * VIEWS_COUNT;
    const long long MAX_MEMORY_MB = 512;
    const long long MAX_MEMORY_BYTES = MAX_MEMORY_MB * 1024 * 1024;

    if (estimatedMemory > MAX_MEMORY_BYTES)
    {
        std::wcerr << L"Error: Estimated memory usage too high" << std::endl;
        return false;
    }

    return true;
}

// 设置图像尺寸
bool ModelImport::SetImageSize(int width, int height)
{
    // 验证分辨率参数
    if (!IsValidResolution(width, height))
    {
        std::wcerr << L"Error: Invalid image size " << width << L"x" << height << std::endl;
        return false;
    }

    m_imageWidth = width;
    m_imageHeight = height;

    // 更新视图配置的宽高比
    m_viewConfig.aspectRatio = static_cast<double>(width) / height;
    Handle(V3d_Viewer) m_viewer = GetViewer();
    Handle(AIS_InteractiveContext) m_context = GetContext();
    // 更新性能模式判断
    m_usePerformanceMode = !ShouldUseHighQuality(width, height);
    ImportExportViews(m_viewer, m_context, width, height, true, m_focalLength);
//    ImportModelAndExportViews(m_filePath, width, height, false, m_focalLength);
    std::wcout << L"Image size set to: " << width << L"x" << height << std::endl;
    return true;
}

// 获取图像尺寸
void ModelImport::GetImageSize(int& width, int& height) const
{
    width = m_imageWidth;
    height = m_imageHeight;
}

// 设置焦距
bool ModelImport::SetFocalLength(double focalLength)
{
    // 验证焦距参数
    if (focalLength <= 0.0 || focalLength > 500.0)
    {
        std::wcerr << L"Error: Invalid focal length " << focalLength
            << L"mm (valid range: 0.1-500)" << std::endl;
        return false;
    }

    m_focalLength = focalLength;
    m_viewConfig.focalLength = focalLength;
    ImportModelAndExportViews(m_filePath, m_imageWidth, m_imageHeight, false, focalLength);
    std::wcout << L"Focal length set to: " << focalLength << L"mm" << std::endl;
    return true;
}

// 获取焦距
double ModelImport::GetFocalLength() const
{
    return m_focalLength;
}


// 从 AIS_InteractiveContext 中解析所有几何对象
std::vector<Handle(AIS_InteractiveObject)> ModelImport::ParseObjectsFromContext(Handle(AIS_InteractiveContext)& context)
{
    std::vector<Handle(AIS_InteractiveObject)> parsedObjects;

    if (context.IsNull())
    {
        std::wcerr << L"Context is null in ParseObjectsFromContext" << std::endl;
        return parsedObjects;
    }

    // 获取 context 中的所有对象（包括显示和未显示的）
    AIS_ListOfInteractive allObjects;
    context->ObjectsInside(allObjects, AIS_KindOfInteractive_None, -1);

    std::wcout << L"Found " << allObjects.Extent() << L" objects in context" << std::endl;

    if (allObjects.IsEmpty())
    {
        std::wcerr << L"No objects found in context" << std::endl;
        return parsedObjects;
    }

    // 遍历所有对象并进行解析
    for (AIS_ListIteratorOfListOfInteractive iter(allObjects); iter.More(); iter.Next())
    {
        Handle(AIS_InteractiveObject) obj = iter.Value();

        if (obj.IsNull())
            continue;

        // 解析不同类型的 AIS 对象
        std::vector<Handle(AIS_InteractiveObject)> parsedFromObject = ParseSingleAISObject(obj);

        // 将解析结果添加到总列表中
        parsedObjects.insert(parsedObjects.end(), parsedFromObject.begin(), parsedFromObject.end());
    }

    std::wcout << L"Successfully parsed " << parsedObjects.size() << L" geometry objects" << std::endl;
    return parsedObjects;
}

// 解析单个 AIS 对象
//std::vector<Handle(AIS_InteractiveObject)> ModelImport::ParseSingleAISObject(Handle(AIS_InteractiveObject)& obj)
//{
//    std::vector<Handle(AIS_InteractiveObject)> results;
//
//    if (obj.IsNull())
//        return results;
//
//    try
//    {
//        // 尝试转换为不同类型的 AIS 对象
//
//        // 1. 检查是否是 XCAFPrs_AISObject（XCAF 装配体对象）
//        Handle(XCAFPrs_AISObject) xcafObj = Handle(XCAFPrs_AISObject)::DownCast(obj);
//        if (!xcafObj.IsNull())
//        {
//            std::wcout << L"  Found XCAFPrs_AISObject" << std::endl;
//            // XCAF 对象已经包含了颜色、材质等信息，直接使用
//            results.push_back(xcafObj);
//            return results;
//        }
//
//        // 2. 检查是否是普通的 AIS_Shape 对象
//        Handle(AIS_Shape) shapeObj = Handle(AIS_Shape)::DownCast(obj);
//        if (!shapeObj.IsNull())
//        {
//            std::wcout << L"  Found AIS_Shape object" << std::endl;
//            TopoDS_Shape shape = shapeObj->Shape();
//
//            if (!shape.IsNull())
//            {
//                // 解析形状并创建增强的对象
//                ParseAndCreateEnhancedObjects(shape, shapeObj, results);
//            }
//            return results;
//        }
//
//        // 3. 检查其他类型的 AIS 对象
//        Handle(AIS_ColoredShape) coloredShapeObj = Handle(AIS_ColoredShape)::DownCast(obj);
//        if (!coloredShapeObj.IsNull())
//        {
//            std::wcout << L"  Found AIS_ColoredShape object" << std::endl;
//            TopoDS_Shape shape = coloredShapeObj->Shape();
//
//            if (!shape.IsNull())
//            {
//                // 创建保持颜色信息的对象
//                Handle(AIS_ColoredShape) newColoredObj = CreateColoredShapeObject(shape, coloredShapeObj);
//                if (!newColoredObj.IsNull())
//                {
//                    results.push_back(newColoredObj);
//                }
//            }
//            return results;
//        }
//
//        // 4. 对于其他类型的 AIS 对象，直接添加
//        std::wcout << L"  Found other AIS object type" << std::endl;
//        results.push_back(obj);
//
//    }
//    catch (const Standard_Failure& e)
//    {
//        std::wcerr << L"Error parsing AIS object: " << e.GetMessageString() << std::endl;
//    }
//
//    return results;
//}
std::vector<Handle(AIS_InteractiveObject)> ModelImport::ParseSingleAISObject(Handle(AIS_InteractiveObject)& obj)
{
    std::vector<Handle(AIS_InteractiveObject)> results;
    if (obj.IsNull())
        return results;

    results.push_back(obj);
    return results;
}


// 解析形状并创建增强的对象
void ModelImport::ParseAndCreateEnhancedObjects(const TopoDS_Shape& shape,
    Handle(AIS_Shape) originalObj,
    std::vector<Handle(AIS_InteractiveObject)>& results)
{
    if (shape.IsNull())
        return;

    try
    {
        switch (shape.ShapeType())
        {
        case TopAbs_COMPOUND:
        {
            std::wcout << L"    Parsing compound shape" << std::endl;
            // 递归解析复合形状
            TopoDS_Iterator shapeIter(shape);
            for (; shapeIter.More(); shapeIter.Next())
            {
                TopoDS_Shape subShape = shapeIter.Value();
                ParseAndCreateEnhancedObjects(subShape, originalObj, results);
            }
            break;
        }
        case TopAbs_COMPSOLID:
        case TopAbs_SOLID:
        {
            std::wcout << L"    Parsing solid shape" << std::endl;
            Handle(AIS_Shape) newObj = CreateEnhancedShapeWithGeometryInfo(shape, originalObj);
            if (!newObj.IsNull())
            {
                results.push_back(newObj);
            }
            break;
        }
        case TopAbs_SHELL:
        case TopAbs_FACE:
        {
            std::wcout << L"    Parsing surface shape" << std::endl;
            Handle(AIS_Shape) newObj = CreateEnhancedShapeWithGeometryInfo(shape, originalObj);
            if (!newObj.IsNull())
            {
                results.push_back(newObj);
            }
            break;
        }
        case TopAbs_WIRE:
        case TopAbs_EDGE:
        {
            std::wcout << L"    Parsing curve shape" << std::endl;
            Handle(AIS_Shape) newObj = CreateEnhancedShapeWithGeometryInfo(shape, originalObj);
            if (!newObj.IsNull())
            {
                results.push_back(newObj);
            }
            break;
        }
        default:
            // 对于其他类型，创建基本对象
            Handle(AIS_Shape) newObj = new AIS_Shape(shape);
            if (!newObj.IsNull())
            {
                CopyBasicAttributes(originalObj, newObj);
                results.push_back(newObj);
            }
            break;
        }
    }
    catch (const Standard_Failure& e)
    {
        std::wcerr << L"Error in ParseAndCreateEnhancedObjects: " << e.GetMessageString() << std::endl;
    }
}

// 创建带几何信息的增强形状对象
Handle(AIS_Shape) ModelImport::CreateEnhancedShapeWithGeometryInfo(const TopoDS_Shape& shape,
    Handle(AIS_Shape) originalObj)
{
    if (shape.IsNull())
        return Handle(AIS_Shape)();

    try
    {
        Handle(AIS_Shape) newObj = new AIS_Shape(shape);

        // 复制原始对象的属性
        CopyBasicAttributes(originalObj, newObj);

        // 根据几何类型设置特定属性
        SetGeometrySpecificAttributes(newObj, shape.ShapeType());

        return newObj;
    }
    catch (const Standard_Failure& e)
    {
        std::wcerr << L"Error creating enhanced shape: " << e.GetMessageString() << std::endl;
        return Handle(AIS_Shape)();
    }
}

// 创建彩色形状对象
Handle(AIS_ColoredShape) ModelImport::CreateColoredShapeObject(const TopoDS_Shape& shape,
    Handle(AIS_ColoredShape) originalObj)
{
    if (shape.IsNull() || originalObj.IsNull())
        return Handle(AIS_ColoredShape)();

    try
    {
        Handle(AIS_ColoredShape) newObj = new AIS_ColoredShape(shape);

        // 复制颜色信息
        CopyColoredShapeAttributes(originalObj, newObj);

        return newObj;
    }
    catch (const Standard_Failure& e)
    {
        std::wcerr << L"Error creating colored shape: " << e.GetMessageString() << std::endl;
        return Handle(AIS_ColoredShape)();
    }
}

// 复制基本属性
void ModelImport::CopyBasicAttributes(Handle(AIS_Shape) sourceObj, Handle(AIS_Shape) targetObj)
{
    if (sourceObj.IsNull() || targetObj.IsNull())
        return;

    try
    {
        // 复制显示模式
        targetObj->SetDisplayMode(sourceObj->DisplayMode());

        // 复制颜色
        if (sourceObj->HasColor())
        {
            Quantity_Color color;
            sourceObj->Color(color);
            targetObj->SetColor(color);
        }

        // 复制透明度
        if (sourceObj->IsTransparent())
        {
            targetObj->SetTransparency(sourceObj->Transparency());
        }

        // 复制材质
        if (sourceObj->HasMaterial())
        {
            targetObj->SetMaterial(sourceObj->Material());
        }
    }
    catch (const Standard_Failure& e)
    {
        std::wcerr << L"Error copying basic attributes: " << e.GetMessageString() << std::endl;
    }
}

// 复制彩色形状属性 - 根据原代码处理流程修改
void ModelImport::CopyColoredShapeAttributes(Handle(AIS_ColoredShape) sourceObj,
    Handle(AIS_ColoredShape) targetObj)
{
    if (sourceObj.IsNull() || targetObj.IsNull())
        return;

    try
    {
        // 复制基本属性（和原代码LoadModelFromFile中的处理方式一致）
        CopyBasicAttributes(Handle(AIS_Shape)::DownCast(sourceObj),
            Handle(AIS_Shape)::DownCast(targetObj));

        // 根据原代码中XCAFPrs_AISObject的处理方式，使用XCAFPrs_Style来处理颜色
        TopoDS_Shape shape = sourceObj->Shape();
        if (!shape.IsNull())
        {
            // 检查是否是XCAFPrs_AISObject类型
            Handle(XCAFPrs_AISObject) xcafSourceObj = Handle(XCAFPrs_AISObject)::DownCast(sourceObj);
            if (!xcafSourceObj.IsNull())
            {
                // 对于XCAF对象，颜色信息已经自动处理，直接复制基本属性即可
                std::wcout << L"      Source is XCAFPrs_AISObject - colors handled automatically" << std::endl;
                return;
            }

            // 对于普通的AIS_ColoredShape，使用传统方法处理
            // 遍历所有面并复制颜色（使用正确的API）
            TopExp_Explorer exp(shape, TopAbs_FACE);
            for (; exp.More(); exp.Next())
            {
                const TopoDS_Face& face = TopoDS::Face(exp.Current());

                try
                {
                    // 检查面是否有自定义颜色
                }
                catch (...)
                {
                    // 某些面可能不支持颜色，继续处理下一个面
                    continue;
                }
            }

            // 复制整体对象属性（和原代码中的属性设置方式一致）
            if (sourceObj->HasColor())
            {
                Quantity_Color objColor;
                sourceObj->Color(objColor);
                if (!targetObj->HasColor())
                {
                    targetObj->SetColor(objColor);
                    std::wcout << L"      Copied object color: R="
                        << objColor.Red() << L" G=" << objColor.Green()
                        << L" B=" << objColor.Blue() << std::endl;
                }
            }

            // 复制材质（参考原代码中的材质处理）
            if (sourceObj->HasMaterial())
            {
                targetObj->SetMaterial(sourceObj->Material());
                std::wcout << L"      Copied material" << std::endl;
            }

            // 复制透明度
            if (sourceObj->IsTransparent())
            {
                Standard_Real transparency = sourceObj->Transparency();
                targetObj->SetTransparency(transparency);
                std::wcout << L"      Copied transparency: " << transparency << std::endl;
            }
        }

        std::wcout << L"    Successfully copied colored shape attributes using original code style" << std::endl;
    }
    catch (const Standard_Failure& e)
    {
        std::wcerr << L"Error copying colored shape attributes: " << e.GetMessageString() << std::endl;
    }
    catch (...)
    {
        std::wcerr << L"Unknown error in CopyColoredShapeAttributes" << std::endl;
    }
}

// 根据几何类型设置特定属性
void ModelImport::SetGeometrySpecificAttributes(Handle(AIS_Shape) shapeObj, TopAbs_ShapeEnum shapeType)
{
    if (shapeObj.IsNull())
        return;

    try
    {
        Handle(Prs3d_Drawer) attributes = shapeObj->Attributes();
        if (attributes.IsNull())
            return;

        switch (shapeType)
        {
        case TopAbs_SOLID:
        case TopAbs_COMPSOLID:
        {
            if (!shapeObj->HasMaterial()) {
                attributes->SetShadingAspect(new Prs3d_ShadingAspect());
                attributes->ShadingAspect()->SetMaterial(Graphic3d_MaterialAspect(Graphic3d_NOM_PLASTIC));
            }
            break;
        }
        case TopAbs_SHELL:
        case TopAbs_FACE:
        {
            if (!shapeObj->HasMaterial()) {
                attributes->SetShadingAspect(new Prs3d_ShadingAspect());
                attributes->ShadingAspect()->SetMaterial(Graphic3d_MaterialAspect(Graphic3d_NOM_SATIN));
            }
            break;
        }
        case TopAbs_WIRE:
        case TopAbs_EDGE:
        {
            // 线框的特殊设置
            Handle(Prs3d_LineAspect) lineAspect = new Prs3d_LineAspect(
                Quantity_NOC_RED, Aspect_TOL_SOLID, 2.0);
            attributes->SetWireAspect(lineAspect);
            attributes->SetLineAspect(lineAspect);
            break;
        }
        default:
            break;
        }
    }
    catch (const Standard_Failure& e)
    {
        std::wcerr << L"Error setting geometry specific attributes: " << e.GetMessageString() << std::endl;
    }
}

// 确保对象在 context 中显示
void ModelImport::EnsureObjectsDisplayed(Handle(AIS_InteractiveContext)& context,
    const std::vector<Handle(AIS_InteractiveObject)>& objects)
{
    if (context.IsNull())
        return;

    try
    {
        for (const auto& obj : objects)
        {
            if (!obj.IsNull() && !context->IsDisplayed(obj))
            {
                context->Display(obj, Standard_False);
            }
        }
        context->UpdateCurrentViewer();

        std::wcout << L"Ensured " << objects.size() << L" objects are displayed in context" << std::endl;
    }
    catch (const Standard_Failure& e)
    {
        std::wcerr << L"Error ensuring objects displayed: " << e.GetMessageString() << std::endl;
    }
}

// 从 context 中计算模型包围盒
void ModelImport::CalculateModelBoundsFromContext(Handle(AIS_InteractiveContext)& context)
{
    if (context.IsNull())
    {
        m_boundsCalculated = false;
        InvalidateHeightIntersector();
        return;
    }

    try
    {
        AIS_ListOfInteractive displayedObjects;
        context->DisplayedObjects(displayedObjects);

        if (displayedObjects.IsEmpty())
        {
            m_boundsCalculated = false;
            std::wcerr << L"No displayed objects found for bounds calculation" << std::endl;
            return;
        }

        // 使用现有的 CalculateModelBounds 方法
        CalculateModelBounds();

        std::wcout << L"Model bounds calculated from context objects" << std::endl;
    }
    catch (const Standard_Failure& e)
    {
        std::wcerr << L"Error calculating bounds from context: " << e.GetMessageString() << std::endl;
        m_boundsCalculated = false;
    }
}

void ModelImport::ExportViews(const Handle(V3d_Viewer)& viewer,
    const Handle(AIS_InteractiveContext)& context,
    const std::wstring& baseFileName,
    const int w, const int h,
    const ViewConfig& config)
{
    // 修复：添加空指针检查，避免 Handle 赋值错误
    if (viewer.IsNull()) {
        throw std::runtime_error("Viewer is null in ExportViews");
    }

    if (context.IsNull()) {
        throw std::runtime_error("Context is null in ExportViews");
    }

    // 修复：安全的视图获取和赋值
    Handle(V3d_View) view;


    // 检查已定义的视图
    V3d_ListOfView definedViews = viewer->DefinedViews();
    if (!definedViews.IsEmpty()) {
        view = definedViews.First();  // 修复：使用 First() 而不是直接索引
    }
    else {
        // 创建新视图
        view = viewer->CreateView();
        if (view.IsNull()) {
            throw std::runtime_error("Failed to create V3d_View");
        }
    }

    // 修复：在使用 view 前再次检查
    if (view.IsNull()) {
        throw std::runtime_error("Failed to obtain valid V3d_View");
    }

    // 保存当前视图设置
    Standard_Integer originalWidth = 0, originalHeight = 0;
    if (!view->Window().IsNull()) {  // 修复：检查 Window 是否为空
        view->Window()->Size(originalWidth, originalHeight);
    }

    // 配置渲染参数
    if (!m_preserveExternalViewState) {
        ConfigureRendering(view, config);
    }

    // 使用 FBO 生成所有视图
    GenerateAllViewsWithFBO(view, baseFileName, w, h, config);

    // 恢复原始视图大小（如果需要）
    if (!view->Window().IsNull()) {  // 修复：检查 Window 是否为空
        view->MustBeResized();
    }
}

void ModelImport::GenerateAllViewsWithFBO(Handle(V3d_View)& view,
    const std::wstring& baseFileName,
    const int w, const int h,
    const ViewConfig& config)
{
    if (view.IsNull()) {
        throw std::runtime_error("View is null in GenerateAllViewsWithFBO");
    }

    struct ViewDirection {
        std::wstring name;
        gp_Dir direction;
        gp_Dir up;
    };

    std::vector<ViewDirection> viewDirections = {
        {L"front", gp_Dir(0, -1, 0), gp_Dir(0, 0, 1)},
        {L"left",  gp_Dir(-1, 0, 0), gp_Dir(0, 0, 1)},
        {L"top",   gp_Dir(0, 0, -1), gp_Dir(0, 1, 0)},
        {L"iso",   gp_Dir(1, -1, 1), gp_Dir(0, 0, 1)}
    };

    gp_Pnt originalEye;
    gp_Dir originalDirection;
    gp_Dir originalUp;
    Standard_Real originalScale = 1.0;

    try {
        Handle(Graphic3d_Camera) camera = view->Camera();
        if (!camera.IsNull()) {
            originalEye = camera->Eye();
            originalDirection = camera->Direction();
            originalUp = camera->Up();
            originalScale = camera->Scale();
        }
    }
    catch (...) {
        originalEye = gp_Pnt(0, 0, 100);
        originalDirection = gp_Dir(0, 0, -1);
        originalUp = gp_Dir(0, 1, 0);
        originalScale = 1.0;
    }

    m_imageDataBuffers.clear();
    m_imageDataBuffers.reserve(viewDirections.size());

    for (const auto& viewDir : viewDirections) {
        try {
            if (!m_preserveExternalViewState) {
                SetViewDirection(view, viewDir.direction, viewDir.up, m_context);
            }
            else {
                if (m_scaleCalibrated) {
                    ApplyScaleCalibration(view, w, h, /*keepCenter=*/true);
                }
            }
            if (!m_scaleCalibrated && !m_preserveExternalViewState) {
                view->FitAll();
            }
            Image_AlienPixMap pixmap;
            if (view->ToPixMap(pixmap, w, h)) {
                const size_t bytesPerPixel = 3;
                const size_t imageSize = (size_t)w * (size_t)h * bytesPerPixel;
                const Standard_Byte* srcData = pixmap.Data();

                m_imageDataBuffers.emplace_back(imageSize);
                auto& currentBuffer = m_imageDataBuffers.back();

                // 拷贝 RGB -> BGR
                for (size_t i = 0; i < (size_t)w * (size_t)h; ++i) {
                    size_t o = i * bytesPerPixel;
                    currentBuffer[o + 0] = srcData[o + 2];
                    currentBuffer[o + 1] = srcData[o + 1];
                    currentBuffer[o + 2] = srcData[o + 0];
                }

                // === DOF ===
                if (m_enableDOF) {
                    std::vector<float> depth;
                    bool td = false;
                    if (GrabDepth(view, w, h, depth, td)) {
                        PostProcessDOF_Basic(currentBuffer, depth, w, h, view);
                    }
                }

                if (m_generateImages && !baseFileName.empty()) {
                    std::wstring fileName = baseFileName + L"_" + viewDir.name + L".png";
                    TCollection_AsciiString asciiFileName(fileName.c_str());
                    pixmap.Save(asciiFileName);
                }
            }
            else {
                std::wcerr << L"Failed to render view: " << viewDir.name << std::endl;
                m_imageDataBuffers.emplace_back(); // 占位
            }
        }
        catch (const std::exception& e) {
            std::cerr << "Error rendering view " << std::string(viewDir.name.begin(), viewDir.name.end())
                << ": " << e.what() << std::endl;
            m_imageDataBuffers.emplace_back();
        }
    }

    try {
        Handle(Graphic3d_Camera) camera = view->Camera();
        if (!camera.IsNull()) {
            camera->SetEye(originalEye);
            camera->SetDirection(originalDirection);
            camera->SetUp(originalUp);
            camera->SetScale(originalScale);
            view->Redraw();
        }
    }
    catch (...) {
        std::wcerr << L"Warning: Failed to restore original camera settings" << std::endl;
    }
}

void ModelImport::SetViewDirection(Handle(V3d_View)& view,
    const gp_Dir& direction,
    const gp_Dir& up,
    Handle(AIS_InteractiveContext)& context)
{
    if (view.IsNull()) {
        throw std::runtime_error("View is null in SetViewDirection");
    }

    if (context.IsNull()) {
        throw std::runtime_error("Context is null in SetViewDirection");
    }

    Bnd_Box boundingBox;
    Standard_Integer objectCount = 0;

    AIS_ListOfInteractive displayedObjects;
    context->DisplayedObjects(displayedObjects);

    for (AIS_ListIteratorOfListOfInteractive it(displayedObjects); it.More(); it.Next()) {
        Handle(AIS_InteractiveObject) obj = it.Value();
        Handle(AIS_Shape) shapeObj = Handle(AIS_Shape)::DownCast(obj);

        if (!shapeObj.IsNull()) {
            BRepBndLib::Add(shapeObj->Shape(), boundingBox);
            objectCount++;
        }
    }

    if (boundingBox.IsVoid() || objectCount == 0) {
        gp_Pnt defaultCenter(0, 0, 0);
        Standard_Real defaultDistance = 100.0;
        gp_Pnt eye = defaultCenter.Translated(gp_Vec(direction.Reversed().XYZ() * defaultDistance));

        Handle(Graphic3d_Camera) camera = view->Camera();
        if (!camera.IsNull()) {
            camera->SetEye(eye);
            camera->SetCenter(defaultCenter);
            camera->SetUp(up);
            camera->SetDirection(direction);
        }
        return;
    }

    Standard_Real xmin, ymin, zmin;
    Standard_Real xmax, ymax, zmax;
    boundingBox.Get(xmin, ymin, zmin, xmax, ymax, zmax);

    gp_Pnt center((xmin + xmax) / 2.0, (ymin + ymax) / 2.0, (zmin + zmax) / 2.0);

    Standard_Real sizeX = xmax - xmin;
    Standard_Real sizeY = ymax - ymin;
    Standard_Real sizeZ = zmax - zmin;

    Standard_Real maxSize = sizeX > sizeY ? (sizeX > sizeZ ? sizeX : sizeZ) : (sizeY > sizeZ ? sizeY : sizeZ);

    Standard_Real distance = maxSize * 2.0;
    gp_Pnt eye = center.Translated(gp_Vec(direction.Reversed().XYZ() * distance));

    Handle(Graphic3d_Camera) camera = view->Camera();
    if (!camera.IsNull()) {
        camera->SetEye(eye);
        camera->SetCenter(center);
        camera->SetUp(up);
        camera->SetDirection(direction);
    }
}



bool ModelImport::TurnToScalePara(int zoom_index, const TScalePara& scale_para, double DOFmm)
{
    m_zoomIndex = zoom_index;
    m_scalePara = scale_para;
    m_DOFmm = DOFmm;
    m_scaleCalibrated = (scale_para.calibrated_flag && scale_para.x_scale > 0.0 && scale_para.y_scale > 0.0);

    SetDofParams(DOFmm, (m_maxDofBlurPx > 0 ? m_maxDofBlurPx : 8));
    return true;
}

void ModelImport::ApplyScaleCalibration(const Handle(V3d_View)& view, int w, int h, bool keepCenter)
{
    if (view.IsNull() || !m_scaleCalibrated) return;
    Handle(Graphic3d_Camera) cam = view->Camera();
    if (cam.IsNull()) return;
    cam->SetProjectionType(Graphic3d_Camera::Projection_Orthographic);
    const double mmPerPxY = m_scalePara.y_scale * 2;
    const double mmPerPxX = m_scalePara.x_scale * 2;
    const double WUperPxY = mmPerPxY / (std::max)(1e-12, m_modelUnitToMM);
    const double WUperPxX = mmPerPxX / (std::max)(1e-12, m_modelUnitToMM);

    cam->SetScale(0.5 * h * WUperPxY);
    cam->SetAspect((std::max)(1e-9, (w * WUperPxX) / (h * WUperPxY)));

    if (!keepCenter) {
        if (m_viewConfig.useCustomFocus) cam->SetCenter(m_viewConfig.focusPoint);
        else if (m_boundsCalculated)     cam->SetCenter(m_modelCenter);
    }

    if (std::abs(m_scalePara.angle) > 1e-9) {
        const double ang = m_scalePara.angle * M_PI / 180.0;
        gp_Ax1 rollAxis(cam->Center(), cam->Direction());
        cam->SetUp(cam->Up().Rotated(rollAxis, ang));
    }

    view->MustBeResized();
    view->Redraw();
}

static void DrawCenterCross(unsigned char* buf,
    int width,
    int height,
    int halfSize = 10,
    int thickness = 1)
{
    if (!buf || width <= 0 || height <= 0) return;

    const int cx = width / 2;
    const int cy = height / 2;

    auto putPixel = [&](int x, int y)
    {
        if (x < 0 || x >= width || y < 0 || y >= height) return;

        const int idx = (y * width + x) * 3;
        buf[idx + 0] = 0; // B
        buf[idx + 1] = 0; // G
        buf[idx + 2] = 255; // R
    };

    for (int dy = -thickness / 2; dy <= thickness / 2; ++dy)
    {
        int y = cy + dy;
        for (int dx = -halfSize; dx <= halfSize; ++dx)
        {
            putPixel(cx + dx, y);
        }
    }

    for (int dx = -thickness / 2; dx <= thickness / 2; ++dx)
    {
        int x = cx + dx;
        for (int dy = -halfSize; dy <= halfSize; ++dy)
        {
            putPixel(x, cy + dy);
        }
    }
}


bool ModelImport::SnapshotFromCurrentView(int w, int h)
{
    if (m_viewer.IsNull() || m_context.IsNull())
        return false;

    bool oldPreserve = m_preserveExternalViewState;
    m_preserveExternalViewState = true;

    try
    {
        ExportCurrentView(m_viewer, m_context, L"", w, h, m_viewConfig);

        if (!m_imageDataBuffers.empty())
        {
            std::vector<unsigned char>& buf = m_imageDataBuffers.back();
            const size_t needSize = static_cast<size_t>(w) * static_cast<size_t>(h) * 3;
            if (buf.size() >= needSize)
            {
                DrawCenterCross(buf.data(), w, h, 10, 1);
            }
        }

        m_preserveExternalViewState = oldPreserve;
        return true;
    }
    catch (...)
    {
        m_preserveExternalViewState = oldPreserve;
        return false;
    }
}



bool ModelImport::GrabDepth(const Handle(V3d_View)& view, int w, int h,
    std::vector<float>& outDepth, bool& outTopDown)
{
    outDepth.clear(); outTopDown = false;
    if (view.IsNull()) return false;

    Image_PixMap depthPx;

#if OCC_VERSION_HEX >= 0x070600
    if (!view->ToPixMap(depthPx, w, h, Graphic3d_BufferType_Depth)) return false;
#else
    if (!view->ToPixMap(depthPx, w, h, Graphic3d_BT_Depth)) return false;
#endif

    if (depthPx.Format() != Image_Format_GrayF) return false;

    const int stride = static_cast<int>(depthPx.SizeRowBytes());
    const unsigned char* base = depthPx.Data();
    outDepth.resize(w * h);
    for (int y = 0; y < h; ++y) {
        const float* srow = reinterpret_cast<const float*>(base + y * stride);
        float* drow = outDepth.data() + (h - 1 - y) * w;
        std::memcpy(drow, srow, sizeof(float) * w);
    }
    return true;
}

void ModelImport::EstimateDepthRangeAlongView(const Handle(V3d_View)& view,
    double& outNearWU, double& outFarWU)
{
    outNearWU = 0.0; outFarWU = 1.0;
    if (view.IsNull() || !m_boundsCalculated) return;

    Handle(Graphic3d_Camera) cam = view->Camera();
    if (cam.IsNull()) return;

    gp_Dir V = cam->Direction();

    const double xs[2] = { m_modelMin.X(), m_modelMax.X() };
    const double ys[2] = { m_modelMin.Y(), m_modelMax.Y() };
    const double zs[2] = { m_modelMin.Z(), m_modelMax.Z() };

    double dmin = 1e100;
    double dmax = -1e100;

    for (int ix = 0; ix < 2; ++ix)
        for (int iy = 0; iy < 2; ++iy)
            for (int iz = 0; iz < 2; ++iz) {
                gp_Pnt P(xs[ix], ys[iy], zs[iz]);
                double d = gp_Vec(cam->Eye(), P).Dot(gp_Vec(V)); 
                dmin = (std::min)(dmin, d);
                dmax = (std::max)(dmax, d);
            }
    if (dmin > dmax) std::swap(dmin, dmax);
    outNearWU = dmin;
    outFarWU = dmax;
}

void ModelImport::GaussianBlurSeparableBGR(std::vector<unsigned char>& bgr, int w, int h, int radius)
{
    radius = (std::max)(1, (std::min)(radius, 12));
    const int bpp = 3;
    std::vector<unsigned char> tmp(bgr.size());

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int sumB = 0, sumG = 0, sumR = 0, sumW = 0;
            for (int dx = -radius; dx <= radius; ++dx) {
                int xx = (std::min)(w - 1, (std::max)(0, x + dx));
                int wgt = radius + 1 - std::abs(dx);
                const unsigned char* p = &bgr[(y * w + xx) * bpp];
                sumB += p[0] * wgt; sumG += p[1] * wgt; sumR += p[2] * wgt; sumW += wgt;
            }
            unsigned char* q = &tmp[(y * w + x) * bpp];
            q[0] = (unsigned char)(sumB / sumW);
            q[1] = (unsigned char)(sumG / sumW);
            q[2] = (unsigned char)(sumR / sumW);
        }
    }

    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            int sumB = 0, sumG = 0, sumR = 0, sumW = 0;
            for (int dy = -radius; dy <= radius; ++dy) {
                int yy = (std::min)(h - 1, (std::max)(0, y + dy));
                int wgt = radius + 1 - std::abs(dy);
                const unsigned char* p = &tmp[(yy * w + x) * bpp];
                sumB += p[0] * wgt; sumG += p[1] * wgt; sumR += p[2] * wgt; sumW += wgt;
            }
            unsigned char* q = &bgr[(y * w + x) * bpp];
            q[0] = (unsigned char)(sumB / sumW);
            q[1] = (unsigned char)(sumG / sumW);
            q[2] = (unsigned char)(sumR / sumW);
        }
    }
}

void ModelImport::PostProcessDOF_Basic(std::vector<unsigned char>& bgr,
    const std::vector<float>& depth,
    int w, int h,
    const Handle(V3d_View)& view)
{
    if (!m_enableDOF || bgr.size() != (size_t)w * h * 3 || w <= 0 || h <= 0)
        return;

    if (view.IsNull())
        return;

    (void)depth;

    if (m_DOFmm <= 1e-6)
        return;

    const float tCam = ClampT<float>((float)m_camHeightFactor, 0.0f, 1.0f);
    if (tCam <= 0.0f)
    {
        return;
    }

    if (m_maxDofBlurPx <= 0.5f)
        return;

    int radius = (int)std::round(m_maxDofBlurPx * tCam);
    if (radius < 1)
        radius = 1;

    std::vector<unsigned char> blurred = bgr;
    GaussianBlurSeparableBGR(blurred, w, h, radius);

    const int pixelCount = w * h;
    const int bpp = 3;
    const float t = tCam;

    for (int i = 0; i < pixelCount; ++i)
    {
        unsigned char* dst = &bgr[i * bpp];
        const unsigned char* srcBlur = &blurred[i * bpp];

        dst[0] = (unsigned char)((1.0f - t) * dst[0] + t * srcBlur[0]); // B
        dst[1] = (unsigned char)((1.0f - t) * dst[1] + t * srcBlur[1]); // G
        dst[2] = (unsigned char)((1.0f - t) * dst[2] + t * srcBlur[2]); // R
    }
}


bool ModelImport::GetHeightAtXY(const gp_Ax2& cs, double x, double y, double& outZ)
{
    outZ = 0.0;

    if (m_context.IsNull() || !m_boundsCalculated)
        return false;

    const gp_Pnt O = cs.Location();
    const gp_Dir xdir = cs.XDirection();
    const gp_Dir ydir = cs.YDirection();
    const gp_Dir zdir = cs.Direction(); 

    gp_Pnt corners[8] = {
        m_modelMin,
        gp_Pnt(m_modelMax.X(), m_modelMin.Y(), m_modelMin.Z()),
        gp_Pnt(m_modelMin.X(), m_modelMax.Y(), m_modelMin.Z()),
        gp_Pnt(m_modelMin.X(), m_modelMin.Y(), m_modelMax.Z()),
        gp_Pnt(m_modelMax.X(), m_modelMax.Y(), m_modelMin.Z()),
        gp_Pnt(m_modelMax.X(), m_modelMin.Y(), m_modelMax.Z()),
        gp_Pnt(m_modelMin.X(), m_modelMax.Y(), m_modelMax.Z()),
        m_modelMax
    };

    double wMin = 1e100;
    double wMax = -1e100;

    for (int i = 0; i < 8; ++i)
    {
        gp_Vec v(O, corners[i]);
        double w = v.Dot(gp_Vec(zdir));
        if (w < wMin) wMin = w;
        if (w > wMax) wMax = w;
    }

    double dz = wMax - wMin;
    if (dz < 1e-6) dz = 100.0;

    double zStart = wMax + dz;
    gp_Pnt startP = O
        .Translated(gp_Vec(xdir) * x)
        .Translated(gp_Vec(ydir) * y)
        .Translated(gp_Vec(zdir) * zStart);

    gp_Dir  dirDown = -zdir;
    gp_Lin  line(startP, dirDown);

    if (!m_heightInterValid)
    {
        RebuildHeightIntersectorFromScene();
    }
    if (!m_heightInterValid)
    {
        return false;
    }

    m_heightInter.Perform(line, -Precision::Infinite(), Precision::Infinite());

    if (!m_heightInter.IsDone()) {
        std::cout << "Intersection not done." << std::endl;
        return false;
    }

    Standard_Integer nPts = m_heightInter.NbPnt();
    if (nPts <= 0) {
        std::cout << "No intersection points found." << std::endl;
        return false;
    }

    Standard_Real bestH = -Precision::Infinite();
    Standard_Boolean hasBest = Standard_False;

    for (Standard_Integer i = 1; i <= nPts; ++i)
    {
        gp_Pnt P = m_heightInter.Pnt(i);

        gp_Vec vSP(startP, P);
        Standard_Real t = vSP.Dot(gp_Vec(dirDown));
        if (t < 0.0)
        {
            continue;
        }

        gp_Vec vOP(O, P);
        Standard_Real h = vOP.Dot(gp_Vec(zdir));

        if (!hasBest || h > bestH)
        {
            bestH = h;
            hasBest = Standard_True;
        }
    }

    if (!hasBest)
        return false;

    outZ = bestH;
    return true;
}

void ModelImport::InvalidateHeightIntersector()
{
    m_heightShape.Nullify();
    m_heightInterValid = false;
}

void ModelImport::RebuildHeightIntersectorFromScene()
{
    InvalidateHeightIntersector();

    if (m_context.IsNull())
        return;

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);

    bool hasShape = false;

    if (!m_currentDoc.IsNull())
    {
        Handle(XCAFDoc_ShapeTool) shapeTool =
            XCAFDoc_DocumentTool::ShapeTool(m_currentDoc->Main());

        TDF_LabelSequence freeShapes;
        shapeTool->GetFreeShapes(freeShapes);

        for (Standard_Integer i = 1; i <= freeShapes.Length(); ++i)
        {
            TopoDS_Shape s = shapeTool->GetShape(freeShapes.Value(i));
            if (!s.IsNull())
            {
                builder.Add(comp, s);
                hasShape = true;
            }
        }
    }

    AIS_ListOfInteractive aObjects;
    m_context->ObjectsInside(aObjects, AIS_KindOfInteractive_None, -1);

    for (AIS_ListIteratorOfListOfInteractive it(aObjects); it.More(); it.Next())
    {
        Handle(AIS_InteractiveObject) obj = it.Value();
        Handle(AIS_Shape) shapeObj = Handle(AIS_Shape)::DownCast(obj);
        if (shapeObj.IsNull()) continue;

        TopoDS_Shape s = shapeObj->Shape();
        if (!s.IsNull())
        {
            builder.Add(comp, s);
            hasShape = true;
        }
    }

    if (!hasShape)
        return; 

    m_heightShape = comp;

    m_heightInter.Load(m_heightShape, Precision::Confusion());
    m_heightInterValid = true;
}


void ModelImport::SetCameraHeightDiffMM(double diffMM)
{
    if (m_DOFmm <= 1e-9) { m_camHeightFactor = 0.0; return; }

    double diff = std::fabs(diffMM);
    if (diff <= m_DOFmm) { 
        m_camHeightFactor = 0.0;
        return;
    }
    double r = (diff - m_DOFmm) / m_DOFmm;
    if (r > 1.0) r = 1.0;
    m_camHeightFactor = r;
}

void ModelImport::UpdateDOFHeightFromView(const Handle(V3d_View)& view,
    const gp_Ax2& cs)
{
    m_camHeightFactor = 0.0;

    if (!m_enableDOF || m_DOFmm <= 1e-9 || !m_boundsCalculated)
        return;

    gp_Pnt eye;
    gp_Pnt dummyCenter;
    gp_Dir dummyUp;

    bool hasVirtCam = GetVirtualCamera(eye, dummyCenter, dummyUp);
    if (!hasVirtCam)
    {
        if (view.IsNull())
            return;

        Handle(Graphic3d_Camera) cam = view->Camera();
        if (cam.IsNull())
            return;

        eye = cam->Eye();
    }

    const gp_Pnt O = cs.Location();
    const gp_Dir xdir = cs.XDirection();
    const gp_Dir ydir = cs.YDirection();
    const gp_Dir zdir = cs.Direction();

    gp_Vec vOE(O, eye);
    const double camX = vOE.Dot(gp_Vec(xdir));
    const double camY = vOE.Dot(gp_Vec(ydir));
    const double camZ = vOE.Dot(gp_Vec(zdir));

    double objZ = 0.0;
    if (!GetHeightAtXY(cs, camX, camY, objZ))
    {
        m_camHeightFactor = 0.0;
        return;
    }

    double diffWU = camZ - objZ;
    double diffAbsWU = std::fabs(diffWU);

    double unitToMM = (m_modelUnitToMM > 1e-12) ? m_modelUnitToMM : 1.0;
    double diffMM = diffAbsWU * unitToMM;

    SetCameraHeightDiffMM(diffMM);
}
