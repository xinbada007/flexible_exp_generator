#include "stdafx.h"
#include "carEvent.h"
#include "math.h"

#include <osg/Notify>
#include <osgGA/EventVisitor>

CarEvent::CarEvent() :
_car(NULL), _carState(NULL), _vehicle(NULL), _mTransform(NULL), _leftTurn(false), _updated(false)
, _lastAngle(0.0f), _autoNavi(false), _shifted(false), _speedLock(false)
{
	_buttons = new osg::UIntArray;
	_buttons->assign(10, 0);
}

CarEvent::~CarEvent()
{
	_car = NULL;
	_carState = NULL;
	_vehicle = NULL;
	_mTransform = NULL;
	_buttons = NULL;

	std::cout << "Deconstruct CarEvent" << std::endl;
}

void CarEvent::checkRotationLimit()
{
	const double MAX_ROTATIONACCL = _vehicle->_rotationAccl;
	if (!MAX_ROTATIONACCL || _carState->_dynamic)
	{
		return;
	}
	double &angle = _carState->_angle;
	const double ROTATIONACCL = abs(angle - _lastAngle) * frameRate::instance()->getRealfRate();
	if (ROTATIONACCL > MAX_ROTATIONACCL)
	{
		if (angle * _lastAngle <= 0)
		{
			if (_lastAngle >= 0)
			{
				angle = _lastAngle - MAX_ROTATIONACCL / frameRate::instance()->getRealfRate();
				_leftTurn = (angle > 0);
			}
			if (_lastAngle < 0)
			{
				angle = _lastAngle + MAX_ROTATIONACCL / frameRate::instance()->getRealfRate();
				_leftTurn = (angle > 0);
			}
		}
		if (angle * _lastAngle > 0)
		{
			if (angle >= _lastAngle)
			{
				angle = _lastAngle + MAX_ROTATIONACCL / frameRate::instance()->getRealfRate();
			}
			else
			{
				angle = _lastAngle - MAX_ROTATIONACCL / frameRate::instance()->getRealfRate();
			}
		}
	}
	_lastAngle = angle;
}

void CarEvent::calculateCarMovement()
{
	//Limit the speed under MAX
	const double dr_rate = frameRate::instance()->getDesignfRate() / frameRate::instance()->getRealfRate();

	_carState->_speed = abs(_carState->_speed) > _vehicle->_speed ? 
		(_vehicle->_speed*(abs(_carState->_speed) / _carState->_speed)) : _carState->_speed;

	_carState->_angle = abs(_carState->_angle) > _vehicle->_rotate ? _vehicle->_rotate : _carState->_angle;

	//set rotation direction and limit
	_carState->_angle = abs(_carState->_angle) * (_leftTurn ? 1 : -1);
	_carState->_angle = (abs(_carState->_speed) == 0) ? 0.0f : _carState->_angle;

	//set the acceleration of rotation
	checkRotationLimit();

	//apply the rotation and the speed and direction
	_carState->_directionLastFrame = _carState->_direction;
	if (_carState->_dynamic || _autoNavi)
	{
		osg::Vec3d origin = (_leftTurn) ? _carState->_backWheel->front() : _carState->_backWheel->back();
		osg::Matrix rotation = osg::Matrix::translate(-origin);
		rotation *= osg::Matrix::rotate(_carState->_angle*(1.0f / frameRate::instance()->getRealfRate()), Z_AXIS);
		rotation *= osg::Matrix::translate(origin);

		_moment *= rotation;

		_carState->_direction = _carState->_direction * 
			osg::Matrix::rotate(_carState->_angle*(1.0f / frameRate::instance()->getRealfRate()), Z_AXIS);
	}
	else
	{
		osg::Vec3d expected = _carState->_heading*osg::Matrix::rotate(_carState->_angle, Z_AXIS);
		expected.normalize();
		osg::Quat qt;
		qt.makeRotate(_carState->_direction, expected);
		_carState->_direction = _carState->_direction * osg::Matrix::rotate(qt);
	}
	_carState->_direction.normalize();
//	_carState->_angle = 0.0f;

	_moment *= osg::Matrix::translate(_carState->_direction * _carState->_speed / frameRate::instance()->getRealfRate());

	//apply the shift vector
	_carState->_shiftD = (_carState->_speed == 0) ? osg::Vec3d(0.0f, 0.0f, 0.0f) : _carState->_shiftD;
	_moment *= osg::Matrix::translate(_carState->_shiftD);

	//apply reset matrix;
	if (!_reset.isIdentity())
	{
		_moment *= _reset;
		_carState->_direction = _carState->_direction * osg::Matrix::rotate(_reset.getRotate());
		_carState->_heading = _carState->_heading * osg::Matrix::rotate(_reset.getRotate());
	}
}

