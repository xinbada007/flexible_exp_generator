#pragma once
#include <osgViewer/Viewer>
#include <osgViewer/CompositeViewer>
#include <osgText/Text>
#include <osgViewer/api/Win32/GraphicsHandleWin32>

#include <oculusviewer.h>
#include <oculuseventhandler.h>

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
	inline osgViewer::View * getNormalView() const { return _normalView; };
	inline osgViewer::View * getMainView() const { return (_normalView) ? _normalView : _HMDViewer; };
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

	void genMainView(osg::ref_ptr<osg::Node> node);
	bool setMainViewSceneData(osg::Node *node);
	void createHUDView();
	void createBackgroundView();

	int go();

private:
	osgViewer::View * createPowerWall();
	osg::Camera * createSlaveCamerainNormalView(const unsigned screenNum, const osg::GraphicsContext::ScreenSettings &ss, const int startX = 0, const int startY = 0);
	osg::Camera * createHUDCamerainWindow(osg::GraphicsContext *windows);
	void createHMD(osg::ref_ptr<osg::Node> node);
	
	osg::ref_ptr<Screens> _screens;
	osg::ref_ptr<osgViewer::View> _normalView;
	std::vector<osg::Camera*> _slaveCamerasinNormalView;
	osg::ref_ptr<osgViewer::View> _HUDView;
	osgText::Text* _HUDText;
	osg::ref_ptr<osgViewer::View> _BGView;

	osg::ref_ptr<osgViewer::Viewer> _HMDViewer;
	osg::ref_ptr<OculusViewer> _OculusViewer;

	double _hFOV;

protected:
	virtual ~MulitViewer();
};