#pragma once
#include"SelectBaseType.h"
#include"Occ.h"
#include"RgnBox.h"
#include "CBase.h"
#include "Camera.h"
#include <GProp_GProps.hxx>
#include <afxole.h>
#include <Standard_Handle.hxx>
#include <XCAFDoc_DocumentTool.hxx>
#include <mutex>
#include <atomic>
#ifdef MYMFCDLL1_EXPORTS
#define MYMFCDLL1_API __declspec(dllexport)
#else
#define MYMFCDLL1_API __declspec(dllimport)
#endif
static UINT WM_POST_FIRST_SNAP = WM_USER + 101;
// 回调签名：24位BGR，内存由调用方释放
//typedef void(__stdcall* pfnImageCallback)(int width, int height, unsigned char* pBuffer);
// 在共享头文件中定义（同时包含在DLL和MFC项目中）
// 注意添加 const 修饰符，与DLL中传递的 const 缓冲区匹配
// 导出函数（供外部 DLL 调用）
//extern "C" __declspec(dllexport)
//typedef void (*CameraPositionCallback)(double x, double y, double z);

typedef void(*MouseButtonCallBack)(gp_Pnt);
typedef void(*AddItemCallBack)(FeatureType type, CString& str);
typedef void(*SwitchCallBack)(BOOL isOver);
typedef void(*ButtonCallBack)();
typedef void(*ClearCallBack)();
/// <param name="z">Z坐标</param>
typedef struct datamul
{
	/// <summary>
	/// 点个数
	/// </summary>
	int pointNum;
	/// <summary>
	/// 面轮廓度
	/// </summary>
	double profileOfSurface;
	/// <summary>
	/// 元素标识符
	/// </summary>
	std::string elementId;
	/// <summary>
	/// 3D模型文件
	/// </summary>
	std::string modelFilePath;
	/// <summary>
	/// 坐标点集
	/// </summary>
	std::vector<gp_Pnt> points;
}DATAMUL_T;
struct VirtualCamera {
	gp_Pnt Position;      // 相机在3D空间中的位置
	gp_Dir Direction;     // 相机朝向方向
	gp_Dir UpVector;      // 相机上方向
	double FieldOfView;   // 视场角(FOV) - 度
	double FocalLength;   // 焦距 - mm
	double SensorWidth;   // 传感器宽度 - mm
	double SensorHeight;  // 传感器高度 - mm
	int ResolutionX;      // 水平分辨率 - 像素
	int ResolutionY;      // 垂直分辨率 - 像素
	Handle(V3d_View) View;
};
#pragma pack(push, 1) 
struct BMPHeader {
	// 文件头
	uint16_t signature = 0x4D42; // "BM"
	uint32_t fileSize = 0;
	uint16_t reserved1 = 0;
	uint16_t reserved2 = 0;
	uint32_t dataOffset = 54;

	// 信息头
	uint32_t headerSize = 40;
	int32_t width = 0;
	int32_t height = 0;
	uint16_t planes = 1;
	uint16_t bitsPerPixel = 24;
	uint32_t compression = 0; // BI_RGB
	uint32_t imageSize = 0;
	int32_t xPixelsPerMeter = 0;
	int32_t yPixelsPerMeter = 0;
	uint32_t colorsUsed = 0;
	uint32_t importantColors = 0;
};
#pragma pack(pop)
struct CameraState {
	gp_Pnt position;      // 相机位置
	gp_Dir direction;     // 相机方向
	double pixelScaleX = 0.00002;   // X方向像素当量 (mm/pixel)，关键调整局部大小
	double pixelScaleY = 0.00002;   // Y方向像素当量 (mm/pixel)，关键调整局部大小
	double depthOfField = 0.5;  // 景深 (mm)
	double focalLength;
	;     // 相机在自定义坐标系中的位置
	gp_Pnt target
		;       // 目标点在自定义坐标系中的位置
	gp_Dir upDirection
		;  // 相机的上方向
};

struct CameraImageData
{
	CameraState state
		;
	std
		::vector<unsigned char> imageData;

