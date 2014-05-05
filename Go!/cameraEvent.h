#pragma once
#include <osgGA/CameraManipulator>
#include <osg/Quat>

class CameraEvent :
	public osgGA::CameraManipulator
{
public:
	CameraEvent();
	virtual ~CameraEvent();
protected:
	virtual osg::Matrixd getInverseMatrix() const;
	virtual osg::Matrixd getMatrix() const;
	virtual void setByInverseMatrix(const osg::Matrixd& matrix);
	virtual void setByMatrix(const osg::Matrixd& matrix);
	virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us);
private:
	osg::Quat _camRotation;
	osg::Vec3 _offset;
	osg::Vec3 _eyePoint;
};

