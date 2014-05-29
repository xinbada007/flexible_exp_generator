#include "stdafx.h"
#include "carEvent.h"
#include "math.h"

#include <osg/Notify>
#include <osgGA/EventVisitor>

CarEvent::CarEvent():
_carState(NULL), _vehicle(NULL), _mTransform(NULL), _leftTurn(false), _acceleration(true), _updated(false)
{
	_lastAngle = 0.0f;
	_lastTurn = false;
}

CarEvent::~CarEvent()
{
	std::cout << "Deconstruct CarEvent" << std::endl;
}

void CarEvent::calculateCarMovement()
{
	//Limit the speed under MAX
	_carState->_speed = abs(_carState->_speed) > _vehicle->_speed ? _vehicle->_speed*(_carState->_speed > 0 ? 1 : -1) : _carState->_speed;
	_carState->_angle = abs(_carState->_angle) > _vehicle->_rotate ? _vehicle->_rotate*(_carState->_angle > 0 ? 1 : -1) : _carState->_angle;

	//set rotation direction and limit
	_carState->_angle = abs(_carState->_angle) * (_leftTurn ? 1 : -1);
	_carState->_angle = (_carState->_speed == 0) ? 0.0f : _carState->_angle;

	//set the acceleration of rotation
	const double ROTATIONACCL = abs(_carState->_angle - _lastAngle)*frameRate;
	const double MAX_ROTATIONACCL = _vehicle->_rotationAccl;
	if (ROTATIONACCL > MAX_ROTATIONACCL)
	{
		if (_carState->_angle * _lastAngle <= 0)
		{
			if (_lastAngle >= 0)
			{
				_carState->_angle = _lastAngle - MAX_ROTATIONACCL / frameRate;
				_leftTurn = (_carState->_angle > 0);
			}
			if(_lastAngle < 0)
			{
				_carState->_angle = _lastAngle + MAX_ROTATIONACCL / frameRate;
				_leftTurn = (_carState->_angle > 0);
			}
		}
		if (_carState->_angle * _lastAngle > 0)
		{
			if (_carState->_angle >= _lastAngle)
			{
				_carState->_angle = _lastAngle + MAX_ROTATIONACCL / frameRate;
			}
			else
			{
				_carState->_angle = _lastAngle - MAX_ROTATIONACCL / frameRate;
			}
		}
	}
	_lastAngle = _carState->_angle;

	//apply the rotation and speed and direction
	osg::Vec3d origin = (_leftTurn) ? _carState->_backWheel->front() : _carState->_backWheel->back();
	osg::Matrix rotation = osg::Matrix::translate(-origin);
	rotation *= osg::Matrix::rotate(_carState->_angle*(1.0f / frameRate), Z_AXIS);
	rotation *= osg::Matrix::translate(origin);
	_carState->_moment *= rotation;

	_carState->_direction = _carState->_direction * 
							osg::Matrix::rotate(_carState->_angle*(1.0f / frameRate), Z_AXIS);
	_carState->_direction.normalize();
	_carState->_directionLastFrame = _carState->_direction;

	_carState->_moment *= osg::Matrix::translate(_carState->_direction * _carState->_speed);
}

void CarEvent::autoNavigation()
{
	if (_acceleration || !(*_carState->_OQuad))
	{
		return;
	}

	Plane::reverse_across_iterator i = *_carState->_OQuad;
	osg::ref_ptr<osg::Vec3dArray> naviEdge = (*i)->getLoop()->getNavigationEdge();
	osg::Vec3d roadHeading = naviEdge->front() - naviEdge->back(); roadHeading.normalize();
	osg::Vec3d carHeading = _carState->_direction; carHeading.normalize();
	if (_carState->_speed < 0)	carHeading = -carHeading;
	
	const double DOTPRODUCT = roadHeading*carHeading;
	const double CROSSPRODUCT = (roadHeading^carHeading).z();
	const double COSANGLE = acos(DOTPRODUCT);
	const double SINANGLE = asin(CROSSPRODUCT);
	const bool SAMEDIRECTION = (DOTPRODUCT >= 0);
	const bool LEFT = (CROSSPRODUCT >= 0);
	const bool TURN = SAMEDIRECTION ? !LEFT : LEFT;
	double headingError = abs(SINANGLE) * abs(_carState->_speed*frameRate);
	headingError = (headingError > 1.0f * TO_RADDIAN) ? (headingError - 1.0f * TO_RADDIAN) : 0.0f;

	double roadDistance(0.0f);
	double angle_1d(0.0f);
	bool turnF(TURN);
	const int FORWAROD = (SAMEDIRECTION) ? 1 : -1;
	while (*(i += FORWAROD))
	{
		naviEdge = (*i)->getLoop()->getNavigationEdge();
		roadHeading = naviEdge->front() - naviEdge->back();
		roadDistance += roadHeading.length();
		roadHeading.normalize();
		const double DOTPRODUCT = roadHeading*carHeading;
		const double CROSSPRODUCT = (roadHeading^carHeading).z();
		const double SINANGLE = asin(CROSSPRODUCT);
		const double COSANGLE = acos(DOTPRODUCT);
		const bool SAMEDIRECTION = (DOTPRODUCT >= 0);
		const bool LEFT = (CROSSPRODUCT >= 0);
		turnF = SAMEDIRECTION ? !LEFT : LEFT;
		const double ANGLE = abs(SINANGLE);
		if (ANGLE / TO_RADDIAN >= 1.0f)
		{
			angle_1d = ANGLE;
			angle_1d /= (roadDistance / (_carState->_speed * frameRate));
			angle_1d = abs(angle_1d);
			break;
		}
		if (roadDistance >= abs(_carState->_speed) * frameRate)
		{
			break;
		}
	}

	const double ANGLE(angle_1d);

	const double RatationAngle = (ANGLE > headingError) ? ANGLE : headingError;
	const bool RatationTurn = (ANGLE > headingError) ? turnF : TURN;

	_leftTurn = RatationTurn;
	_carState->_angle = RatationAngle;

	return;
}

bool CarEvent::Joystick()
{
	extern bool poll_joystick(int &x, int &y, int &b);
	int x(0), y(0), b(0);
	if (!poll_joystick(x, y, b))
	{
		return false;
	}

	const double MAX(32767.0f);
	const double MAX_ANGLE = 120.0f;
	const double DEAD = 0.10f;
	const int DeadZone(MAX*DEAD);
	_carState->_swangle = (x / MAX)*MAX_ANGLE;

	if ((abs(x)) > DeadZone)
	{
		_carState->_angle = _vehicle->_rotate * (double(abs(x)) / MAX) * (x > 0 ? -1 : 1);
	}
	else if ((abs(x)) <= DeadZone)
	{
		_carState->_angle = 0.0f;
		_carState->_swangle = 0.0f;
	}

	if (abs(y) > DeadZone)
	{
		_carState->_speed += _vehicle->_speed * (double(abs(y)) / MAX) * (y > 0 ? -1 : 1);
	}
	return true;
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

		Joystick();

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

			if (_carState->_speed > 0 && _carState->_collide)
			{
				_carState->_speed = _vehicle->_speed * .01f;
				_carState->_collide = false;
			}

			autoNavigation();
			calculateCarMovement();
			applyCarMovement();

			break;

		default:
			break;
		}
	}
	
	traverse(node, nv);
}

void CarEvent::applyCarMovement()
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