	// 构造函数
	CameraImageData() = default;
	CameraImageData(const CameraState& camState, const std::vector<unsigned char>& imgData)
		: state(camState), imageData(imgData) {
	}
};
// 成像数据结构体
struct CameraData {
	CameraState state;
	std::vector<unsigned char> image; // 图像数据
};
class CPDFManager;
// COccView 视图
class COccView : public CView
{
	DECLARE_DYNCREATE(COccView)

public:
	COccView();           // 动态创建所使用的受保护的构造函数
	virtual ~COccView();

    void SafeReleaseIpc() noexcept;

public:
	virtual void OnDraw(CDC* pDC);      // 重写以绘制该视图
#ifdef _DEBUG
	virtual void AssertValid() const;
#ifndef _WIN32_WCE
	virtual void Dump(CDumpContext& dc) const;
#endif
#endif
    enum class EGridMode : int
    {
        Off = 0,
        XY,
        XZ,
        YZ
    };
    EGridMode     m_gridMode = EGridMode::Off;
    Standard_Real m_gridStep = 10.0;
    void ApplyViewerGrid();
    void CycleViewerGrid(Standard_Real step);
    bool IsGridOn() const { return m_gridMode != EGridMode::Off; }
    EGridMode GetGridMode() const { return m_gridMode; }

