#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <unordered_map>
#include <locale>
#include <codecvt>
#include <list>
#include <cmath>
#include <memory>
#include <cstring>
#include <sys/stat.h>

#define OCCT_NO_DEPRECATED

// Windows平台相关
#ifdef _WIN32
#include <windows.h>
#include <WNT_Window.hxx>
#include <WNT_WClass.hxx>
#endif // _WIN32

// AIS (Application Interactive Services) - 交互显示服务
#include <AIS_InteractiveContext.hxx>
#include <AIS_Shape.hxx>
#include <AIS_ColoredShape.hxx>

// TopoDS - 拓扑数据结构
#include <TopoDS_Shape.hxx>
#include <TopoDS_Face.hxx>

// XCAF Presentation - 扩展CAF表示层
#include <XCAFPrs_AISObject.hxx>
#include <XCAFPrs_Driver.hxx>
#include <TPrsStd_AISPresentation.hxx>

// 拓扑探索和几何基础
#include <TopExp_Explorer.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>
#include <Bnd_Box.hxx>
#include <BRepBndLib.hxx>

// 3D视图和图形渲染
#include <Aspect_DisplayConnection.hxx>
#include <OpenGl_GraphicDriver.hxx>
#include <V3d_Viewer.hxx>
#include <V3d_View.hxx>
#include <Graphic3d_CLight.hxx>
#include <Graphic3d_MaterialAspect.hxx>
#include <Quantity_Color.hxx>
#include <Prs3d_PointAspect.hxx>

// XCAF文档工具 - 扩展CAF文档管理
#include <XCAFApp_Application.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFDoc_ColorTool.hxx>
#include <XCAFDoc_VisMaterialTool.hxx>
#include <XCAFDoc_VisMaterial.hxx>
#include <TDF_ChildIterator.hxx>

// CAD文件导入
#include <STEPCAFControl_Reader.hxx>
#include <IGESCAFControl_Reader.hxx>

// 图像处理
#include <Image_AlienPixMap.hxx>
#include <Image_PixMap.hxx>

// 用于顶点和形状映射
#include <TopTools_IndexedMapOfShape.hxx>
#include <TopExp.hxx>

// 用于边和曲线处理
#include <TopoDS_Edge.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Curve.hxx>

// 用于标准类型
#include <Standard_Real.hxx>

// 其他可能需要的（如果还没包含）
#include <TopoDS_Vertex.hxx>
#include <TopoDS.hxx>
#include <Interface_Static.hxx>

#include <StlAPI_Reader.hxx>
#include <AIS_PointCloud.hxx>
#include <Graphic3d_ArrayOfPoints.hxx>
#include <Bnd_Box.hxx>

#include <TopoDS_Shape.hxx>
#include <IntCurvesFace_ShapeIntersector.hxx>

