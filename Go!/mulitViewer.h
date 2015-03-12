#pragma once
#include <osgViewer/Viewer>
#include <osgViewer/CompositeViewer>
#include <osgText/Text>
#include <osgViewer/api/Win32/GraphicsHandleWin32>

#include <OVR.h>
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>

#include <vector>

#include "readConfig.h"

namespace RenderOrder{
	typedef enum RENDERORDER
	{
		BACKGROUND, HUDDISPLAY
	}RENDERORDER;
}

class MulitViewer :
	public osgViewer::CompositeViewer
{
public:
	MulitViewer(osg::ref_ptr<ReadConfig> refRC);

	inline double getHorizontalFov() const { return _hFOV; };
	inline osgViewer::View * getHMDView() const { return _hmdView; };
	inline osgViewer::View * getNormalView() const { return _normalView; };
	inline osgViewer::View * getMainView() const { return (_normalView) ? _normalView : _hmdView; };
	inline std::vector<osg::Camera*> getSlaveCamerasinMainView() const { return _slaveCamerasinNormalView; };

	inline osgViewer::View * getHuDView() const { return _HUDView; };
	enum HUDPOS
	{
		LEFT,
		CENTRE,
		RIGHT
	};
	inline osg::Camera * getHUDCamera(HUDPOS ref) const 
	{
		if (!_HUDView) return NULL;
		if (!_HUDView->getNumSlaves())
		{
			return _HUDView->getCamera();
		}

		switch (ref)
		{
		case::MulitViewer::LEFT:
			return _HUDView->getSlave(0)._camera;
			break;
		case::MulitViewer::CENTRE:
			return _HUDView->getSlave(_HUDView->getNumSlaves() / 2)._camera;
			break;
		case::MulitViewer::RIGHT:
			return _HUDView->getSlave(_HUDView->getNumSlaves() - 1)._camera;
			break;
		default:
			return NULL;
			break;
		}
	}

	inline osgViewer::View * getBGView() const { return _BGView; };

	void genMainView();
	bool setMainViewSceneData(osg::Node *node);
	void createHUDView();
	void createBackgroundView();

	int go();
private:
	osgViewer::View * createPowerWall();
	osg::Camera * createSlaveCamerainNormalView(const unsigned screenNum, const osg::GraphicsContext::ScreenSettings &ss, const int startX = 0, const int startY = 0);
	osg::Camera * createHUDCamerainWindow(osg::GraphicsContext *windows);
	
	osg::Camera * createRTTCamera(osg::Camera::BufferComponent buffer,osg::Texture *tex);
	osg::Camera * createSlaveCamerainHMD(osgViewer::View *view, osg::ref_ptr<osg::Texture2D> tex);

	bool hmd_Initialise();
	bool setHMDSceneData(osg::Node *node);
	void runHMD();
	void shutdownHMD();
	ovrHmd _hmd;
	ovrGLConfig _glCfg;
	ovrEyeRenderDesc _eyeRenderDesc[2];
	ovrVector3f _eyeOffsets[2];
	ovrPosef _eyePoses[2];
	ovrTexture _eyeTextures[2];
	OVR::Matrix4f _projectionMatrici[2];
	OVR::Sizei _renderTargetSize;
	ovrSizei _windowsR;
	ovrSizei _targetSize;
	ovrSizei _eyeTextureSize[2];
	osg::ref_ptr<osg::Texture2D> _masterTex;
	osg::ref_ptr<osgViewer::View> _hmdView;

	osg::ref_ptr<Screens> _screens;
	osg::ref_ptr<osgViewer::View> _normalView;
	std::vector<osg::Camera*> _slaveCamerasinNormalView;
	osg::ref_ptr<osgViewer::View> _HUDView;
	osgText::Text* _HUDText;
	osg::ref_ptr<osgViewer::View> _BGView;

	double _hFOV;

protected:
	virtual ~MulitViewer();
};

class swapcallback:
	public osg::GraphicsContext::SwapCallback
{
public:
	swapcallback(){};
	~swapcallback(){};
	virtual void swapBuffersImplementation(osg::GraphicsContext* gc){};
};