	//--------------occ----------------
	// 操作
public:
    bool m_hasValidClickedPoint;     // 是否有有效点击点
    double m_clickedPointX;          // 点击点X坐标
    double m_clickedPointY;          // 点击点Y坐标
    double m_clickedPointZ;          // 点击点Z坐标
    double temparr[3];
	vector<Handle(AIS_Shape)> m_Sphere;
	ActiveSelectParameter ActiveSelect;
	Standard_Boolean m_drawBias = FALSE;
	Standard_Boolean m_showBias = FALSE;
	std::unique_ptr<SphereCamera> m_camera;
	//Handle(V3d_View) myView;
	void FitAll();
	void selectMode(Handle(AIS_Shape) selectmode); //高亮模型的选择模式
	void selectMode(); //针对所有对象
	BOOL SetAISContext(Handle(AIS_InteractiveContext) myAISContext);
	//设置当前上下文
	BOOL SetViewer(Handle(V3d_Viewer) myViewer);
	//BOOL SwitchAISContext(Handle(AIS_InteractiveContext) myAISContext);
	BOOL SwitchAISContext(int index);//切换上下文
	TopoDS_Shape GetActiveShape();//获取当前模型
	void ClearAISContext(int index);
	void PauseCollecting();
	void CompareCollecting();
	void StartCollecting();
	void CancelCollecting();
	void RemoveCollectedData();
	void SetSphereData(vector<gp_Pnt> position);
	void ClearSphereData();
	void Add(Handle(AIS_Shape) shape);
	void Add(Handle(AIS_Shape) shape, FeatureType type);
	void AddLable(gp_Pnt position, Standard_CString str, bool isSmallBlack = false);
	void Delete(int index);
	void Hide(int index);
	void Show(int index);
	void HighLight(int index);
    Handle(AIS_Shape) m_prevHighlightedShape;
    Quantity_Color m_prevColor;
    Standard_Real m_prevTransparency;
    Standard_Integer m_prevDisplayMode;
    Graphic3d_NameOfMaterial m_prevMaterial;//void ClearHighlight();
	Bnd_Box GetShapeBox(const int index = 0);
	std::vector<std::vector<CString>> m_shapeNames;
	void DisplayShape(Handle(AIS_Shape) aisShape);
	void RemoveShape(Handle(AIS_Shape) aisShape);
	Handle(AIS_Shape)         m_showSphere;   // 小黑球
	Handle(AIS_TextLabel)     m_showLabel;    // 坐标文字
	FeatureType GetFaceType(TopoDS_Shape shape);
	FeatureType GetEdgeType(TopoDS_Shape shape);
	//分别选中面、线、点
	void ActivateFeature(FeatureBaseType type);
	gp_Pnt GetSelectedElementPosition(int index);
	std::vector<CameraImageData> m_ImageData; // 修改为正确的类型
	double GetSelectedElementArea(int index);
	// 声明设置坐标系颜色的方法
	void SetTrihedronColors(Quantity_Color xColor, Quantity_Color yColor, Quantity_Color zColor);
	Quantity_Color m_triColorX
		; gp_Pnt m_cameraPosInCustomCS; // 相机在自定义坐标系中的位置
	Quantity_Color m_triColorY
		; bool m_bAutoGeneratePDF;  // 控制是否自动生成PDF
    Quantity_Color m_triColorZ;
	void GetTrihedronColors(Quantity_Color& xCol, Quantity_Color& yCol, Quantity_Color& zCol) const;
	// 保存成像数据的容器
	// ... 其他成员 ...
    bool m_bFirstTimeGeneratePDF; // 标记是否是第一次生成PDF
    vector<vector<gp_Pnt>> COccView::SortLocation(vector<gp_Pnt> pos);
public:
    void SetAutoGeneratePDF(bool bAuto) {
        m_bAutoGeneratePDF = bAuto;
        m_bFirstTimeGeneratePDF = true; // 重置第一次标志
    }
    bool GetAutoGeneratePDF() const { return m_bAutoGeneratePDF; }
public:
	std
		::vector<Handle(AIS_Shape)> m_projectionSpheres;    // 投影点小球
	std
		::vector<Handle(AIS_Shape)> m_measurementSpheres;   // 测量点小球
	Handle(AIS_Trihedron) m_Trihedron;
	gp_Ax2 m_CurrentCoordinate;
	Handle(AIS_Shape) GetAISShape(int index);
	// 切换透明度状态
	void ToggleTransparency(int shapeIndex);
	int GetShapeCount();
	bool HasTrihedron();
	Handle(V3d_View) m_offscreenView;                     // 离屏视图（独立于主视图）
	Handle(V3d_Viewer) m_offscreenViewer;                 // 离屏视图的Viewer（与主Viewer共享场景）
	CRITICAL_SECTION m_offscreenLock;                     // 线程锁（避免多线程竞争缓冲区）
	bool m_isOffscreenInited;                             // 离屏渲染是否初始化完成
	int m_offscreenWidth;                                 // 离屏渲染分辨率（宽）
	int m_offscreenHeight;
	TopoDS_Shape GetTrihedronAsShape();
	void DisplayProjectionPoint(const gp_Pnt2d& measuredXY);
	//void DisplayProjectionPoint(gp_Pnt measuredPoint);
	//double CalculateZCoordinate(double x, double y);
    BOOL  m_bFirstSnapDone = FALSE;   // 是否已截过 objectSnap
  // 自定义消息//void UpdateActiveFace();
	void ClearRealTimeDisplay();
    void ClearAll();
    void TakeObjectSnapOnce();
	gp_Pnt ProjectPointToSurface(double x, double y);
	/*gp_Pnt SearchNearestPoint(const gp_Pnt& measuredPoint);*/
	gp_Pnt CalculateZFromXY(const gp_Pnt2d& xyPoint);
	/*void DisplayProjectionPoint(const gp_Pnt2d& measuredXY);*/
	//std::vector<gp_Pnt> ProjectPointsToSurface(const std::vector<gp_Pnt>& rawPts);
	//void ClearNearestPoints();
	HWND m_hOffscreenWnd; // 不可见窗口句柄
	HDC m_hOffscreenDC;   // 离屏设备上下文
	TopoDS_Face m_ActiveFace;  // 当前激活的面（用于计算Z坐标）
	Handle(Geom_Surface) m_ActiveSurface;  // 激活面的几何曲面
	Handle(TDocStd_Document) m_XCAFDoc;
	//TDF_Label m_TrihedronLabel; // 保存坐标系标签的引用
	double m_pixelScaleX = 10;          // 默认X像素当量 (mm/pixel)
	double m_pixelScaleY = 10;          // 默认Y像素当量 (mm/pixel)
	double m_dof = 50.0;                   // 默认景深 (mm)
	gp_Trsf  m_csToWorld;   // 自定义坐标系 → 世界
	gp_Trsf  m_worldToCs;   // 世界 → 自定义坐标系
	// 生成图像数据
	Handle(AIS_Shape) m_cameraMarker; // 小红点
	void SetTransparency(int shapeIndex, double transparency);
	Handle(Geom_Axis2Placement) m_trihedronAxis;
	//视图
	void SetTopView();
	void SetSideView();
	void SetFrontView();
	void SetPDF(bool bOpen=true);
	bool CaptureScreenPng(CString Path);
	void MakeBackgroundTransparent(CImage& img, COLORREF bgColor);
	bool COccView::IsTransformValid(const gp_Trsf& trsf) const;
	//Transform
	void Translate(gp_Pnt newpos);

