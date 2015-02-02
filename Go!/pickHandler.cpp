#include "stdafx.h"
#include "pickHandler.h"
#include "math.h"

#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osg/PolygonMode>
#include <osgViewer/Viewer>
#include <osgViewer/CompositeViewer>
#include <osg/Camera>
#include <osg/View>
#include <osg/Group>

PickHandler::PickHandler()
{
}


PickHandler::~PickHandler()
{
}

bool PickHandler::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
	if (ea.getEventType() != osgGA::GUIEventAdapter::PUSH ||
		ea.getButton() != osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
		return false;

	osg::View *viewer = aa.asView();
	if (viewer)
	{
		osg::Matrix vpMatrix;
		osg::Camera *slave(NULL);
		if (viewer->getNumSlaves())
		{
			slave = viewer->getSlave(0)._camera;
			osg::Viewport *vp = slave->getViewport();
			if (vp)
			{
				vpMatrix = vp->computeWindowMatrix();
			}
		}

		_windowsPick.set(ea.getX(), ea.getY());
		osg::Camera *master = viewer->getCamera();
		osg::Matrix mvpw = master->getViewMatrix()*master->getProjectionMatrix()*vpMatrix;
		osg::Matrix inverseMVPW(osg::Matrix::inverse(mvpw));

		_zNear = osg::Vec3d(_windowsPick.x(), _windowsPick.y(), 0.0f) * inverseMVPW;
		_zFar = osg::Vec3d(_windowsPick.x(), _windowsPick.y(), 1.0f) * inverseMVPW;

		osg::Vec3 eye, center, up;
		master->getViewMatrixAsLookAt(eye, center, up);

		osg::Vec3d from = center - eye;
		from.normalize();
		osg::Vec3d to = _zFar - eye;
		to.normalize();

		_updateLookAt.makeIdentity();
		_updateLookAt = osg::Matrix::rotate(from, to);
	}
	return false;
}