#pragma once
#include "eulerPoly.h"
#include "readConfig.h"
#include <osg/BoundingBox>

#include <vector>

typedef std::vector<Plane*> quadList;
typedef std::vector<Solid*> solidList;

typedef struct CarState:public osg::Referenced
{
	CarState()
	{
		this->_O.set(O_POINT);
		this->_O_Project.set(O_POINT);
		this->_direction.set(UP_DIR);
		this->_directionLastFrame.set(_direction);
		_angle = 0.0f;
		_angle_incr = 0.2f;
		_swangle = 0.0f;
		_speed = 0.0f;
		_speed_incr = 0.2f;
		_dither = 0.0f;
		_dynamic = true; //acceleration mode

		_collide = false;
		_updated = false;

		_frameStamp = 0;
		_timeReference = 0.0f;

		//mid-Line of current Road
		_midLine = new osg::Vec3dArray;

		//right-bottom first and them left-bottom
		_backWheel = new osg::Vec3dArray;
		//right-top first and then left top
		_frontWheel = new osg::Vec3dArray;

		//from right-bottom to left-bottom under counter-clockwise
		_carArray = new osg::Vec3dArray;

		_OQuad = NULL;

		_D_Speed = _speed;
		_D_Angle = _angle;

		_state.makeIdentity();
		_moment.makeIdentity();

		_saveState = NULL;
		_dynamicState = NULL;
		_replay = false;
	};
	osg::Vec3d _O;
	osg::Vec3d _O_Project;
	osg::Vec3d _shiftD;
	osg::Vec3d _direction;
	osg::Vec3d _directionLastFrame;
	osg::ref_ptr<osg::Vec3dArray> _midLine;
	osg::ref_ptr<osg::Vec3dArray> _backWheel;
	osg::ref_ptr<osg::Vec3dArray> _frontWheel;

	osg::ref_ptr<osg::Vec3dArray> _carArray;
	
	quadList _lastQuad;
	quadList _currentQuad;
	quadList _collisionQuad;
	Plane::iterator _OQuad;

	double _angle;
	double _angle_incr;
	double _swangle;
	double _speed;
	double _speed_incr;	
	double _dither;
	bool _dynamic;

	bool _collide;
	bool _updated;

	unsigned _frameStamp;
	double _timeReference;

	osg::Matrix _state;
	osg::Matrix _moment;
	osg::ref_ptr<osg::MatrixdArray> _saveState;
	osg::ref_ptr<osg::IntArray> _dynamicState;
	bool _replay;

	void convertSpeed(){ _D_Speed = _speed * frameRate *3.6f; };
	void convertAngle(){ _D_Angle = _angle * frameRate / TO_RADDIAN; };
	double getSpeed() const { return _D_Speed; };
	double getAngle() const { return _D_Angle; };
	void setReplayText(std::string ref) { if (_replay) _replayText = ref; };
	const std::string & getReplayText() const { return _replayText; };
	~CarState(){};
private:
	double _D_Speed;
	double _D_Angle;
	std::string _replayText;
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
	virtual ~Car();
	osg::ref_ptr<Vehicle> _vehicle;
	osg::ref_ptr<CarState> _carState;
};