	gp_Pnt Get3dPos(double x, double y);
	Handle(AIS_Shape) m_cameraIcon;       // 相机图标
	Handle(AIS_Shape) m_cameraFrustum;    // 视锥体
	double ComparePoints(vector<gp_Pnt> collectedPoints);
	double SearchNstPoints();
    //vector<vector<pair<gp_Pnt, int>>> SortLocation(vector<gp_Pnt> pos);
	gp_Pnt SearchNearestPoint(gp_Pnt pt);
	//	vector<vector<gp_Pnt>> SortLocation(vector<gp_Pnt> pos);
	Handle(Graphic3d_Camera) occCamera = new Graphic3d_Camera();
	void GetPointsUV(Handle(Geom_Surface) surface, gp_Pnt point, Standard_Real& u, Standard_Real& v);
	void PointOnFace(gp_Pnt& point, gp_Dir dir);
	TopoDS_Face GetSelectFace(int x, int y);
	//面采样
	void FaceSampling(TopoDS_Face face);
	void MeasureCYL(int index, Standard_Boolean isClockWise, Standard_Real startDepth, Standard_Real endDepth, Standard_Real angle, Standard_Integer pointCount, Standard_Integer pathCount);
	void MeasurePln(int index, Standard_Integer uCount, Standard_Integer vCount, Standard_Real ustart, Standard_Real vstart, Standard_Real uend, Standard_Real vend);
	//void MeasurePln(int index, Standard_Real startLoc,Standard_Integer referIndex, Standard_Integer pointCount, Standard_Real offset, Standard_Real startAngle, Standard_Real endAngle);
	void MeasureCir(int index, Standard_Real start, int pointCount, Standard_Boolean isClockWise, Standard_Real depth);
	void MeasureARC(int index, int pointCount, Standard_Real start, Standard_Real end, Standard_Boolean isClockWise, Standard_Real depth);
	void MeasureLine(int index, Standard_Real start, Standard_Real end, int pointCount, gp_Dir deepDir, Standard_Real depth);
	void AddSamplingPoints();
	void RemoveSamplingShape();

	void FaceTriangulation(TopoDS_Face face);
	void FaceMesh(TopoDS_Face face);
	void SetTransparent();
	void SetSolid();
	void CreateCoordinate(gp_Ax2 anAxis);
	// COccView.h
	void DeleteCoordinate();
	void CoordConversion(gp_Ax2 anAxis);
	// COccView.h
	void SetActiveIndex(int index) { activeindex = index; }
	void GetPlaneParameter(int index, gp_Pnt& location, gp_Dir& direction);
	gp_Dir GetLineDir(int index);
	gp_Pnt GetPosition(int index);
	TopoDS_Shape GetShape(int index);
	Standard_Boolean isEmpty();
	std::vector<Handle(AIS_Trihedron)> m_coordSystems;
	void SwitchToMousePicking();
	void PreventMousePicking();

