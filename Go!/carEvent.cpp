#include "stdafx.h"
#include "carEvent.h"

#include <osg/Notify>
#include <osgGA/EventVisitor>

#include <iterator>

CarEvent::CarEvent():
_carState(NULL), _vehicle(NULL), _mTransform(NULL), _leftTurn(false), _acceleration(true), _updated(false)
{
}

CarEvent::~CarEvent()
{
}

void CarEvent::move()
{
	osg::Vec3d origin = (_leftTurn) ? _carState->_backWheel->front() : _carState->_backWheel->back();

	osg::Matrix rotation = osg::Matrix::translate(-origin);
	rotation *= osg::Matrix::rotate(_carState->_angle*(1.0f / frameRate), Z_AXIS);
	rotation *= osg::Matrix::translate(origin);
	_carState->_moment *= rotation;

	_carState->_direction = _carState->_direction * 
							osg::Matrix::rotate(_carState->_angle*(1.0f / frameRate), Z_AXIS);
	_carState->_direction.normalize();

	_carState->_moment *= osg::Matrix::translate(_carState->_direction * _carState->_speed);
}

void CarEvent::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
	osg::MatrixTransform *refM = dynamic_cast<osg::MatrixTransform*> (node);
	Car *refC = (refM) ? dynamic_cast<Car*>(refM->getUserData()) : NULL;

	osgGA::EventVisitor *ev = dynamic_cast<osgGA::EventVisitor*>(nv);
	osgGA::EventQueue::Events events = (ev)?ev->getEvents():events;
	osgGA::GUIEventAdapter *ea = (!events.empty())?events.front():NULL;

	if (refC && ea)
	{
		//update
		if (!_updated)
		{
			_carState = refC->getCarState();
			_vehicle = refC->getVehicle();
			_mTransform = refM;
			_acceleration = _vehicle->_acceleration;
			_updated = true;
		}
		_carState->_moment.makeIdentity();

		const int &key = ea->getKey();
		switch (ea->getEventType())
		{
		case osgGA::GUIEventAdapter::KEYDOWN:
			if ((key == 'a' || key == 'd'))
			{
				int sign = (key == 'a') ? 1 : -1;
				_leftTurn = (sign == 1) ? true : false;
				_carState->_angle += _vehicle->_rotate*sign*_carState->_angle_incr;
				break;
			}
			if (key == 'w' || key == 's')
			{
				int sign = (key == 'w') ? 1 : -1;
				_carState->_speed += _vehicle->_speed*sign*_carState->_speed_incr;
				break;
			}
			if ((key == '`'))
			{
				_acceleration = !_acceleration;
				break;
			}
		case osgGA::GUIEventAdapter::KEYUP:
			if (key == 'a' || key == 'd')
			{
				_carState->_angle = 0.0f;
				break;
			}
		case osgGA::GUIEventAdapter::FRAME:
			_carState->_speed = abs(_carState->_speed) > _vehicle->_speed ? _vehicle->_speed*(_carState->_speed > 0 ? 1 : -1) : _carState->_speed;
			_carState->_angle = abs(_carState->_angle) > _vehicle->_rotate ? _vehicle->_rotate*(_carState->_angle > 0 ? 1 : -1) : _carState->_angle;

			if (_carState->_speed > 0 && _carState->_collide)
			{
				_carState->_speed = 0;
				_carState->_collide = false;
			}

			if (!_acceleration)
			{
				if(*(_carState->_OQuad))
				{
					osg::Vec3 carHeading = _carState->_direction;

					Plane::reverse_across_iterator i = *_carState->_OQuad;
					osg::Vec3 nextRoad = (*i)->getLoop()->getNavigationEdge()->front();
					nextRoad -= (*i)->getLoop()->getNavigationEdge()->back();

					carHeading.normalize();
					nextRoad.normalize();

					_leftTurn = ((carHeading^nextRoad).z() > 0);
					_carState->_angle = acos(carHeading*nextRoad)*(_leftTurn ? 1 : -1);
				}
			}

			move();
			_carState->_directionLastFrame = _carState->_direction;
			caclCarMovement();

			break;

		default:
			break;
		}
	}
	
	traverse(node, nv);
}

void CarEvent::caclCarMovement()
{
	_carState->_state *= _carState->_moment;

	_carState->_O = _carState->_O * _carState->_moment;
	arrayByMatrix(_carState->_backWheel, _carState->_moment);
	arrayByMatrix(_carState->_frontWheel, _carState->_moment);
	arrayByMatrix(_carState->_carArray, _carState->_moment);

	_mTransform->setMatrix(_carState->_state);

	if (!_carState->_lastQuad.empty())
	{
		osg::ref_ptr<osg::Vec3dArray> navigationEdge = _carState->_lastQuad.back()->getLoop()->getNavigationEdge();
		const osg::Vec3d naviEdge = navigationEdge->back() - navigationEdge->front();
		const osg::Vec3d O = _carState->_O;
		const double lamida = ((O - navigationEdge->front()) * naviEdge) / naviEdge.length2();
		const osg::Vec3d N = navigationEdge->back()*lamida + navigationEdge->front()*(1 - lamida);
		const double dis = (N - O).length();
		_carState->_O_Project = N;
		_carState->_dither = dis;

		_carState->_midLine->clear();
		copy(navigationEdge->begin(), navigationEdge->end(), std::back_inserter(*_carState->_midLine));

		_carState->_updated = true;
	}	
}