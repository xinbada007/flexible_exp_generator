#include "stdafx.h"
#include "cameraEvent.h"
#include "car.h"
#include "math.h"

#include <osgViewer/Viewer>

CameraEvent::CameraEvent():
_reset(false), _eyeTracker(false), _rotationInterval(5.0f * TO_RADDIAN), _offsetInterval(0.5f)
{
	osg::Matrix lMat;
	lMat.makeRotate(PI_2, X_AXIS);
	_camRotation = lMat.getRotate();
	_camRotationOrigin = _camRotation;

	_eye_X_Axis = X_AXIS;
	_eye_Z_Axis = Z_AXIS;

	_offset.set(0.0f, 0.0f, 0.0f);
	_offsetOrigin = _offset;

	_eyePoint.set(O_POINT + _offset);
}


CameraEvent::~CameraEvent()
{
}

void CameraEvent::setByMatrix(const osg::Matrixd& matrix)
{
}

void CameraEvent::setByInverseMatrix(const osg::Matrixd& matrix)
{
}

osg::Matrixd CameraEvent::getMatrix() const
{
	return osg::Matrix::inverse(_matrixLookAt) *
			    osg::Matrix::rotate(_camRotation) *
				    osg::Matrix::translate(_eyePoint);
}

osg::Matrixd CameraEvent::getInverseMatrix() const
{
	return osg::Matrix::translate(-_eyePoint) * 
		       osg::Matrix::rotate(_camRotation.inverse()) * 
					_matrixLookAt;
}

void CameraEvent::updateLookAt(osg::View *viewer)
{
	if (!viewer)
	{
		return;
	}

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

	osg::Camera *master = viewer->getCamera();
	osg::Matrix mvpw = master->getViewMatrix()*master->getProjectionMatrix()*vpMatrix;
	osg::Matrix inverseMVPW(osg::Matrix::inverse(mvpw));

	_zNear = osg::Vec3d(_windowsPick.x(), _windowsPick.y(), 0.0f) * inverseMVPW;
	_zFar = osg::Vec3d(_windowsPick.x(), _windowsPick.y(), 1.0f) * inverseMVPW;

	osg::Vec3 eye, center, up;
	master->getViewMatrixAsLookAt(eye, center, up);

	osg::Matrix toView;
	toView.makeLookAt(eye, _zFar, up);

	_matrixLookAt = osg::Matrix::inverse(master->getViewMatrix()) * toView;
}

bool CameraEvent::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
	const Car * refC = dynamic_cast<Car*>(this->getUserData());
	osg::View *viewer = us.asView();

	if (refC)
	{
		const CarState * refCS = refC->getCarState();
		switch (ea.getEventType())
		{
		case osgGA::GUIEventAdapter::KEYDOWN:
			switch (ea.getKey())
			{
			case osgGA::GUIEventAdapter::KEY_Up:
				_eyeRotation.makeRotate(_rotationInterval / frameRate, _eye_X_Axis);
				break;
			case osgGA::GUIEventAdapter::KEY_Down:
				_eyeRotation.makeRotate(-_rotationInterval / frameRate, _eye_X_Axis);
				break;
			case osgGA::GUIEventAdapter::KEY_Left:
				_eyeRotation.makeRotate(_rotationInterval / frameRate, _eye_Z_Axis);
				break;
			case osgGA::GUIEventAdapter::KEY_Right:
				_eyeRotation.makeRotate(-_rotationInterval / frameRate, _eye_Z_Axis);
				break;
			case osgGA::GUIEventAdapter::KEY_KP_Up:
				_eyeOffset = _eyeOffset * osg::Matrix::translate(_eye_Z_Axis * (_offsetInterval / frameRate));
				break;
			case osgGA::GUIEventAdapter::KEY_KP_Down:
				_eyeOffset = _eyeOffset * osg::Matrix::translate(-_eye_Z_Axis * (_offsetInterval / frameRate));
				break;
			case osgGA::GUIEventAdapter::KEY_KP_Left:
				_eyeOffset = _eyeOffset * osg::Matrix::translate(-_eye_X_Axis * (_offsetInterval / frameRate));
				break;
			case osgGA::GUIEventAdapter::KEY_KP_Right:
				_eyeOffset = _eyeOffset * osg::Matrix::translate(_eye_X_Axis * (_offsetInterval / frameRate));
				break;
			case osgGA::GUIEventAdapter::KEY_KP_Enter:
				_reset = true;
				break;
			default:
				break;
			}
			break;
		case osgGA::GUIEventAdapter::KEYUP:
			if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Up || ea.getKey() == osgGA::GUIEventAdapter::KEY_Down
				|| ea.getKey() == osgGA::GUIEventAdapter::KEY_Left || ea.getKey() == osgGA::GUIEventAdapter::KEY_Right)
			{
				_eyeRotation = osg::Matrix::identity().getRotate();
			}
			if (ea.getKey() == osgGA::GUIEventAdapter::KEY_KP_Up || ea.getKey() == osgGA::GUIEventAdapter::KEY_KP_Down
				|| ea.getKey() == osgGA::GUIEventAdapter::KEY_KP_Left || ea.getKey() == osgGA::GUIEventAdapter::KEY_KP_Right)
			{
				_eyeOffset.set(0.0f, 0.0f, 0.0f);
			}
			break;
		case osgGA::GUIEventAdapter::MOVE:
			if (viewer && _eyeTracker)
			{
				_windowsPick.set(ea.getX(), ea.getY());
				updateLookAt(viewer);
			}
		case osgGA::GUIEventAdapter::FRAME:
			_offsetOrigin = _offsetOrigin * osg::Matrix::rotate(refCS->_moment.getRotate());
			_offset = _offset * osg::Matrix::rotate(refCS->_moment.getRotate());
			_offset = _offset * osg::Matrix::translate(_eyeOffset);

			_camRotationOrigin *= refCS->_moment.getRotate();
			_camRotation *= refCS->_moment.getRotate();
			_camRotation *= _eyeRotation;

			_eye_X_Axis = _eye_X_Axis * osg::Matrix::rotate(refCS->_moment.getRotate());
			_eye_Z_Axis = _eye_Z_Axis * osg::Matrix::rotate(refCS->_moment.getRotate());

			if (_reset)
			{
				_camRotation = _camRotationOrigin;
				_offset = _offsetOrigin;
				_reset = false;
			}

			_eyePoint.set(refCS->_O + _offset);
			break;
		default:
			break;
		}

		return false;
	}
	return false;
}

void CameraEvent::genCamera(osg::ref_ptr<ReadConfig> refRC)
{
	if (refRC->getCameraSet())
	{
		if (refRC->getCameraSet()->_offset->size() == _offset.num_components)
		{
			_offset.x() = refRC->getCameraSet()->_offset->at(0);
			_offset.y() = refRC->getCameraSet()->_offset->at(1);
			_offset.z() = refRC->getCameraSet()->_offset->at(2);
		}
		_offsetOrigin = _offset;

		_eyeTracker = refRC->getCameraSet()->_eyeTracker;
	}
}