void CarEvent::autoNavigation()
{
	if (_carState->_dynamic || !(*_carState->_OQuad))
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
	const double COSANGLE = acosR(DOTPRODUCT);
	const double SINANGLE = asinR(CROSSPRODUCT);
	const bool SAMEDIRECTION = (DOTPRODUCT >= 0);
	const bool LEFT = (CROSSPRODUCT >= 0);
	const bool TURN = SAMEDIRECTION ? !LEFT : LEFT;
	double headingError = abs(SINANGLE) * abs(_carState->_speed);
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
		const double SINANGLE = asinR(CROSSPRODUCT);
		const double COSANGLE = acosR(DOTPRODUCT);
		const bool SAMEDIRECTION = (DOTPRODUCT >= 0);
		const bool LEFT = (CROSSPRODUCT >= 0);
		turnF = SAMEDIRECTION ? !LEFT : LEFT;
		const double ANGLE = abs(SINANGLE);
		if (ANGLE / TO_RADDIAN >= 1.0f)
		{
			angle_1d = ANGLE;
			angle_1d /= (roadDistance / (_carState->_speed));
			angle_1d = abs(angle_1d);
			break;
		}
		if (roadDistance >= abs(_carState->_speed))
		{
			break;
		}
	}

	const double ANGLE(angle_1d);

	const double RotationAngle = (ANGLE > headingError) ? ANGLE : headingError;
	const bool RotationTurn = (ANGLE > headingError) ? turnF : TURN;

	_leftTurn = RotationTurn;
	_carState->_angle = RotationAngle;
	_autoNavi = true;

	return;
}

void CarEvent::makeResetMatrix()
{
	if (_carState->_midLine->empty())
	{
		return;
	}

	double degree(0.0f);
	osg::Vec3d carD, roadD;
	carD = _carState->_heading;
	carD.normalize();
	roadD = (_carState->_midLine->front() - _carState->_midLine->back());
	roadD.normalize();
	osg::Quat qt;
	qt.makeRotate(carD, roadD);

	_reset.makeIdentity();
	osg::Vec3d MO = _carState->_O_Project;
	if (_vehicle->_resetMode != 0)
	{
		MO = _carState->_lastCarArray->back();
		const osg::Vec3d MD = (_carState->_O_Project - _carState->_lastCarArray->back());
		const double ratio = _vehicle->_width / MD.length();
		MO = MO * osg::Matrix::translate(MD*ratio);
	}

	_reset *= osg::Matrix::translate(MO - _carState->_O);
	_reset *= osg::Matrix::translate(-MO);
	_reset *= osg::Matrix::rotate(qt);
	_reset *= osg::Matrix::translate(MO);
}

void CarEvent::shiftVehicle()
{
	if (_carState->_dynamic || !_shifted)
	{
		_carState->_shiftD = osg::Vec3d(0.0f, 0.0f, 0.0f);
		return;
	}

	_carState->_shiftD.x() = _carState->_heading.y();
	_carState->_shiftD.y() = -_carState->_heading.x();
	_carState->_shiftD.z() = _carState->_heading.z();
	_carState->_shiftD.normalize();

	osg::Vec3d origin = (_leftTurn) ? _carState->_backWheel->front() : _carState->_backWheel->back();
	osg::Vec3d r = origin - _carState->_O;

	double R = r.length();
	R *= sin(abs(_carState->_angle) * 1.0f / frameRate::instance()->getRealfRate());
	R *= _vehicle->_dynamicSensitive;

	_carState->_shiftD *= R;
	_carState->_shiftD = (_leftTurn) ? -_carState->_shiftD : _carState->_shiftD;

	_shifted = false;
}

