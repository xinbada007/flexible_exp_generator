#include "stdafx.h"
#include "carEvent.h"
#include "math.h"
#include "edge.h"
#include "halfedge.h"
#include "points.h"

#include <osg/Notify>
#include <osgGA/EventVisitor>

#include <assert.h>

CarEvent::CarEvent() :
_car(NULL), _carState(NULL), _vehicle(NULL), _mTransform(NULL), _leftTurn(false), _updated(false)
, _lastAngle(0.0f), _autoNavi(false), _shifted(false), _lockedSpeed(false),_lockedSW(false), _speedSign(1)
{
	_buttons = new osg::UIntArray;
	_buttons->assign(20, 0);
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

void CarEvent::dealCollision()
{
	if (_carState->_collide && _carState->_crashPermit)
	{
		const obstacleList &obsList = _carState->getObsList();
		const quadList &wallList = _carState->_collisionQuad;
		obstacleList::const_iterator obsIndex = obsList.cbegin();
		quadList::const_iterator wallIndex = wallList.cbegin();

		osg::Vec3d carO = _carState->_O;
		carO.z() = 0.0f;
		while (obsIndex != obsList.cend())
		{
			osg::Vec3d obsO = (*obsIndex)->absoluteTerritory.center;
			obsO.z() = 0.0f;

			const osg::Vec3d cartoObs = obsO - carO;
			const osg::Vec3d &carDirection = _carState->_direction;
			if (carDirection*cartoObs > 0.0f)
			{
				_carState->_speed = (_carState->_speed > 0.0f) ? 0.0f : _carState->_speed;
			}
			else
			{
				_carState->_speed = (_carState->_speed < 0.0f) ? 0.0f : _carState->_speed;
			}
			++obsIndex;
		}
		while (wallIndex != wallList.cend())
		{
			const edgelist &list = (*wallIndex)->getLoop()->getFlagEdge();
			if (!list.empty())
			{
				edgelist::const_iterator i = list.cbegin();
				edgelist::const_iterator listEND = list.cend();
				while (i != listEND)
				{
					if ((*i)->getEdgeFlag()->_collsionEdge)
					{
 						const osg::Vec3d &a = (*i)->getHE1()->getPoint()->getPoint();
 						const osg::Vec3d &b = (*i)->getHE2()->getPoint()->getPoint();
						osg::Vec3d wallO = (a + b)*0.5f;
						wallO.z() = 0.0f;

						const osg::Vec3d cartoWall = wallO - carO;
						const osg::Vec3d &carDirection = _carState->_direction;
						if (carDirection*cartoWall > 0.0f)
						{
							_carState->_speed = (_carState->_speed > 0.0f) ? 0.0f : _carState->_speed;
						}
						else
						{
							_carState->_speed = (_carState->_speed < 0.0f) ? 0.0f : _carState->_speed;
						}
					}
					++i;
				}
			}
			++wallIndex;
		}
	}
}

void CarEvent::calculateCarMovement()
{
	//Limit the speed under MAX
	const double dr_rate = frameRate::instance()->getDesignfRate() / frameRate::instance()->getRealfRate();

	{
		//set rotation direction and limit
		_carState->_angle = abs(_carState->_angle) * (_leftTurn ? 1 : -1);
		if (_carState->_speed < 0) _carState->_angle = -_carState->_angle;
	}

	//set the acceleration of rotation
	checkRotationLimit();

	//apply the rotation and the speed and direction
	_carState->_directionLastFrame = _carState->_direction;
	if (_carState->_dynamic || _autoNavi)
	{
		osg::Vec3d &origin = _carState->_turningCenter;
		const double &radius = _carState->_turningRadius;
		const double perimeter = 2 * PI * radius;
		double ratio = 0.0f;
		if (radius > 0.0f)
		{
			ratio = abs((_carState->_speed / radius));
			ratio = (_carState->_speed > 0.0f) ? (_leftTurn) ? ratio : -ratio : (_leftTurn) ? ratio: -ratio;
		}
		const osg::Matrix circle = osg::Matrix::rotate(ratio / frameRate::instance()->getRealfRate(), Z_AXIS);
		
		osg::Matrix rotation = osg::Matrix::translate(-origin);
		rotation *= circle;
		rotation *= osg::Matrix::translate(origin);

		_moment *= rotation;

		_carState->_direction = _carState->_direction * circle;
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
	if (!_carState->_angle)
	{
		_moment *= osg::Matrix::translate(_carState->_direction * _carState->_speed / frameRate::instance()->getRealfRate());
	}

	//apply the shift vector
	_carState->_shiftD = (_carState->_speed == 0) ? osg::Vec3d(0.0f, 0.0f, 0.0f) : _carState->_shiftD;
	_moment *= osg::Matrix::translate(_carState->_shiftD);

// 	//apply reset matrix;
// 	if (!_reset.isIdentity())
// 	{
// 		_moment *= _reset;
// 		_carState->_direction = _carState->_direction * osg::Matrix::rotate(_reset.getRotate());
// 		_carState->_heading = _carState->_heading * osg::Matrix::rotate(_reset.getRotate());
// 	}

	//applay ForceReset matrix
	if (!_carState->_forceReset.isIdentity())
	{
		_moment *= _carState->_forceReset;
		_carState->_direction = _carState->_direction * osg::Matrix::rotate(_carState->_forceReset.getRotate());
		_carState->_heading = _carState->_heading * osg::Matrix::rotate(_carState->_forceReset.getRotate());
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
	_carState->_angle = RotationAngle > eps_1000 ? RotationAngle : 0.0f;
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
	if (_vehicle->_resetMode == 1)
	{
		MO = _carState->_lastCarArray->back();
		const osg::Vec3d MD = (_carState->_O_Project - MO);
		const double &length = MD.length();
		if (length)
		{
			const double ratio = _vehicle->_width / MD.length();
			MO = MO * osg::Matrix::translate(MD*ratio);
		}
	}
	else if (_vehicle->_resetMode == 2)
	{
		MO = _carState->_O;
		MO = MO * (osg::Matrix::inverse(_carState->_state) * _vehicle->_initialState);
		qt = (osg::Matrix::inverse(_carState->_state) * _vehicle->_initialState).getRotate();
	}

	_reset *= osg::Matrix::translate(MO - _carState->_O);
	_reset *= osg::Matrix::translate(-MO);
	_reset *= osg::Matrix::rotate(qt);
	_reset *= osg::Matrix::translate(MO);

	_carState->_forceReset = _reset;
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

// 	osg::Vec3d origin = (_leftTurn) ? _carState->_backWheel->front() : _carState->_backWheel->back();
// 	osg::Vec3d r = origin - _carState->_O;
// 	double test = r.length();

	double R = abs(_vehicle->_wheelBase / sin(_vehicle->_rotate));
	R *= sin(abs(_carState->_angle) / frameRate::instance()->getRealfRate());
	R *= _vehicle->_dynamicSensitive;
// 	std::cout << "Shift Speed:\t" << R << std::endl;

	_carState->_shiftD *= R;
	_carState->_shiftD = (_leftTurn) ? -_carState->_shiftD : _carState->_shiftD;

	_shifted = false;
}

bool CarEvent::Joystick()
{
	static const bool &steering = _carState->_steer;

	extern bool poll_joystick(int &x, int &y, int &b);
	int x(0), y(0), b(-1);
	if (!poll_joystick(x, y, b))
	{
		return false;
	}

	const double MAX(32767.0f);
	const double MAX_ANGLE = 120.0f;
	_carState->_swangle = (x / MAX)*MAX_ANGLE;
	const double &DEAD = _vehicle->_deadband;
	const int DeadZone(MAX*DEAD);

	if ((abs(x)) > DeadZone && !_lockedSW)
	{
		_carState->_swDeDead = (x / MAX)*MAX_ANGLE;

		_carState->_angle = _vehicle->_rotate * (double(abs(x)) / MAX);
		_leftTurn = (_carState->_speed >= 0) ? (x < 0) : (x > 0);
		_shifted = true;
	}
	else if ((abs(x)) <= DeadZone && !_lockedSW)
	{
		_carState->_swDeDead = 0.0f;

		_carState->_angle = 0.0f;
		_shifted = false;
	}

	if (!_lockedSpeed)
	{
		if (abs(y) > DeadZone)
		{
			if (y < 0)
			{
				_carState->_speed = _vehicle->_speed * _speedSign;
				if (_carState->_insertTrigger)
				{
					_carState->_startTime = _carState->_timeReference;
					_carState->_insertTrigger = false;
				}
			}
			else
			{
				_carState->_speed = 0.0f;
			}
		}
		else
		{
			_carState->_speed = 0.0f;
		}
	}

	if (b >= 0 && b < _buttons->size())
	{
		_buttons->at(b) = 1;

		//Record User Hit
		_carState->_userHit = b;
	}
	else if (b == -1)
	{
		//Reset User Hit
		_carState->_userHit = -1;

		if (_buttons->at(1) == 1 && !_vehicle->_disabledButton->at(1))
		{
			if (_carState->_insertTrigger)
			{
				_carState->_startTime = _carState->_timeReference;
				_carState->_insertTrigger = false;
			}

			{
				_carState->_speed = _vehicle->_speed;
			}
		}
// 		else if (_buttons->at(0) == 1 && !_vehicle->_disabledButton->at(0))
// 		{
// 			if (_carState->_O != _vehicle->_O && _vehicle->_carReset == Vehicle::VEHICLE_RESET_TYPE::MANUAL)
// 			{
// 				_carState->_reset = true;
// 			}
// 		}
		else if (_buttons->at(8) == 1 && !_vehicle->_disabledButton->at(8))
		{
			if (!_carState->_speed)
			{
				_speedSign = -1;
			}
		}
		else if (_buttons->at(9) == 1 && !_vehicle->_disabledButton->at(9))
		{
			if (!_carState->_speed)
			{
				_speedSign = 1;
			}
		}
		
		_buttons->assign(_buttons->size(), 0);
	}

	return true;
}

void CarEvent::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
//	osg::notify(osg::NOTICE) << "CarEvent..Begins..." << std::endl;

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
	osgGA::GUIEventAdapter *ea = (!events.empty()) ? dynamic_cast<osgGA::GUIEventAdapter*>(events.front().get()) : NULL;

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
				_carState->_angle = abs(_carState->_angle);

				_leftTurn = (_carState->_speed >= 0) ? (sign == 1) : (sign == -1);
				_shifted = true;
				break;
			}
			else if (key == osgGA::GUIEventAdapter::KEY_W || key == osgGA::GUIEventAdapter::KEY_S)
			{
				double sign = (key == osgGA::GUIEventAdapter::KEY_W) ? 1 : -1;
				sign *= sign > 0 ? 1 : 2.5;
				_carState->_speed += sign*_vehicle->_speedincr;
				break;
			}
			else if (key == osgGA::GUIEventAdapter::KEY_R)
			{
				if (_vehicle->_carReset == Vehicle::VEHICLE_RESET_TYPE::MANUAL)
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
				++_carState->_detailedDisplay;
				_carState->_detailedDisplay %= 3;
				break;
			}
		case osgGA::GUIEventAdapter::KEYUP:
			if (key == osgGA::GUIEventAdapter::KEY_A || key == osgGA::GUIEventAdapter::KEY_D)
			{
				_carState->_angle = 0.0f;
				_shifted = false;
				break;
			}
		case osgGA::GUIEventAdapter::FRAME:
			//DEBUG
// 			_carState->_speed = _vehicle->_speed;
// 			_carState->_angle = _vehicle->_rotate * 0.2;
			//DEBUG

			if (_carState->_reset)
			{
				makeResetMatrix();
				_carState->_reset = false;
			}
			if (!_reset.isIdentity())
			{
				_carState->_speed = 0.0f;
			}

			carController();

			//initial check if over speed
			_carState->_speed = abs(_carState->_speed) > abs(_vehicle->_speed) ?
				(_vehicle->_speed*(_carState->_speed >= 0 ? 1 : -1)) : _carState->_speed;

			//1st
			shiftVehicle();
			//2nd
			autoNavigation();
			//3rd
			dealCollision();
			//4th calculate turning radius and turning center
			getTurningFactor();
			//5th
			calculateCarMovement();

			//always final
			applyCarMovement();

			break;

		default:
			break;
		}
	}