	//回调
	void SetMouseCallBack(MouseButtonCallBack callback);
	void SetAddItemCallBack(AddItemCallBack callback);
	void SetSwitchCallBack(SwitchCallBack callback);
	void SetButtonCallBack(ButtonCallBack callback);
	void SetClearCallBack(ClearCallBack callback);
	void SetModelTransparent(bool transparent) {
		double level = transparent ? 0.7 : 0.0;
		if (!m_Shape.empty()) {
			m_Shape[0][0]->SetTransparency(level);
			m_AISContext->UpdateCurrentViewer();
		}
	}
protected:
	//三维场景转换模式
	enum CurrentAction3d
	{
		CurAction3d_Nothing,
		CurAction3d_DynamicPanning, //平移
		CurAction3d_DynamicZooming, //缩放
		CurAction3d_DynamicRotation, //旋转
		CurAction3d_DynamicTranslation //误差点图移动
	};
	Standard_Integer m_x_max;    //记录鼠标平移坐标X
	Standard_Integer m_y_max;    //记录鼠标平移坐标Y
	float m_scale;    //记录滚轮缩放比例 
	CurrentAction3d m_current_mode; //三维场景转换模式(平移\缩放\旋转)
	bool leftMouseBtn = false; //记录鼠标左键状态
	bool midMouseBtn = false;  //记录鼠标中键状态
	CPoint mouseDownPT;    //控制旋转角度
	CPoint m_Pt;
	int m_translationIndex = 0;
	//---------------------------------
public://occ,view,context
	Handle(AIS_InteractiveContext) m_AISContext;
	Handle(V3d_View) myView;
	// 在 COccView.h 中添加
	double m_baseFocalLength = 100.0; // 或者从相机配置中获取实际值
	Handle(AIS_ColorScale) m_ColorScale;
	//Handle(AIS_Trihedron) m_Trihedron;
	//Handle(V3d_Viewer) GetViewer(void) { return m_Viewer; }
	gp_Ax2 GetCurrentCoordinate() const;
	//接收数据
	//CInteractionBase* m_ibase;
	std::thread th1;
	std::shared_ptr<CIPCBase> m_ipc;
	std::shared_ptr<char> m_content;
	bool m_canWrite;
	std::atomic<bool> m_ipcFlag{ false };
	std::atomic<bool> m_th1IsRun{ true };
	std::atomic<bool> m_ifPause;
	SIGNMEMORY_T tmpSignal{ 0 };
	MSGSIGNAL_T m_signal;
	std::shared_ptr<char> m_signalStr;
	std::shared_ptr<char> m_dataStr;
	std::shared_ptr<char> m_mySignal;
	std::atomic<int> m_mySignalLen;
	DATAMUL_T m_dataMul;
	vector<gp_Pnt> m_position; //导入的txt位置
	std::atomic<int> m_pointType = 0;//收点类型

	vector<RgnBox> m_listRgn;//显示报表集合
	//pdf显示的所有string数据
	vector<string> table; vector<vector<string>> ptTable;
	int activeindex = -1;//当前选中的context索引
	BOOL m_MousePicking = false;
	vector<Handle(AIS_TextLabel)> m_lableList;//显示的lable集合
	vector<vector<Handle(AIS_Shape)>> m_Shape;//存储所有模型
	Handle(AIS_Shape) m_samplingShape;
	vector<vector<gp_Pnt>> m_samplingPoints;
	vector<Handle(AIS_InteractiveContext)> m_context;
	vector<gp_Pnt2d> points;
	// 保存特征数据到文档
	// COccView.h
private:
	std::vector<Handle(AIS_InteractiveObject)> m_temporaryShapes; // 临时形状列表
public:///////////后加的+++
    std::atomic_bool m_alive{ false };
    std::atomic_bool m_ipcReleased{ false };
    void OnMultiModel(char* signal, int res) noexcept;
	std::vector<Quantity_Color> m_featureColors;
	/// <summary>
	/// 相机索引标识符
	/// </summary>
	int m_cameraIndex = 0;
public:
	// 清除所有特征及颜色（在ClearAll中调用）
	void ClearFeatureColors() { m_featureColors.clear(); }

