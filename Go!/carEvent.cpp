#include "stdafx.h"
#include "carEvent.h"

#include <osg/Notify>
#include <osgGA/EventVisitor>

#include <iterator>

CarEvent::CarEvent():
_carState(NULL), _mTransform(NULL), _leftTurn(false)
{
}

CarEvent::~CarEvent()
{
}

void CarEvent::turn()
{
	osg::Vec3 origin = (_leftTurn) ? _carState->_backWheel->front() : _carState->_backWheel->back();
	_carState->_moment *= osg::Matrix::translate(-origin);
	_carState->_moment *= osg::Matrix::rotate(_carState->_angle*(1.0f / 60.0f), Z_AXIS);
	_carState->_moment *= osg::Matrix::translate(origin);
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
		//get car state
		_carState = refC->getCarState();
		_carState->_moment.makeIdentity();
		const Vehicle * const refV = refC->getVehicle();
		_mTransform = refM;

		const int &key = ea->getKey();
		switch (ea->getEventType())
		{
		case osgGA::GUIEventAdapter::KEYDOWN:
			if ((key == 'a' || key == 'd'))
			{
				int sign = (key == 'a') ? 1 : -1;
				_leftTurn = (SING == 1) ? true : false;
				_carState->_angle += refV->_rotate*sign*_carState->_angle_incr;
				break;
			}
			if (key == 'w' || key == 's')
			{
				int sign = (key == 'w') ? 1 : -1;
				_carState->_speed += refV->_speed*sign*_carState->_speed_incr;
				break;
			}
		case osgGA::GUIEventAdapter::KEYUP:
			if (key == 'a' || key == 'd')
			{
				_carState->_angle = 0.0f;
				break;
			}
		case osgGA::GUIEventAdapter::FRAME:
			if (_carState->_speed > 0 && _carState->_collide)
			{
				_carState->_speed = 0;
				_carState->_collide = false;
			}

			if (refV->_acceleration)
			{
				osg::Vec3 backwheel = (_carState->_backWheel->front()+_carState->_backWheel->back()) / 2;
				turn();
			}
			_carState->_directionLastFrame = _carState->_direction;
			_carState->_direction = _carState->_direction * osg::Matrix::rotate(_carState->_angle*(1.0f / 60.f), Z_AXIS);
			_carState->_direction.normalize();
			_carState->_moment *= osg::Matrix::translate(_carState->_direction * _carState->_speed * (1.0f / 60.0f));

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
		osg::ref_ptr<osg::Vec3Array> navigationEdge = _carState->_lastQuad.back()->getLoop()->getNavigationEdge();
		const osg::Vec3 naviEdge = navigationEdge->back() - navigationEdge->front();
		const osg::Vec3 O = _carState->_O;
		const double lamida = ((O - navigationEdge->front()) * naviEdge) / naviEdge.length2();
		const osg::Vec3 N = navigationEdge->back()*lamida + navigationEdge->front()*(1 - lamida);
		const double dis = (N - O).length();
		_carState->_O_Project = N;
		_carState->_dither = dis;

		_carState->_midLine->clear();
		copy(navigationEdge->begin(), navigationEdge->end(), std::back_inserter(*_carState->_midLine));

		_carState->_updated = true;
	}	
}