//	osg::notify(osg::NOTICE) << "CarEvent..Traverse..." << std::endl;
	traverse(node, nv);
//	osg::notify(osg::NOTICE) << "CarEvent..END..." << std::endl;
}

void CarEvent::carController()
{
	if (!_carState || !_vehicle)
	{
		return;
	}

	if (abs(_carState->_locked_angle) <= _vehicle->_rotate)
	{
		_carState->_angle = _carState->_locked_angle;
		_shifted = _carState->_angle == 0 ? true : false;
		_leftTurn = (_carState->_angle > 0) ? true : false;
		_leftTurn &= (_carState->_speed >= 0) ? true : false;

		_lockedSW = 1;
	}
	if (abs(_carState->_locked_speed) <= _vehicle->_speed)
	{
		_carState->_speed = _carState->_locked_speed;

		_lockedSpeed = 1;
	}
}

void CarEvent::getTurningFactor()
{
	_carState->_angle = abs(_carState->_angle);
	_carState->_angle = _carState->_angle > _vehicle->_rotate ? _vehicle->_rotate : _carState->_angle;
	_carState->_angle = _carState->_speed == 0 ? 0.0f : _carState->_angle;

	const double sinTheta = sin(_carState->_angle);
	if (!sinTheta)
	{
		_carState->_turningRadius = -2.0f;
		_carState->_turningCenter.set(-2.0f, -2.0f, -2.0f);
		return;
	}

	_carState->_turningRadius = _vehicle->_wheelBase / sinTheta;

	const osg::Vec3d &RIGHT_TOP = _carState->_frontWheel->front();
	const osg::Vec3d &LEFT_TOP = _carState->_frontWheel->back();
	const osg::Vec3d &RIGHT_BOTTOM = _carState->_backWheel->front();
	const osg::Vec3d &LEFT_BOTTOM = _carState->_backWheel->back();
	bool ISLEFT = (_carState->_speed >= 0) ? _leftTurn : !_leftTurn;
	const osg::Vec3d BASEPOS = (ISLEFT) ? RIGHT_TOP : LEFT_TOP;
	osg::Vec3d WHEELBASE = (ISLEFT) ? RIGHT_TOP - RIGHT_BOTTOM : LEFT_TOP - LEFT_BOTTOM;
	const double &Xm = WHEELBASE.x(); const double &Ym = WHEELBASE.y();
	const double WHEELBASE_LENGTH2 = WHEELBASE.length2();
	assert(WHEELBASE_LENGTH2);

	const double A = _carState->_turningRadius*WHEELBASE.length()*(-sinTheta);
	const double &B = _carState->_turningRadius;

	const double a = (WHEELBASE_LENGTH2);
	const double b = - (2 * A * Ym);
	const double c = A*A - B*B*Xm*Xm;
	double delta = b * b - 4 * a * c;
	if (isEqual(delta , 0.0f))
	{
		delta = 0.0f;
	}

	double Y = -b + sqrt(delta);
	Y /= 2 * a;
	double X;
	if (!isEqual(Xm, 0.0f))
	{
		X = (A - Y * Ym) / Xm;
	}

	double Y1 = -b - sqrt(delta);
	Y1 /= 2 * a;
	double X1;
	if (!isEqual(Xm, 0.0f))
	{
		X1 = (A - Y1 * Ym) / Xm;
	}
	else
	{
		X1 = sqrt(B*B - Y*Y1);
		X = -X1;
	}

	const osg::Vec3d R1(X, Y, 0.0f);
	const osg::Vec3d R2(X1, Y1, 0.0f);
	osg::Vec3d realVec;
	if (ISLEFT)
	{
		realVec = (WHEELBASE^R1).z() >= 0 ? R1 : R2;
	}
	else
	{
		realVec = (WHEELBASE^R1).z() <= 0 ? R1 : R2;
	}

	_carState->_turningCenter = BASEPOS + realVec;

// 	osg::notify(osg::NOTICE) << "BASE\t" << BASEPOS.x() << "\t" << BASEPOS.y() << std::endl;
// 	osg::notify(osg::NOTICE) << "VEC\t" << realVec.x() << "\t" << realVec.y() << std::endl;
// 	osg::notify(osg::NOTICE) << "CENTER\t" << _carState->_turningCenter.x() << "\t" << _carState->_turningCenter.y() << std::endl;

	double theta = acosR((realVec*WHEELBASE) / (realVec.length()*WHEELBASE.length()));
	theta -= PI*0.5f;
	theta -= abs(_carState->_angle);
	if (!isEqual(theta,0.0f,eps_100))
	{
		osg::notify(osg::WARN) << "Error decteced in Turning Center\n";
		osg::notify(osg::WARN) << "theta:\t" << theta / TO_RADDIAN << std::endl;
	}
	bool DIR = (ISLEFT) ? (WHEELBASE^realVec).z() > 0 : (WHEELBASE^realVec).z() < 0;
	if (!DIR)
	{
		std::string temp = (ISLEFT) ? "LEFT" : "RIGHT";
		osg::notify(osg::WARN) << "Error decteced in Turning Center\n";
		osg::notify(osg::WARN) << temp << "\t" << (WHEELBASE^realVec).z() << std::endl;
	}
	
// 	osg::notify(osg::NOTICE) << "RESULT\t" << theta - abs(_carState->_angle) / TO_RADDIAN << std::endl;
// 	osg::notify(osg::NOTICE) << "DIR\t" << (WHEELBASE^realVec).z() << std::endl;
// 	osg::notify(osg::NOTICE) << "RADIUS\t" << _carState->_turningRadius << std::endl;
}