template <typename T>
static inline T ClampT(T v, T lo, T hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

// 视图配置结构体
struct ViewConfig
{
    double focalLength;     // 焦距 (mm)
    double aspectRatio;     // 宽高比
    bool useCustomFocus;    // 是否使用自定义焦点
    gp_Pnt focusPoint;      // 自定义焦点位置
    double viewScale;       // 视图缩放比例 (0.1 - 10.0)

    ViewConfig()
        : focalLength(50.0)
        , aspectRatio(1.0)
        , useCustomFocus(false)
        , focusPoint(0, 0, 0)
        , viewScale(1.0)
    {
    }
};

struct TScalePara
{
    double x_scale = 0.0;       // X 轴 像素当量: mm/px
    double y_scale = 0.0;       // Y 轴 像素当量: mm/px
    double angle = 0.0;       // 图像平面滚转角（度，逆时针为正）
    bool   calibrated_flag = false; // true=启用标定
    std::wstring magnification; // 放大倍率文字
};


class ModelImport
{
public:
    //! 默认构造函数
    //! 初始化成员变量和状态
    ModelImport();

    //! 析构函数
    //! 清理缓存、释放OpenCASCADE资源和图像数据缓冲区
    ~ModelImport();

    // true=保留外部状态；false=依旧设置
    bool m_preserveExternalViewState = true;
    void SetPreserveExternalViewState(bool on) { m_preserveExternalViewState = on; }
    bool GetPreserveExternalViewState() const { return m_preserveExternalViewState; }

    // 主要接口 - 支持焦距控制
    //! 导入3D模型并生成多视图图像的主要接口
    //! @param filePath 模型文件路径（支持STEP和IGES格式）
    //! @param w 输出图像宽度（像素）
    //! @param h 输出图像高度（像素）
    //! @param generateImages 是否生成图像文件到磁盘
    //! @param focalLength 虚拟相机焦距（毫米），影响视图缩放效果
    //! @return 成功返回true，失败返回false
    bool ImportModelAndExportViews(const std::wstring& filePath,
        const int w = 640,
        const int h = 480,
        bool generateImages = false,
        double focalLength = 50.0);

    // 主函数，支持焦距控制 - 使用外部viewer对象
    bool ImportExportViews(Handle(V3d_Viewer)& viewer,
        Handle(AIS_InteractiveContext)& context,
        const int w = 640,
        const int h = 480,
        bool generateImages = false,
        double focalLength = 50.0);

    void RenderOneViewToBuffer(const Handle(V3d_View)& view, const std::wstring& baseFileName, const int w, const int h, const ViewConfig& config);

    Handle(V3d_View) FindMainView(const Handle(V3d_Viewer)& viewer);

    Handle(V3d_View) GetOrCreateImageView(int w, int h);

    Handle(V3d_Viewer) GetViewer() const { return m_viewer; };

	Handle(AIS_InteractiveContext) GetContext() const { return m_context; };

    void ClearOffscreenExcludes();
    void AddOffscreenExclude(const Handle(AIS_InteractiveObject)& obj);
    void SetOffscreenExcludes(const std::vector<Handle(AIS_InteractiveObject)>& objs);

    // 获取缓存的图像数据
    //! 获取所有生成视图的图像数据（内存中的BGR格式）
    //! @return 包含4个视图图像数据的vector：前视图、顶视图、右视图、斜视图
    const std::vector<std::vector<unsigned char>>& GetAllViewData() const;

    //! 获取指定索引的视图图像数据
    //! @param index 视图索引（0:前视图, 1:顶视图, 2:右视图, 3:斜视图）
    //! @return 指向图像数据的指针，索引超出范围则返回nullptr
    const std::vector<unsigned char>* GetViewData(size_t index) const;

    // 缓存管理
    //! 清理所有已加载模型的缓存，释放内存和OpenCASCADE对象
    void ClearModelCache();

    //! 设置最大缓存模型数量，超出时将移除最久未使用的模型
    //! @param maxCache 最大缓存数量
    void SetMaxCache(size_t maxCache);

    //! 获取当前设置的最大缓存数量
    //! @return 最大缓存数量
    size_t GetMaxCache() const;

    //! 获取当前缓存中的模型数量
    //! @return 当前缓存大小
    size_t GetCurrentCacheSize() const;

    // 视图配置方法
    //! 设置视图缩放比例
    //! @param scale 缩放比例（有效范围：0.1 - 10.0）
    void SetViewScale(double scale);

    //! 设置自定义焦点位置，视图将围绕此点生成
    //! @param point 3D空间中的焦点坐标
    void SetCustomFocusPoint(const gp_Pnt& point);

    //! 重置为自动焦点模式，使用模型几何中心作为焦点
    void ResetToAutoFocus();

    // 获取模型包围盒信息
    //! 获取已加载模型的边界框信息
    //! @param minPoint 输出参数：模型最小坐标点
    //! @param maxPoint 输出参数：模型最大坐标点
    //! @param center 输出参数：模型几何中心点
    //! @return 成功返回true，模型未加载或边界框未计算返回false
    bool GetModelBounds(gp_Pnt& minPoint, gp_Pnt& maxPoint, gp_Pnt& center) const;

    //! 设置生成图像的宽度和高度
//! @param width 图像宽度（像素），有效范围：100-7680
//! @param height 图像高度（像素），有效范围：100-4320
//! @return 设置成功返回true，参数无效返回false
    bool SetImageSize(int width, int height);

    //! 获取当前设置的图像尺寸
    //! @param width 输出参数：当前图像宽度
    //! @param height 输出参数：当前图像高度
    void GetImageSize(int& width, int& height) const;

    // 新增接口 - 焦距控制
    //! 设置虚拟相机焦距
    //! @param focalLength 焦距（毫米），有效范围：0.1-500.0
    //! @return 设置成功返回true，参数无效返回false
    bool SetFocalLength(double focalLength);

    //! 获取当前设置的焦距
    //! @return 当前焦距值（毫米）
    double GetFocalLength() const;

    std::vector<Handle(AIS_InteractiveObject)> ParseObjectsFromContext(Handle(AIS_InteractiveContext)& context);

    bool TurnToScalePara(int zoom_index, const TScalePara& scale_para, double DOFmm);
    void ApplyScaleCalibration(const Handle(V3d_View)& view, int w, int h, bool keepCenter = true);

    void ExportCurrentView(const Handle(V3d_Viewer)& viewer,
        const Handle(AIS_InteractiveContext)& context,
        const std::wstring& baseFileName,
        const int w, const int h,
        const ViewConfig& config);
    bool SnapshotFromCurrentView(int w, int h);
    void EnableDOF(bool on)
    {
        m_enableDOF = on;
    }

    void SetDofParams(double dof_mm, int maxBlurPx)
    {
        m_focusOffsetMM = 2.0;
        m_DOFmm = (std::max)(0.0, dof_mm);
        m_maxDofBlurPx = ClampT(maxBlurPx, 1, 12);
        m_enableDOF = (m_DOFmm > 0.0 && m_maxDofBlurPx > 0);
    }
    Handle(AIS_InteractiveObject) ModelImport::LoadPCD_AsPointCloud(const std::wstring& wfile);

    double m_camHeightFactor;

    bool GetHeightAtXY(const gp_Ax2& cs, double x, double y, double& outZ);

    void SetCameraHeightDiffMM(double diffMM);

    void UpdateDOFHeightFromView(const Handle(V3d_View)& view, const gp_Ax2& cs);

    double GetDOFmm() const { return m_DOFmm; }
    double GetModelUnitToMM() const { return m_modelUnitToMM; }
private:
    std::vector<Handle(AIS_InteractiveObject)> m_offscreenExcludes;
/// <summary>
/// ///////////////加一个虚拟相机
/// </summary>

public:
    bool m_ownsViewer = false;
    void SetVirtualCamera(const gp_Pnt& eye,
        const gp_Pnt& center,
        const gp_Dir& up)
    {
        m_hasVirtualCamera = true;
        m_camEye = eye;
        m_camCenter = center;
        m_camUp = up;
    }

    bool HasVirtualCamera() const { return m_hasVirtualCamera; }

    Handle(V3d_Viewer)             m_ImageViewer;
    Handle(V3d_View)               m_ImageView;
    Handle(AIS_InteractiveContext) m_ImageContext;

    bool GetVirtualCamera(gp_Pnt& eye,
        gp_Pnt& center,
        gp_Dir& up) const
    {
        if (!m_hasVirtualCamera)
            return false;
        eye = m_camEye;
        center = m_camCenter;
        up = m_camUp;
        return true;
    }

private:
    bool  m_hasVirtualCamera = false;
    gp_Pnt m_camEye;
    gp_Pnt m_camCenter;
    gp_Dir m_camUp;

private:
    TScalePara m_scalePara;
    int        m_zoomIndex = -1;
    double     m_focusOffsetMM = 2.0;
    bool       m_enableDOF = false; 
    double     m_DOFmm = 0.0;
    double     m_maxDofBlurPx = 8.0;
    bool       m_scaleCalibrated = false;
    double     m_modelUnitToMM = 1.0;
    bool GrabDepth(const Handle(V3d_View)& view, int w, int h,
        std::vector<float>& outDepth, bool& outTopDown);
    void EstimateDepthRangeAlongView(const Handle(V3d_View)& view,
        double& outNearWU, double& outFarWU);
    static void GaussianBlurSeparableBGR(std::vector<unsigned char>& bgr, int w, int h, int radius);
    void PostProcessDOF_Basic(std::vector<unsigned char>& bgr,
        const std::vector<float>& depth, int w, int h,
        const Handle(V3d_View)& view);

    TopoDS_Shape                     m_heightShape;    // 场景几何的 compound
    IntCurvesFace_ShapeIntersector   m_heightInter;
    bool                             m_heightInterValid;

    void InvalidateHeightIntersector();
    void RebuildHeightIntersectorFromScene();


    // 模型加载方法
    //! 使用缓存机制加载模型，优先从缓存中获取已加载的模型
    //! @param filePath 模型文件路径
    //! @return 加载的AIS交互对象列表
    std::vector<Handle(AIS_InteractiveObject)> LoadModelWithCache(const std::wstring& filePath);

    //! 从文件直接加载模型，支持STEP和IGES格式
    //! @param filePath 模型文件路径
    //! @param fileType 文件类型扩展名（step/stp/iges/igs）
    //! @return 加载的AIS交互对象列表
    std::vector<Handle(AIS_InteractiveObject)> LoadModelFromFile(const std::wstring& filePath,
        const std::wstring& fileType);

    // 显示方法
    //! 在AIS上下文中显示模型对象，使用阴影着色模式
    //! @param objects 要显示的AIS交互对象列表
    void DisplayShapesWithShadedMode(const std::vector<Handle(AIS_InteractiveObject)>& objects);

    // 视图导出方法
    //! 创建离屏窗口并导出多个视图到内存和文件
    //! @param viewer 3D查看器
    //! @param context AIS交互上下文
    //! @param baseFileName 基础文件名
    //! @param w 图像宽度
    //! @param h 图像高度
    //! @param config 视图配置参数
    void ExportViewsToMemoryAndFile(const Handle(V3d_Viewer)& viewer,
        const Handle(AIS_InteractiveContext)& context,
        const std::wstring& baseFileName,
        const int w, const int h,
        const ViewConfig& config);

    //! 生成所有标准视图（前、顶、右、斜）的图像
    //! @param view 3D视图对象
    //! @param baseFileName 基础文件名
    //! @param windowSize 图像尺寸
    //! @param config 视图配置参数
    void GenerateAllViews(const Handle(V3d_View)& view,
        const std::wstring& baseFileName,
        const Graphic3d_Vec2i& windowSize,
        const ViewConfig& config);

    // 视图设置方法
    //! 设置单个视图的方向、缩放和中心点
    //! @param view 3D视图对象
    //! @param orientation 视图方向枚举
    //! @param config 视图配置参数
    //! @param scale 缩放比例
    //! @param center 视图中心点
    void SetupView(const Handle(V3d_View)& view,
        V3d_TypeOfOrientation orientation,
        const ViewConfig& config,
        double scale,
        const gp_Pnt& center);
    
    //! 计算统一的视图参数，确保所有视图具有一致的缩放和中心
    //! @param view 3D视图对象
    //! @param config 视图配置参数
    //! @param outScale 输出参数：计算的统一缩放值
    //! @param outCenter 输出参数：计算的统一中心点
    void SetupUnifiedViewParameters(const Handle(V3d_View)& view,
        const ViewConfig& config,
        double& outScale,
        gp_Pnt& outCenter);

    // 计算方法
    //! 计算当前显示模型的边界框、中心点和直径
    void CalculateModelBounds();

    // 初始化方法
    //! 初始化OpenCASCADE显示连接
    //! @return 成功返回true，失败返回false
    bool InitializeDisplay();

    //! 初始化3D查看器和图形驱动程序
    //! @return 成功返回true，失败返回false
    bool InitializeViewer();

    //! 初始化AIS交互上下文
    //! @return 成功返回true，失败返回false
    bool InitializeContext();

    // 光照设置
    //! 设置场景光照，包括环境光和多个方向光源
    void SetupLights();

    // 渲染配置
    //! 配置视图渲染参数，包括抗锯齿、阴影等效果
    //! @param view 3D视图对象
    //! @param config 视图配置参数
    void ConfigureRendering(const Handle(V3d_View)& view, const ViewConfig& config);

    // 缓存维护
    //! 维护缓存大小，移除超出限制的最久未使用模型
    void MaintainCacheSize();

    //! 更新模型在缓存中的访问顺序
    //! @param filePath 模型文件路径
    void UpdateCacheOrder(const std::wstring& filePath);

    // 工具方法
    //! 将宽字符串转换为UTF-8编码字符串
    //! @param wstr 宽字符串
    //! @return UTF-8编码的字符串
    std::string ConvertWStringToUtf8(const std::wstring& wstr);

    //! 获取文件扩展名并转换为小写
    //! @param filename 文件名
    //! @return 小写的扩展名
    std::wstring GetFileExtensionLower(const std::wstring& filename);

    //! 检查文件是否存在且可访问
    //! @param filePath 文件路径
    //! @return 文件存在返回true，否则返回false
    bool FileExists(const std::wstring& filePath);

    //! 验证图像分辨率是否在有效范围内
    //! @param width 图像宽度
    //! @param height 图像高度
    //! @return 分辨率有效返回true，否则返回false
    bool IsValidResolution(int width, int height);

    //! 根据分辨率判断是否应使用高质量渲染模式
    //! @param width 图像宽度
    //! @param height 图像高度
    //! @return 应使用高质量模式返回true，否则返回false
    bool ShouldUseHighQuality(int width, int height) const;

    std::vector<Handle(AIS_InteractiveObject)> ParseSingleAISObject(Handle(AIS_InteractiveObject)& obj);
    void ParseAndCreateEnhancedObjects(const TopoDS_Shape& shape, Handle(AIS_Shape) originalObj, std::vector<Handle(AIS_InteractiveObject)>& results);
    Handle(AIS_Shape) CreateEnhancedShapeWithGeometryInfo(const TopoDS_Shape& shape, Handle(AIS_Shape) originalObj);
    Handle(AIS_ColoredShape) CreateColoredShapeObject(const TopoDS_Shape& shape, Handle(AIS_ColoredShape) originalObj);
    void CopyBasicAttributes(Handle(AIS_Shape) sourceObj, Handle(AIS_Shape) targetObj);
    void CopyColoredShapeAttributes(Handle(AIS_ColoredShape) sourceObj, Handle(AIS_ColoredShape) targetObj);
    void SetGeometrySpecificAttributes(Handle(AIS_Shape) shapeObj, TopAbs_ShapeEnum shapeType);
    void EnsureObjectsDisplayed(Handle(AIS_InteractiveContext)& context, const std::vector<Handle(AIS_InteractiveObject)>& objects);
    void CalculateModelBoundsFromContext(Handle(AIS_InteractiveContext)& context);

    void ExportViews(const Handle(V3d_Viewer)& viewer, const Handle(AIS_InteractiveContext)& context, const std::wstring& baseFileName, const int w, const int h, const ViewConfig& config);
    void StoreImageInMemory(const Image_AlienPixMap& pixmap, const std::wstring& viewName);
    void GenerateAllViewsWithFBO(Handle(V3d_View)& view, const std::wstring& baseFileName, const int w, const int h, const ViewConfig& config);
    void SetViewDirection(Handle(V3d_View)& view, const gp_Dir& direction, const gp_Dir& up, Handle(AIS_InteractiveContext) &context);

private:
    // 核心对象
    Handle(Aspect_DisplayConnection) m_displayConnection;   //!< OpenCASCADE显示连接
    Handle(V3d_Viewer) m_viewer;                            //!< 3D查看器
    Handle(AIS_InteractiveContext) m_context;               //!< AIS交互上下文
    Handle(TDocStd_Document) m_currentDoc;                  //!< 当前XCAF文档

    // 缓存管理
    size_t m_maxCache = 10;                                                     //!< 最大缓存模型数量
    std::list<std::wstring> m_cacheOrder;                                       //!< 缓存访问顺序（LRU）
    std::unordered_map<std::wstring, std::vector<Handle(AIS_InteractiveObject)>> m_loadedModelsCache; //!< 模型缓存映射

    // 图像数据
    std::vector<std::vector<unsigned char>> m_imageDataBuffers;                 //!< 生成的图像数据缓冲区

    // 文件路径
    std::wstring m_filePath;                                                    //!< 当前加载的文件路径

    // 标志位
    bool m_generateImages = false;           //!< 是否生成图像文件
    bool m_usePerformanceMode = false;      //!< 是否使用性能优先模式
    int m_lastImageWidth = 0;               //!< 上次生成图像的宽度
    int m_lastImageHeight = 0;              //!< 上次生成图像的高度

    // 模型几何信息
    bool m_boundsCalculated = false;        //!< 边界框是否已计算
    gp_Pnt m_modelMin;                      //!< 模型最小坐标点
    gp_Pnt m_modelMax;                      //!< 模型最大坐标点
    gp_Pnt m_modelCenter;                   //!< 模型几何中心点
    double m_modelDiameter = 0.0;           //!< 模型对角线长度
    
    // 图片信息
    int m_imageWidth = 640;                 //!< 图像宽度（像素）
    int m_imageHeight = 480;                //!< 图像高度（像素）
    double m_focalLength = 50.0;            //!< 虚拟相机焦距（毫米）
    
    // 视图配置
    ViewConfig m_viewConfig;

    //!< 当前视图配置参数
};

// TODO: 不能用3D文件自带坐标系，要根据测量的内容去翻转3D模型
// TODO: 读写速度进一步优化，加快文件加载速度
