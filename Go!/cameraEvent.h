#pragma once
#include "readConfig.h"

#include <osgGA/CameraManipulator>
#include <osg/Quat>

typedef std::vector<osg::Camera*> cameraList;
typedef std::vector<osg::Matrixd*> matrixList;

class CameraEvent :
	public osgGA::CameraManipulator
{
public:
	CameraEvent(osg::ref_ptr<ReadConfig> refRC);
	inline void addOffsetMatrixtoList(osg::Matrixd *offset){ _matrixList.push_back(offset); };
	inline void addCameratoList(osg::Camera *cam){ _camList.push_back(cam); };
	inline void setEyePointOffset(osg::Matrixd *eyeOffset) { _eyePointOffset = eyeOffset; };

protected:
	virtual ~CameraEvent();
	virtual osg::Matrixd getInverseMatrix() const;
	virtual osg::Matrixd getMatrix() const;
	virtual void setByInverseMatrix(const osg::Matrixd& matrix);
	virtual void setByMatrix(const osg::Matrixd& matrix);
	virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us);

private:
	void genCamera(osg::ref_ptr<ReadConfig> refRC);
	void updateLookAt(osg::View *viewer);
	osg::Quat _camRotationOrigin;
	osg::Quat _camRotation;
	osg::Vec3d _offsetOrigin;
	osg::Vec3d _offset;
	osg::Vec3d _realOffset;
	osg::Vec3d _eyePoint;

	osg::Quat _eyeRotation;
	osg::Vec3d _eyeOffset;
	const double _offsetInterval;
	const double _rotationInterval;

	osg::Vec3d _eye_X_Axis;
	osg::Vec3d _eye_Z_Axis;
	osg::Vec3d _eye_Y_Axis;

	bool _reset;
	bool _eyeTracker;
	const bool _useHMD;

	osg::Vec2d _windowsPick;
	osg::Vec3d _zNear;
	osg::Vec3 _zFar;
	osg::Matrix _matrixLookAt;

	osg::Matrix _stateLast;

	cameraList _camList;
	matrixList _matrixList;

	osg::Matrix *_eyePointOffset;
};

