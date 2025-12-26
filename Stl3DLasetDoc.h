// Stl3DLasetDoc.h :  CStl3DLasetDoc 类的接口
//
#pragma once
#include "..\..\inc\GeomKernel\Entity.h"
#include "figureobj.h"
#include "GocatorManager.h"
#include "Occ.h"

class CStl3DLasetDoc : public CDocument
{
public: // 仅从序列化创建
	CStl3DLasetDoc() noexcept;
	DECLARE_DYNCREATE(CStl3DLasetDoc)

// 属性
public:
	struct LayEr
	{
		COLORREF m_color;
        double m_power;///功率
		int m_markNum;//标记次数
		int m_outMode;//输出模式
		double m_Middlepoint;//中点延时
		double m_NullTread;//空走步长
		double m_MarkTread;	// 标记步长
		double m_MrakSpeed;//速度
		double m_FoldTime;//折点延时
		double m_VectorTime;	// 曲线延时
		double m_ObjTime;	// 对象间延时
		double m_Thefirst;//首点延时
		double m_Eendorder;//末点延时
		double m_close;
		double m_Frequency;
		bool m_SELEFILL;
		double m_fillBeapartfrom;////填充间距
		bool m_Fill_Direction;   //填充方向
		bool X_anti;
		bool Y_anti;
		int FILLMODE;
	}  m_LAYERS[15];
	//CBezierObj* pObj;
	//文档对象链表
	CFigureObjList m_objects;
	void Add(CFigureObj* pObj);
public:

    Handle(AIS_Shape) m_userModel;
    Handle(AIS_Shape) m_autoModel;
    gp_Trsf           m_autoTrsf; 

	//--------------occ----------------
	Handle(V3d_Viewer) GetViewer(void) { return myViewer; }
	Handle(AIS_InteractiveContext) myAISContext;
	void ReadSTEP();
	void ReadIGS();
	void ImportIGS(CString name, INT pyte=1);
	void ImportSTEP(CString name, INT pyte=1);

    // Initialize m_stepContext in the constructor of CStl3DLasetDoc  
  // 添加上下文数组和当前活动上下文索引
	std::vector<Handle(AIS_InteractiveContext)> m_contexts;
	int m_activeContextIndex;
    // Add the declaration for m_stepContexts in the CStl3DLasetDoc class  
 
    // Initialize m_stepContexts in the constructor of CStl3DLasetDoc  
   
	void ResetDocument();

	void ResetViewState();

	void DrawSphere();
	bool ImportModelWithViews(const std::wstring& filePath, int width, int height, double focalLength);
	void RenderMultipleViews(int width, int height, double focalLength);
	//void ClearAISContext(int contextindex);

protected:
	//--------------------------------

public:
	BOOL m_stlRead;
	CSTLModel* pSTLModel;
	CSTLModel pSTLModellist[50];
	CSTLModel* pSTLModel_Scan;
	CSTLModel* pSTLModel_Probe;
	CPart	m_Part;
	CPart	m_Part_Probe;
	CPart   m_Part_PCD;
	int viewnum;//切换视图索引
	std::string FileName;
	//CString STLName;
	bool newfile = false;//新打开文件标识
	CPart Active_part;//当前切换文件
	bool switchflag = false;
	int activeNum = -1;
	bool matchingflag = false;//匹配按钮响应

	PCLMSG* pclmsg;

	//---------点激光数据--------------------------------------
	//CIPCBase* m_ipc;
	//PointCloud3D m_points{ 0 };
	/*WCHAR* m_data;
	double m_dataLen{ 0 };
	int m_dataType{ 0 };
	double Dis[28];
	CPoint3D pts[25];*/
	//---------点激光数据--------------------------------------

public:
	int cenN;
	double zN;
	
	void GetHwnd(HWND hWnd);
// 操作
public:
	CSTLModel* pSTLModel_Pcd;
	CSTLModel* pSTLModel_CT;
	CSTLModel* pSTLModel_TXT;
	vector<float> m_Scanvertex;
	bool txtflag = false;
	int ActiveMenu;//当前选中菜单

// 重写
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	void Remove(CFigureObj* pObj);
// 实现
public:
	virtual ~CStl3DLasetDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成的消息映射函数
protected:
	DECLARE_MESSAGE_MAP()
public:
    Handle(V3d_Viewer) myViewer;
	afx_msg void OnFile3Ddata();
    void Load3DData(LPCTSTR fullpath);
	afx_msg void OnOpenData();
	afx_msg void OnLoadProbe();
	afx_msg
		void SyncFileToTreeAfterLoad(const CString& filePath);
	void OnLoadScan();
public:
    WaferParameters m_wafer_params;	//保存晶圆参数和路径
    CString m_PDFSavePath;
	int m_SelectDistance;
	int GetSetectDistance() { return m_SelectDistance;}
	GocatorManager* m_goManager;
private:
		
public:
	void Import_Stl(CString name, INT pyte);
	void ImportStl(CString name, INT pyte);
    void SetPDFPath(CString path);
	void ImportPcd(CString name, INT pyte);
	void ImportStep_Stp(CString name, INT pyte);
	void ImportExcel(CString name, INT pyte);
	void ImportTxt(CString name, INT pyte);
	void ImportSTPTxt(CString name, INT pyte);
	void ImportSTLTxt(CString name, INT pyte);
	void ImportCT(CString name, INT pyte);
	afx_msg void OnParameSet();
	afx_msg void OnMenuSensorConf();
	afx_msg void OnMenuShowSensorData();
	//切换到不同类型的文件显示
	void SwitchToPCD(string name);
	void SwitchToCT(string name);
	void SwitchToSTL(int stlnum);
	void SwitchToOcc(int index);
	afx_msg void OnFileSave();
	afx_msg void OnCpd2Stl();
	void ClosePCDView();
	void SaveSTL();
	void SaveSTEP();
	void SaveIGS();
};