bool CarEvent::Joystick()
{
	extern bool poll_joystick(int &x, int &y, int &b);
	int x(0), y(0), b(-1);
	if (!poll_joystick(x, y, b))
	{
		return false;
	}

	const double MAX(32767.0f);
	const double MAX_ANGLE = 120.0f;
	const double DEAD = 0.075f;
	const int DeadZone(MAX*DEAD);
	_carState->_swangle = (x / MAX)*MAX_ANGLE;

	if ((abs(x)) > DeadZone)
	{
		_carState->_swDeDead = (x / MAX)*MAX_ANGLE;

		_carState->_angle = _vehicle->_rotate * (double(abs(x)) / MAX);
		_leftTurn = (x < 0);
		_shifted = true;
	}
	else if ((abs(x)) <= DeadZone)
	{
		_carState->_swDeDead = 0.0f;

		_carState->_angle = 0.0f;
		_shifted = false;
	}

	if (!_speedLock)
	{
		if (abs(y) > DeadZone)
		{
			_carState->_speed = _vehicle->_speed * (double(abs(y)) / MAX) * (y > 0 ? -1 : 1);
		}
		else if (abs(y) <= DeadZone)
		{
			_carState->_speed = 0.0f;
		}
	}

	if (b >= 0 && b < _buttons->size())
	{
		_buttons->at(b) = 1;

		//Record User Hit
		if (b == 0)
		{
			_carState->_userHit = 0;
		}
	}
	else if (b == -1)
	{
		if (_buttons->at(0) == 1)
		{
			//Reset UserHit
			_carState->_userHit = -1;
		}
		else if (_buttons->at(1) == 1)
		{
			if (_carState->_O == _vehicle->_O)
			{
				_carState->_speed = _vehicle->_speed;
				_speedLock = !_speedLock;
			}
		}
		else if (_buttons->at(6) == 1)
		{
			if (_carState->_O != _vehicle->_O && _vehicle->_carReset == 2)
			{
				_carState->_reset = true;
			}
		}
		_buttons->assign(_buttons->size(), 0);
	}

	return true;
}