	// 获取指定索引的特征颜色
	Quantity_Color GetFeatureColor(int index) const {
		if (index >= 0 && index < (int)m_featureColors.size()) {
			return m_featureColors[index];
		}
		return Quantity_Color(Quantity_NOC_WHITE); // 默认白色
	}
public:
	// 临时存储特征数据的结构
	struct FeatureData {
		TopoDS_Shape shape;
		FeatureType type;
		gp_Pnt labelPos;
		CString name;
		Quantity_Color color;       // 新增：保存颜色
		Standard_Real transparency; // 新增：保存透明度
	};
	// COccView.h 中声明
	gp_Ax2 GetCoordinate() const { return m_CurrentCoordinate; }	std::vector<FeatureData> m_featureData;
	//回调函数
	MouseButtonCallBack mouseCallBack;
	AddItemCallBack addCallBack;
	SwitchCallBack switchCallBack;
	ButtonCallBack btCallBack;//按钮回调函数
	ClearCallBack clrCallBack;
	CPDFManager* pdfMgr;
	void SetFeatureColor(int index, Quantity_NameOfColor color);
    Handle(V3d_Viewer) GetViewer() const;
    Handle(V3d_View) GetView() const { return myView; }
protected:
	DECLARE_MESSAGE_MAP()
public:
	int m_currentZoomIndex = 0;
	// 变倍命令处理函数
	// 2. 1x按钮点击事件


	// 4. 4x按钮点击事件


	virtual void OnInitialUpdate();
	afx_msg
		void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	void ShowPointCoordinate(const gp_Pnt& pt);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	// 在COccView.h中添加声明
	Handle(AIS_InteractiveContext) GetAISContext() const { return m_AISContext; }
	void CreateLable(RgnBox& rgn, int row, int col, int length);
	//报表位置排列
	//void DrawLableList();
	void RemoveLableList();
	void RedrawLableLine();
	void RedrawSingleLine(int index);
	Quantity_Color RGBToQuantityClr(int r, int g, int b);
	Handle(AIS_ColorScale) ShowColorScale(Standard_CString str, double min, double max, bool isBlueToRed);
    BOOL
        HasPreparedPoints() const {
        return m_isDllPointsReady && !m_tempProfilePoints.empty();
    }
	gp_Pnt m_currentFocus;

	bool m_bTrackFocus = false; // Add this member variable to resolve the undefined identifier error  
	gp_Pnt m_selectedTargetPoint
		;          // 选中的目标点（世界坐标）
	std
		::function<void(gp_Pnt)> m_cameraPosInCustomCSCallback; // 目标点选择回调

	// Initialize the member variable in the constructor  
	bool m_hasTargetPoint;         // 是否已设置目标点
	; // 当前焦点位置
	std
		::vector<unsigned char> m_lastRenderedImage; // 最后渲染的图像数据
	bool m_bRealTimeRender = true; // 实时渲染开关
	gp_Pnt
		GetCurrentFocus() const { return m_currentFocus; }
	void SetRealTimeRender(bool enable) { m_bRealTimeRender = enable; }
	Handle(AIS_Shape) m_aisFocusSphere;




	// 在类定义中添加
	Handle(AIS_Shape) m_cameraPosInCustomCSMarker; // 目标点可视化标记

	gp_Pnt m_cameraPosition;          // 相机位置（新增）
	Standard_Boolean m_hasCameraPosition;  // 是否已设置相机位置（新增）

	// 保持原有的目标点变量（现在作为成像焦点）


	CStatic m_imageDisplay;           // 用于显示图像的静态控件
	CImage m_displayImage;            // 要显示的图像
	bool m_bShowImage;                // 是否显示图像标志



	bool m_bIsGeneratingImage; // 防止重入的标志
	std
		::mutex m_imageDataMutex; // 保护图像数据的互斥锁


	Handle(V3d_View) m_virtualView;        // 虚拟相机视图
	Handle(Image_PixMap) m_virtualPixMap;  // 虚拟视图的像素映射
	bool m_bVirtualViewInitialized;         // 虚拟视图初始化标志
	gp_Pnt m_virtualCameraTarget;           // 虚拟相机目标点（焦点）
	// 存储选取的目标点

	// 目标点选取回调函数
public:
    std::vector<Handle(AIS_Shape)> m_nearestPointSpheres;
	// 最近点小球的默认半径（可根据模型大小调整）
	const Standard_Real m_nearestPointRadius = 0.5;
	// 最近点小球的颜色（建议用鲜明色，如红色）
	const Quantity_NameOfColor m_nearestPointColor = Quantity_NOC_RED;
	// 获取当前选取的目标点
	gp_Pnt
		GetSelectedTargetPoint() const { return m_cameraPosInCustomCS; }
	// 设置目标点选取回调函数
	typedef std::function<void(const gp_Pnt&)> TargetPointCallback;
	void SetTargetPointCallback(TargetPointCallback callback) { m_cameraPosInCustomCSCallback = callback; }
	SphereCamera m_sphereCamera;       // 球面相机控制器


