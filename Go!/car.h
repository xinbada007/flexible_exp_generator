#pragma once
#include "eulerPoly.h"
#include "readConfig.h"
#include "obstacle.h"

#include <osg/BoundingBox>
#include <vector>

typedef std::vector<Plane*> quadList;
typedef std::vector<Solid*> solidList;
typedef std::vector<Obstacle*> obstacleList;

typedef struct CarState:public osg::Referenced
{
	CarState()
	{
		this->_O.set(O_POINT);
		this->_lastO.set(O_POINT);
		this->_O_Project.set(O_POINT);
		this->_direction.set(UP_DIR);
		this->_eyeDirection.set(_direction);
		this->_heading.set(_direction);
		this->_directionLastFrame.set(_direction);
		_angle = 0.0f;
		_lastAngle = _angle;
		_angle_incr = 0.2f;
		_swangle = 0.0f;
		_swDeDead = 0.0f;
		_speed = 0.0f;
		_speed_incr = 0.02f;
		_dither = 0.0f;
		_distancefromBase = 0.0f;
		_dynamic = true; //acceleration mode

		_collide = false;
		_crashPermit = true;
		_userHit = -1;
		_pointsEarned = 0;

		_updated = false;

		_frameStamp = 0;
		_timeReference = 0.0f;
		_startTime = INT_MAX;
		_lastTimeR = 0.0f;

		//mid-Line of current Road
		_midLine = new osg::Vec3dArray;

		//right-bottom first and them left-bottom
		_backWheel = new osg::Vec3dArray;
		//right-top first and then left top
		_frontWheel = new osg::Vec3dArray;

		//from right-bottom to left-bottom under counter-clockwise
		_carArray = new osg::Vec3dArray;
		_lastCarArray = new osg::Vec3dArray;
		
		_distancetoObsBody = new osg::DoubleArray;

		_OQuad = NULL;

		_reset = false;
		_steer = true;

		_D_Speed = _speed;
		_R_Speed = _speed;
		_D_Angle = _angle;
		_R_Angle = _angle;

		_state.makeIdentity();
		_moment.makeIdentity();

		_saveState = NULL;
		_dynamicState = NULL;
		_replay = false;

		_detailedDisplay = false;
	};
	osg::Vec3d _O;
	osg::Vec3d _O_Project;
	osg::Vec3d _shiftD;
	osg::Vec3d _heading;
	osg::Vec3d _eyeDirection;
	osg::Vec3d _direction;
	osg::Vec3d _directionLastFrame;
	osg::ref_ptr<osg::Vec3dArray> _midLine;
	osg::ref_ptr<osg::Vec3dArray> _backWheel;
	osg::ref_ptr<osg::Vec3dArray> _frontWheel;

	osg::ref_ptr<osg::Vec3dArray> _carArray;
	osg::ref_ptr<osg::Vec3dArray> _lastCarArray;
	
	quadList _lastQuad;
	quadList _currentQuad;
	quadList _collisionQuad;
	Plane::iterator _OQuad;

	bool _reset;
	bool _steer;

	double _angle;
	double _angle_incr;
	double _swangle;
	double _swDeDead;
	double _speed;
	double _speed_incr;	
	double _dither;
	double _distancefromBase;
	bool _dynamic;

	bool _collide;
	bool _crashPermit;
	int _userHit;
	unsigned _pointsEarned;

	bool _updated;

	unsigned _frameStamp;
	double _timeReference;
	double _startTime;

	osg::Matrix _state;
	osg::Matrix _moment;
	osg::ref_ptr<osg::MatrixdArray> _saveState;
	osg::ref_ptr<osg::IntArray> _dynamicState;
	bool _replay;

	bool _detailedDisplay;

	inline void cacluateSpeedandAngle() const
	{ 
		_D_Speed = _speed * 3.6f;
		_D_Angle = _angle / TO_RADDIAN;

		if (_lastTimeR != _timeReference)
		{
			_R_Speed = ((_O - _lastO).length());
			_R_Speed /= (_timeReference - _lastTimeR);
			_R_Speed *= 3.6f;

			_R_Angle = abs(_angle - _lastAngle);
			_R_Angle /= (_timeReference - _lastTimeR);
			_R_Angle /= TO_RADDIAN;

			_lastTimeR = _timeReference;
			_lastO = _O;
			_lastAngle = _angle;
		}
	};
	inline double getSpeed() const { return _D_Speed; };
	inline double getRSpeed() const { return _R_Speed; };
	inline double getAngle() const { return _D_Angle; };
	inline double getRAngle() const { return _R_Angle; };
	inline void updateLastO(osg::Vec3d ref) { _lastO = ref; };
	inline void setReplayText(std::string ref) { if (_replay) _replayText = ref; };
	inline const std::string & getReplayText() const { return _replayText; };
	inline void setObsList(obstacleList list){ _obsList = list; };
	inline obstacleList getObsList() const { return _obsList; };
	inline osg::ref_ptr<osg::DoubleArray> getDistancetoObsBody() const { return _distancetoObsBody; };
private:
	mutable double _D_Speed;
	mutable double _R_Speed;
	mutable double _D_Angle;
	mutable double _R_Angle;

	mutable osg::Vec3d _lastO;
	mutable double _lastAngle;

	mutable double _lastTimeR;

	std::string _replayText;

	obstacleList _obsList;
	osg::ref_ptr<osg::DoubleArray> _distancetoObsBody;
protected:
	~CarState()
	{ 
		std::cout << "Deconstruct CarState" << std::endl; 

		_midLine = NULL;
		_backWheel = NULL;
		_frontWheel = NULL;
		_carArray = NULL;
		_lastCarArray = NULL;
		_saveState = NULL;
		_dynamicState = NULL;
		_distancetoObsBody = NULL;
	};
}CarState;

class Car:public EulerPoly
{
public:
	Car();
	Car(const Car &copy, osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY);
	META_Node(Car, Car);
	void genCar(osg::ref_ptr<ReadConfig> refRC);
	Vehicle * getVehicle() const { return _vehicle.get(); };
	CarState * getCarState() const { return _carState.get(); };
private:
	osg::ref_ptr<Vehicle> _vehicle;
	osg::ref_ptr<CarState> _carState;
protected:
	virtual ~Car();
};