void CarEvent::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
	if (!_updated)
	{
		osg::MatrixTransform *refM = dynamic_cast<osg::MatrixTransform*> (node);
		Car *refC = (refM) ? dynamic_cast<Car*>(refM->getUserData()) : NULL;

		if (refC)
		{
			_car = refC;
			_carState = refC->getCarState();
			_vehicle = refC->getVehicle();
			_mTransform = refM;
			_updated = true;
		}
	}
	
	osgGA::EventVisitor *ev = dynamic_cast<osgGA::EventVisitor*>(nv);
	osgGA::EventQueue::Events events = (ev) ? ev->getEvents() : events;
	osgGA::GUIEventAdapter *ea = (!events.empty()) ? events.front() : NULL;

	if (_carState && ea)
	{
		Joystick();

		const int &key = ea->getKey();
		switch (ea->getEventType())
		{
		case osgGA::GUIEventAdapter::KEYDOWN:
			if ((key == osgGA::GUIEventAdapter::KEY_A || key == osgGA::GUIEventAdapter::KEY_D))
			{
				int sign = (key == osgGA::GUIEventAdapter::KEY_A) ? 1 : -1;
				_carState->_angle += _vehicle->_rotate*sign*_carState->_angle_incr;
				_leftTurn = (sign == 1);
				_shifted = true;
				break;
			}
			else if (key == osgGA::GUIEventAdapter::KEY_W || key == osgGA::GUIEventAdapter::KEY_S)
			{
				double sign = (key == osgGA::GUIEventAdapter::KEY_W) ? 1 : -1;
//				_carState->_speed += _vehicle->_speed*sign*_carState->_speed_incr;
				sign *= sign > 0 ? 1 : 2.5;
				_carState->_speed += sign*_vehicle->_speedincr;
				break;
			}
			else if (key == osgGA::GUIEventAdapter::KEY_R)
			{
				if (_vehicle->_carReset == 2)
				{
					_carState->_reset = true;
				}
				break;
			}
			else if ((key == '`'))
			{
				_carState->_dynamic = !_carState->_dynamic;
				break;
			}
			else if (key == osgGA::GUIEventAdapter::KEY_Equals)
			{
				_vehicle->increaseMaxSpeed();
				break;
			}
			else if (key == osgGA::GUIEventAdapter::KEY_Minus)
			{
				_vehicle->decreaseMaxSpeed();
				break;
			}
			else if (key == osgGA::GUIEventAdapter::KEY_H)
			{
				_carState->_detailedDisplay = !_carState->_detailedDisplay;
				break;
			}
		case osgGA::GUIEventAdapter::KEYUP:
			if (key == 'a' || key == 'd')
			{
				_carState->_angle = 0.0f;
				_shifted = false;
				break;
			}
		case osgGA::GUIEventAdapter::FRAME:

			if (_carState->_collide)
			{
				const double MAXSPEED(1.0f/3.6f);
				int sign = (_carState->_speed > 0) ? -1 : 1;
				_carState->_speed = (abs(_carState->_speed) > MAXSPEED) ? MAXSPEED*sign : _carState->_speed;
				_carState->_collide = false;
			}
			else
			{
				if (_speedLock)
				{
					_carState->_speed = _vehicle->_speed;
				}
			}
			if (_carState->_reset && _vehicle->_carReset)
			{
				makeResetMatrix();
				_carState->_reset = false;
			}

			if (!_reset.isIdentity())
			{
				_carState->_speed = 0.0f;
			}

			//1st
			shiftVehicle();
			//2nd
			autoNavigation();
			//3rd
			calculateCarMovement();
			//always final
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
	_carState->_state *= _moment;
	_carState->_moment = _moment;
	_carState->_O = _carState->_O * _moment;
	arrayByMatrix(_carState->_backWheel, _moment);
	arrayByMatrix(_carState->_frontWheel, _moment);
	arrayByMatrix(_carState->_carArray, _moment);

	_mTransform->setMatrix(_carState->_state);
	if (_carState->_state != osg::Matrix::identity())
	{
		dirtyVisitor dv;
		_mTransform->accept(dv);
	}

	//Initialize
	_moment.makeIdentity();
	_reset.makeIdentity();

	//update heading
	osg::Vec3d hD = (_carState->_frontWheel->front() + _carState->_frontWheel->back()) * 0.5f;
	hD -= _carState->_O;
	hD.normalize();
	_carState->_heading = hD;

	_car->absoluteTerritory.center = _carState->_O;

	//set carstate
//	_carState->cacluateSpeedandAngle();

	//set carstate
	if (*_carState->_OQuad)
	{
		osg::ref_ptr<osg::Vec3dArray> navigationEdge = (*_carState->_OQuad)->getLoop()->getNavigationEdge();
		const osg::Vec3d naviEdge = navigationEdge->back() - navigationEdge->front();
		const osg::Vec3d O = _carState->_O;
		const double lamida = ((O - navigationEdge->front()) * naviEdge) / naviEdge.length2();
		const osg::Vec3d N = navigationEdge->back()*lamida + navigationEdge->front()*(1 - lamida);
		const double dis = (N - O).length();
		_carState->_O_Project = N;
		const osg::Vec3d midL = -naviEdge;
		const osg::Vec3d disL = O - N;
		const int sign = (disL^midL).z() > 0 ? 1 : -1;
		const double distherwithsign = dis*sign;
		
		_carState->_distancefromBase = distherwithsign - _vehicle->_baseline;
		_carState->_dither = distherwithsign;

		if (!_carState->_midLine->size())
		{
			copy(navigationEdge->begin(), navigationEdge->end(), std::back_inserter(*_carState->_midLine));
		}
		else
		{
			copy(navigationEdge->begin(), navigationEdge->end(), _carState->_midLine->begin());
		}		

		_carState->_updated = true;
	}
	quadList::const_iterator quadI = _carState->_currentQuad.cbegin();
	unsigned notFound(0);
	while (quadI != _carState->_currentQuad.cend())
	{
		if (!(*quadI))
		{
			++notFound;
			break;
		}
		++quadI;
	}
	if (!notFound)
	{
		copy(_carState->_carArray->begin(),_carState->_carArray->end(),_carState->_lastCarArray->begin());
	}

	//isNAN Test
	bool NANTEST(true);
	osg::Vec3dArray *testNAN = _carState->_backWheel;
	osg::Vec3dArray::const_iterator i = testNAN->begin();
	while (i != testNAN->end())
	{
		NANTEST = NANTEST && (*i).isNaN();
		i++;
	}

	testNAN = _carState->_carArray;
	i = testNAN->begin();
	while (i != testNAN->end())
	{
		NANTEST = NANTEST && (*i).isNaN();
		i++;
	}

	testNAN = _carState->_frontWheel;
	i = testNAN->begin();
	while (i != testNAN->end())
	{
		NANTEST = NANTEST && (*i).isNaN();
		i++;
	}

	testNAN = _carState->_midLine;
	i = testNAN->begin();
	while (i != testNAN->end())
	{
		NANTEST = NANTEST && (*i).isNaN();
		i++;
	}

	NANTEST = NANTEST && _carState->_heading.isNaN();
	NANTEST = NANTEST && _carState->_direction.isNaN();
	NANTEST = NANTEST && _carState->_directionLastFrame.isNaN();
	NANTEST = NANTEST && _carState->_moment.isNaN();
	NANTEST = NANTEST && _carState->_O.isNaN();
	NANTEST = NANTEST && _carState->_O_Project.isNaN();
	NANTEST = NANTEST && _carState->_heading.isNaN();
	NANTEST = NANTEST && _carState->_state.isNaN();

	if (NANTEST)
	{
		osg::notify(osg::WARN) << "NAN FOUND!!!" << std::endl;
	}
}

void dirtyVisitor::apply(osg::Geode &geo)
{
	const osg::Geode::DrawableList &drawList = geo.getDrawableList();
	osg::Geode::DrawableList::const_iterator i = drawList.cbegin();
	while (i != drawList.cend())
	{
		(*i)->dirtyBound();
		(*i)->dirtyDisplayList();
		++i;
	}
}