	// 处理位置回调

	// 与 OpenCascade 视图同步

	std::string m_currentMagnification; // 当前变倍级别描述
	std::function<void(gp_Pnt)> m_onCameraMovedInCustomCS;
	void COccView::SetOnCameraMovedInCustomCS(std::function<void(gp_Pnt)> callback)
	{
		m_onCameraMovedInCustomCS = callback;
	}
	//CameraPositionCallback m_cameraPositionCallback = nullptr;

	DWORD m_dwLastCaptureTime; // 上次成像时间（毫秒，控制频率）
	// 新增函数声明

	bool IsAdvancedRendering() const { return m_bUseAdvancedRendering; }
	bool m_bUseAdvancedRendering = false; // 默认使用原始渲染
	void SetRenderingMode(bool useAdvanced) {
		m_bUseAdvancedRendering
			= useAdvanced;
	}
	CPoint m_lastMousePos;

	COpenGLDC* m_pDC;  // 添加OpenGL设备上下文指针
	gp_Pnt focusPointWorld;                // 焦点（世界坐标）
	std::vector<unsigned char> imageData;
	Handle(AIS_Shape) m_cameraPointAIS;
	double m_zOffset = 0.0;
	double m_focalLength = 150.0;     // 50mm焦距

	// 新增：标记图像是否已初始化
	bool m_isImageInited = false;
	int m_imageWidth; // 图像宽度（像素）
	int m_imageHeight;
public:
	// 私有成员：OCC 渲染 + 回调管理
	Handle(Image_PixMap) m_pOccImageBuffer; // OCC 离屏渲染缓冲区（24 位 BGR）
	Handle(Graphic3d_Camera) m_pCamera; // OCC 相机对象
	// 自定义坐标系相关成员（根据原有逻辑补充，示例）
	bool m_isTrihedronReady; // 自定义坐标系是否就绪
	gp_Pnt m_customCameraPos; // 自定义坐标系下的相机位置
	gp_Pnt m_CameraPosInCustomCS
		;   // 相机在自定义坐标系中的位置 (例如 0,0,100)
	gp_Pnt m_TargetPosInCustomCS
		;   // 目标点在自定义坐标系中的位置
	gp_Dir m_CameraUpInCustomCS
		;    // 相机在自定义坐标系中的上方向 (通常就是 m_CurrentCoordinate.YDirection())

			// 派生状态（由核心状态计算而来，用于驱动Occ视图）
	gp_Pnt m_CameraPosInWorldCS
		;    // 相机在世界坐标系中的位置（由 m_CameraPosInCustomCS 经 m_csToWorld 变换得到）
	gp_Pnt m_TargetPosInWorldCS
		;    // 目标点在世界坐标系中的位置（由 m_TargetPosInCustomCS 经 m_csToWorld 变换得到）
	gp_Dir m_CameraUpInWorldCS
		;
#pragma pack(push,1)
	struct CamPose {
		double eye[3];
		double at[3];
		double up[3];
	};
#pragma pack(pop)
	//double CalculateCameraDistance() const;
	//double CalculateCameraDistanceByFOV() const;
	double m_FocusDistance = 100.0; // 焦点距离，相机到目标点的距离
	// 在COccView类中添加成员变量
	// 添加相机参数结构
	struct CameraParameters {
		double focalLength;      // 焦距（mm）
		double aperture;         // 光圈值
		double sensorWidth;      // 传感器宽度（mm）
		double sensorHeight;     // 传感器高度（mm）
		int imageWidth;         // 图像宽度（像素）
		int imageHeight;        // 图像高度（像素）
		double nearPlane;       // 近裁剪平面
		double farPlane;        // 远裁剪平面
	};

