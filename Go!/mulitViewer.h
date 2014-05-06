#pragma once
#include <osgViewer/CompositeViewer>
#include "readConfig.h"

class MulitViewer :
	public osgViewer::CompositeViewer
{
public:
	MulitViewer();
	MulitViewer(osg::ref_ptr<ReadConfig> refRC);
	virtual ~MulitViewer();

	void genMainViewer(osg::ref_ptr<ReadConfig> refRC);

private:
	osg::Camera * createSlaveCamera(const unsigned screenNum, const osg::GraphicsContext::ScreenSettings &ss, const int startX = 0, const int startY = 0);

	osgViewer::View * createPowerWall();

	osg::ref_ptr<Screens> _screens;
};

