#include "stdafx.h"
#include "cameraEvent.h"
#include "car.h"

CameraEvent::CameraEvent()
{
	osg::Matrix lMat;
	lMat.makeRotate(PI_2, X_AXIS);
	_camRotation = lMat.getRotate();

	_offset.set(0.0f, -0.15f, 0.030f);
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
	return osg::Matrixd::rotate(_camRotation);
}

osg::Matrixd CameraEvent::getInverseMatrix() const
{
	return osg::Matrix::translate(-_eyePoint) * osg::Matrix::rotate(_camRotation.inverse());
}

bool CameraEvent::handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us)
{
	const Car * refC = dynamic_cast<Car*>(this->getUserData());
	if (refC)
	{
		const CarState * refCS = refC->getCarState();
		switch (ea.getEventType())
		{
		case osgGA::GUIEventAdapter::FRAME:
			_offset = _offset * osg::Matrix::rotate(refCS->_moment.getRotate());
			_camRotation *= refCS->_moment.getRotate();
			_eyePoint.set(refCS->_O + _offset);
		default:
			break;
		}

		return false;
	}
	
	return true;
}