	// 在COccView类中添加成员变量
	CameraParameters m_cameraParams
		;
	Handle(OpenGl_Context) m_glContext;
	Handle(OpenGl_FrameBuffer) m_offscreenFBO; // 离屏渲染缓冲区
	;
	double m_sensorWidth;     // 全画幅传感器宽度36mm
	double m_sensorHeight;    // 全画幅传感器高度24mm
	double m_fieldOfView;
	//bool IsCustomCoordinateValid() const;
	//void UpdateCameraPosition();
	double m_fNumber;        // 光圈值（如f/2.8，影响景深）
	double m_focusDistance;  // 对焦距离（相机对焦的目标距离，如1000mm）
	double m_coc;            // 弥散圆直径（CoC，默认取传感器对角线1/1000）
	int m_imageResW;         // 图像分辨率宽度（px，如1920）
	int m_imageResH;         // 图像分辨率高度（px，如1080）

	// 衍生参数（动态计算）
	double m_nearDOF;        // 近景深（清晰范围起点）
	double m_farDOF;         // 远景深（清晰范围终点）
	HDC m_offScreenDC
		;
	//bool m_isOffscreenInited;
	Handle(Aspect_DisplayConnection) m_displayConnection; // 新增：显示连接

	HGLRC m_offScreenRC
		;   Handle(Image_PixMap) m_pixMap;          // 用于存储像素数据的图像对象
	std::vector<uint8_t> m_brgImgBuf;       // 存储24位BRG格式数据的缓冲区
	bool m_isPixMapInited;
	int m_offScreenWidth;
	int m_offScreenHeight;
	std::mutex m_profilePointMutex;       // 点数据互斥锁（已有）
	std::vector<gp_Pnt> m_tempProfilePoints; // DLL传入的临时点集（已有）
	bool m_isDllPointsReady;              // 点是否传完（已有）

	// 新增：计算结果/状态存储
	int m_totalPointCount;                // 传入的测量点总数（避免点集清空后丢失）
	bool m_isCalculated;                  // 是否已完成面轮廓度计算
	double m_calculatedTolerance;



public:
    int m_osdLayer;
	// 3D控件索引号（可外部设置，默认0）
	int m_3dControlIndex;
	// BGR图像缓冲区（内部维护，避免频繁分配）
	unsigned char* m_pBgrBuffer;
	// 缓冲区大小（字节）
	int m_bufferSize;
	int m_imgW = 0;
	int m_imgH = 0;
	std::vector<Standard_Byte> m_imgBuf;// 图像宽高
	Standard_Real
		ComputeNormalDistance(const gp_Pnt& measuredPnt,
			const TopoDS_Face& theoreticalFace);
	int GetImageWidth() const { return m_imgW; }
	int GetImageHeight() const { return m_imgH; }
	void SetImageSize(int w, int h) { m_imgW = w; m_imgH = h; m_imgBuf.resize(w * h, 0); }
	// 离屏渲染缓冲区（已有成员，复用）
	//// OCC视图和上下文（已有成员，复用）
	//typedef void(__stdcall* PF_InitMeasPoints)(int pointCount);
	//typedef bool(__stdcall* PF_SetMeasPoint)(int index, double x, double y, double z);
	//typedef int(__stdcall* PF_GetMeasPointCount)();
	//
	//typedef void(__stdcall* PF_ClearMeasPoints)();
	//typedef void(__stdcall* PF_TriggerOccCalculate)();


public:
	// 获取轮廓度值的公共接口

	// 内部函数：RGB转BGR（原地转换）
	/*HINSTANCE m_hMeasDll = nullptr;
	PF_InitMeasPoints m_pInitMeas = nullptr;
	PF_SetMeasPoint m_pSetPoint = nullptr;
	PF_GetMeasPointCount m_pGetCount = nullptr;

	PF_ClearMeasPoints m_pClearMeas = nullptr;
	PF_TriggerOccCalculate m_pTriggerOcc = nullptr;*/
    Handle(AIS_Trihedron) COccView::GetTrihedron() const {
        return m_Trihedron;
    }
};// 新增：导出C风格接口（供C# P/Invoke调用）