void CarEvent::applyCarMovement()
{
	_carState->_state *= _moment;
	_carState->_moment = _moment;
	_carState->_forceReset.makeIdentity();
	_carState->_O = _carState->_O * _moment;
	arrayByMatrix(_carState->_backWheel, _moment);
	arrayByMatrix(_carState->_frontWheel, _moment);
	arrayByMatrix(_carState->_carArray, _moment);

	_mTransform->setMatrix(_carState->_state);

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

	osg::Vec3d carD = _carState->_direction;
	carD.normalize();
	if (!_carState->_lastQuad.empty())
	{
		osg::ref_ptr<osg::Vec3dArray> navigationEdge = _carState->_lastQuad.back()->getLoop()->getNavigationEdge();
		osg::Vec3d naviEdge = navigationEdge->front() - navigationEdge->back();
		naviEdge.normalize();
		const osg::Vec3d cross = naviEdge^carD;
		const double theta = (acosR(naviEdge*carD) / TO_RADDIAN) * ((cross.z() >= 0) ? 1.0f : -1.0f);
		_carState->_anglefromRoad = theta;
	}

	//isNAN Test
// 	bool NANTEST(true);
// 	osg::Vec3dArray *testNAN = _carState->_backWheel;
// 	osg::Vec3dArray::const_iterator i = testNAN->begin();
// 	while (i != testNAN->end())
// 	{
// 		NANTEST = NANTEST && (*i).isNaN();
// 		i++;
// 	}
// 
// 	testNAN = _carState->_carArray;
// 	i = testNAN->begin();
// 	while (i != testNAN->end())
// 	{
// 		NANTEST = NANTEST && (*i).isNaN();
// 		i++;
// 	}
// 
// 	testNAN = _carState->_frontWheel;
// 	i = testNAN->begin();
// 	while (i != testNAN->end())
// 	{
// 		NANTEST = NANTEST && (*i).isNaN();
// 		i++;
// 	}
// 
// 	testNAN = _carState->_midLine;
// 	i = testNAN->begin();
// 	while (i != testNAN->end())
// 	{
// 		NANTEST = NANTEST && (*i).isNaN();
// 		i++;
// 	}
// 
// 	NANTEST = NANTEST && _carState->_heading.isNaN();
// 	NANTEST = NANTEST && _carState->_direction.isNaN();
// 	NANTEST = NANTEST && _carState->_directionLastFrame.isNaN();
// 	NANTEST = NANTEST && _carState->_moment.isNaN();
// 	NANTEST = NANTEST && _carState->_O.isNaN();
// 	NANTEST = NANTEST && _carState->_O_Project.isNaN();
// 	NANTEST = NANTEST && _carState->_heading.isNaN();
// 	NANTEST = NANTEST && _carState->_state.isNaN();
// 
// 	if (NANTEST)
// 	{
// 		osg::notify(osg::WARN) << "NAN FOUND!!!" << std::endl;
// 	}
}

void dirtyVisitor::apply(osg::Geode &geo)
{
// 	const osg::Geode::DrawableList &drawList = geo.getDrawableList();
// 	osg::Geode::DrawableList::const_iterator i = drawList.cbegin();
// 	while (i != drawList.cend())
// 	{
// 		(*i)->dirtyBound();
// 		(*i)->dirtyDisplayList();
// 		++i;
// 	}

	const unsigned &num = geo.getNumDrawables();
	for (unsigned i = 0; i < num; i++)
	{
		geo.getDrawable(i)->dirtyBound();
		geo.getDrawable(i)->dirtyDisplayList();
	}
}
