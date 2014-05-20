#pragma once
#include <osgGA/GUIEventHandler>

class PickHandler :
	public osgGA::GUIEventHandler
{
public:
	virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa);
	PickHandler();
	virtual ~PickHandler();

private:
	osg::Vec2d _windowsPick;
	osg::Vec3d _zNear;
	osg::Vec3d _zFar;
	osg::Matrix _updateLookAt;
};

