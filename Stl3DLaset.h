// Stl3DLaset.h : Stl3DLaset 应用程序的主头文件
//
#pragma once
//#define MYMFCDLL1_EXPORTS
//#ifdef MYMFCDLL1_EXPORTS
//#define MYMFCDLL1_API __declspec(dllexport)
//#else
//#define MYMFCDLL1_API __declspec(dllimport)
//#endif


#include "resource.h"       // 主符号
#include "Occ.h"
#include "MainFrm.h"
#include "ModelImport.h"

// CStl3DLasetApp:
// 有关此类的实现，请参阅 Stl3DLaset.cpp
//

class CStl3DLasetApp : public CWinApp
{
public:
	CStl3DLasetApp();
	~CStl3DLasetApp();
	CSingleDocTemplate* pTemplate = nullptr;
	// 重写
public:
	virtual BOOL InitInstance();

	// 实现
	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()

public:
	//--------------occ----------------
	opencascade::handle<OpenGl_GraphicDriver> m_GraphicDriver;
	Handle(OpenGl_GraphicDriver) GetGraphicDriver() { return m_GraphicDriver; }
	//---------------------------------
	int m_nAppLook;  // 存储应用外观设置
	BOOL m_bHiColorIcons;  // 高色彩图标标志

private:
	ULONG_PTR gdiplusupToken;
public:

	virtual int ExitInstance();
};

//extern CStl3DLasetApp theApp;
extern CMainFrame* pMain;
#define MYMFCDLL1_API __declspec(dllexport)

#ifdef __cplusplus
extern "C" {
#endif

	// 3D窗口管理函数
	//MYMFCDLL1_API void* GetDllOccView();
	MYMFCDLL1_API void __stdcall CloseDll();
	//MYMFCDLL1_API HANDLE __stdcall Show3DWindow(HANDLE fatherHwnd);
	//MYMFCDLL1_API int Hide3DWindow(HANDLE hwnd);



	// 轮廓度测量函数
	MYMFCDLL1_API double SetSurfaceProfilePoint(double x, double y, double z, int flagBit);
	MYMFCDLL1_API double GetsurfaceProfile();

	// 模型导入函数
	MYMFCDLL1_API int SetProfile(char* filePath, int len);

#ifdef __cplusplus
}
#endif



typedef void(__stdcall* pfnImageCallback)(int, int, int, const unsigned char*);
typedef void(__stdcall* pfnPositionCallback)(int, double, double, double);
typedef void(__stdcall* pfn3DfileCallback)(int, const char*);

static std::vector<std::vector<unsigned char>> g_images;
static pfnImageCallback g_imageCallback = nullptr;
static pfnPositionCallback g_positionCallback = nullptr;
extern pfn3DfileCallback g_3dFileCallback;
extern int D_index;
static int LocateCameraPos(int index);
struct MotionState
{
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};
static MotionState g_motion;
static std::mutex  g_motionMutex;
extern ModelImport g_modelopened;

static float g_gammaRing = 0.7f;
static float g_gainRing = 1.8f;

static float g_gammaCoax = 0.8f;
static float g_gainCoax = 2.5f;

static float g_gammaBottom = 0.9f;
static float g_gainBottom = 1.2f;

auto mapGamma = [](uint16_t raw, float gamma, float gain)->float
{
    float k = static_cast<float>(raw) / 65535.0f;
    float lum = std::pow(k, gamma) * gain;
    if (lum < 0.0f) lum = 0.0f;
    if (lum > 1.0f) lum = 1.0f;
    return lum;
};

struct LightCache
{
    int ringCount = 0;
    int regionCount = 0;
    std::vector<uint16_t> data;
};
static LightCache g_lightCache;

extern "C" {

    __declspec(dllexport) int Init(int index, const char* file3DPath);
    __declspec(dllexport) int DeInit();
    __declspec(dllexport) HANDLE Show3DWindow2(HANDLE fatherHwnd);
    __declspec(dllexport) int Hide3DWindow2(HANDLE hwnd);
    __declspec(dllexport) int Show3DWindow();
    __declspec(dllexport) int Hide3DWindow();
    __declspec(dllexport) int Set3DfileCallback(pfn3DfileCallback callback);
    __declspec(dllexport) int SetImageCallback(pfnImageCallback callback);
    __declspec(dllexport) int SetPositionCallback(pfnPositionCallback callback);
    __declspec(dllexport) int PositionMotionOffset(double* OffsetArray);
    __declspec(dllexport) int PositionMotion(double* PositionArray);
    __declspec(dllexport) int SetLightBrightness(int RingCount, int RegionCount, int** light_brightness);
    __declspec(dllexport) int SetScalePara(int zoom_index, TScalePara* scale_para, double DOF_mm);
    __declspec(dllexport) int Zoom(int zoom_index);
    __declspec(dllexport) int InitImage(int image_width, int image_height);
}

