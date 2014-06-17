#pragma once
#include <osgViewer/CompositeViewer>
#include <osgText/Text>

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
	MulitViewer();
	MulitViewer(osg::ref_ptr<ReadConfig> refRC);
	virtual ~MulitViewer();

	inline osgViewer::View * getMainView() const { return _mainView; };
	inline osgViewer::View * getHuDView() const { return _HUDView; };
	inline osgViewer::View * getBGView() const { return _BGView; };
	inline std::vector<osg::Camera*> getSlaveCamerasinMainView() const { return _slaveCamerasinMainView; };

	void genMainView(osg::ref_ptr<ReadConfig> refRC);
	void createHUDView();
	void setHUDContent(osgText::Text *ref);
	void createBackgroundView();

private:
	osgViewer::View * createPowerWall();
	osg::Camera * createSlaveCamerainMainView(const unsigned screenNum, const osg::GraphicsContext::ScreenSettings &ss, const int startX = 0, const int startY = 0);
	osg::Camera * createHUDCamerainWindow(osg::GraphicsContext *windows);

	osg::ref_ptr<Screens> _screens;
	osg::ref_ptr<osgViewer::View> _mainView;
	std::vector<osg::Camera*> _slaveCamerasinMainView;
	osg::ref_ptr<osgViewer::View> _HUDView;
	osgText::Text* _HUDText;
	osg::ref_ptr<osgViewer::View> _BGView;
};

