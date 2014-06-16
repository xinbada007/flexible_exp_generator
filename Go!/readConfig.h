#pragma once
#include <string>
#include <vector>
#include <iostream>

#include <osg/Array>
#include <osg/Geode>
#include <osg/Image>

#include "Nurbs.h"
#include "math.h"

typedef std::vector<std::string> stringList;

typedef struct Experiment :public osg::Referenced
{
	Experiment()
	{
		_textTime = new osg::UIntArray;
		_textPeriod = new osg::DoubleArray;
		_dynamicChange = new osg::UIntArray;
	};

	osg::ref_ptr <osg::UIntArray> _textTime;
	osg::ref_ptr <osg::DoubleArray> _textPeriod;
	stringList _textContent;
	osg::ref_ptr <osg::UIntArray> _dynamicChange;
	unsigned _dynamicChangeCondition;
	unsigned _dynamicChangeLasting;

protected:
	virtual ~Experiment(){ std::cout << "Deconstruct Experiment" << std::endl; };
}Experiment;

typedef struct Nurbs:public osg::Referenced
{
	Nurbs()
	{
		_path = new osg::Vec3dArray;
		_path_left = new osg::Vec3dArray;
		_path_right = new osg::Vec3dArray;

		_ctrl_left = new osg::Vec3dArray;
		_ctrl_right = new osg::Vec3dArray;
		_ctrlPoints = new osg::Vec3dArray;

		_knotVector = new osg::DoubleArray;
		_order = 0;
	};
	osg::ref_ptr <osg::Vec3dArray> _path;
	osg::ref_ptr <osg::Vec3dArray> _path_left;
	osg::ref_ptr <osg::Vec3dArray> _path_right;

	osg::ref_ptr<osg::Vec3dArray> _ctrl_left;
	osg::ref_ptr<osg::Vec3dArray> _ctrl_right;

	osg::ref_ptr <osg::Vec3dArray> _ctrlPoints;
	osg::ref_ptr <osg::DoubleArray> _knotVector;
	unsigned _order;

protected:
	virtual ~Nurbs(){ std::cout << "Deconstruct Nurbs" << std::endl; };
}Nurbs;

typedef std::vector<osg::ref_ptr<Nurbs>> nurbsList;

typedef struct RoadSet:public osg::Referenced
{
	RoadSet():
	_laneWidth(3.75)
	{
		_roadLane = 2;
		_width = _laneWidth * _roadLane;

		_wallHeight = 0.50f;
		_density = 1000;

		_scale.set(10.0f,10.0f,1.0f);

		_imgRoad = NULL;
		_imgWall = NULL;
	}
	
	const double _laneWidth;	//laneWidth = 3.75 Meters
	int _roadLane;
	double _width;
	double _wallHeight;

	unsigned _density;
	
	osg::Vec3d _scale;
	
	std::string _texture;
	osg::ref_ptr<osg::Image> _imgRoad;
	std::string _textureWall;
	osg::ref_ptr<osg::Image> _imgWall;
	stringList _roadTxt;
	nurbsList _nurbs;

protected:
	virtual ~RoadSet(){ std::cout << "Deconstruct RoadSet" << std::endl; };
}RoadSet;

typedef struct Vehicle:public osg::Referenced
{
	Vehicle() :_MAXSPEED(280 / 3.6f / frameRate)
	{
		_width = 1.7;
		_length = 4.7;
		_height = 0.15;

		_speed = 60 / 3.6f / frameRate;
		_speedincr = 20 / 3.6f / frameRate;
		_rotate = 42.5*TO_RADDIAN;
		_rotationAccl = _rotate;
		_acceleration = 1;

		_V = new osg::Vec3dArray;
		this->_O.set(O_POINT);
	}
	void increaseMaxSpeed() { _speed += _speedincr; _speed = (_speed > _MAXSPEED) ? _MAXSPEED : _speed; };
	void decreaseMaxSpeed() { _speed -= _speedincr; _speed = (_speed > 0) ? _speed : 0; };

	osg::ref_ptr<osg::Vec3dArray> _V;
	osg::Vec3d _O;

	double _width;
	double _height;
	double _length;
	
	double _speed;
	double _speedincr;
	const double _MAXSPEED;
	double _rotate;
	double _rotationAccl;
	bool _acceleration;
	std::string _texture;

protected:
	virtual ~Vehicle(){ std::cout << "Deconstruct Vehicle" << std::endl; };

}Vehicle;

typedef struct Screens:public osg::Referenced
{
	Screens()
	{
		_aspect = 0.0f;
		_fovy = 30.0f;
		_horDistance = .6f;
		_verDistance = 1.2f;
		_verOffset = 0.0f;
		_distortion = 1.0f;
		_fxaa = 4;

		_scrs = new osg::UIntArray;
		_realworld = new osg::DoubleArray;
		_background = "";
	}

	double _aspect;
	double _fovy;
	double _horDistance;
	double _verDistance;
	double _verOffset;
	double _distortion;
	unsigned _fxaa;

	osg::ref_ptr<osg::UIntArray> _scrs;
	osg::ref_ptr<osg::DoubleArray> _realworld;
	std::string _background;

protected:
	virtual ~Screens(){ std::cout << "Deconstruct Screens" << std::endl; };
}Screens;

typedef struct CameraSet :public osg::Referenced
{
	CameraSet()
	{
		_offset = new osg::DoubleArray;
		_eyeTracker = false;
	}

	osg::ref_ptr<osg::DoubleArray> _offset;
	bool _eyeTracker;

protected:
	virtual ~CameraSet(){ std::cout << "Deconstruct CameraSet" << std::endl; };
}CameraSet;

typedef struct Subjects:public osg::Referenced
{
	Subjects()
	{
		_name = "Demo";
		_age = 0;
		_gender = "N/A";
		_driving = 0;
		_randomRoads = false;
	};
	void setName(const std::string ref){ _name = ref; };
	void setAge(const int ref){ _age = ref; };
	void setGender(const std::string ref){ _gender = ref; };
	void setDriving(const double ref){ _driving = ref; };
	void setFilePath(const std::string ref){ _filePath = ref; };
	void setRecPath(const std::string ref){ _recTxt = ref; };
	void setRandomRoads(const bool ref){ _randomRoads = ref; };

	std::string getName() const { return _name; };
	int getAge() const { return _age; };
	std::string getGender() const { return _gender; };
	double getDriving() const { return _driving; };
	std::string getFilePath() const { return _filePath; };
	std::string getRecPath() const { return _recTxt; };
	bool getRandomRoads() const { return _randomRoads; };

	stringList _roads;
protected:
	virtual ~Subjects(){ std::cout << "Deconstruct Subjects" << std::endl; };

private:
	std::string _name;
	int _age;
	std::string _gender;
	double _driving;
	bool _randomRoads;

	std::string _filePath;
	std::string _recTxt;

}Subjects;

class ReadConfig:public osg::Referenced
{
public:
	ReadConfig(const std::string &filename);
	ReadConfig(const std::string &config, const std::string &replay);

	Nurbs * getNurbs() const { return _nurbs.get(); };
	RoadSet * getRoadSet() const { return _roads.get(); };
	Vehicle * getVehicle() const { return _vehicle.get(); };
	Screens * getScreens() const { return _screens.get(); };
	CameraSet *getCameraSet() const { return _camset.get(); };
	Subjects *getSubjects() const { return _subjects.get(); };
	Experiment *getExpSetting() const { return _experiment.get(); };
	osg::Matrix getDistortionMatrix() const{  return osg::Matrix::scale(osg::Vec3d(1.0f,1.0f,1.0f / _screens->_distortion)); };
	osg::Geode* measuer();
	osg::MatrixdArray *getSaveState() const { return _saveState.get(); };
	osg::IntArray *getDynamicState() const { return _dynamicState.get(); };
	bool isReplay() const { return _replay; };
	//test
	osg::ref_ptr<osg::Vec3dArray> _measuerment;
	//test

protected:
	virtual ~ReadConfig();

private:
	//ReadConfig();
	void assignConfig();
	bool byPassSpace(std::ifstream &in, std::string &content);
	std::string getTillFirstSpaceandToUpper(std::string &content);
	void readTrial(std::ifstream &filein);
	void readReply(std::ifstream &filein);
	void initializeAfterReadTrial();
	Nurbs * readNurbs();
	void scaleCtrlPoints();
	void alignCtrlPoints(Nurbs *refNurbs);
	void updateNurbs(osg::ref_ptr<NurbsCurve> refNB);
	
	osg::ref_ptr<Nurbs> _nurbs;
	osg::ref_ptr<Vehicle> _vehicle;
	osg::ref_ptr<Screens> _screens;
	osg::ref_ptr<RoadSet> _roads;
	osg::ref_ptr<CameraSet> _camset;
	osg::ref_ptr<Subjects> _subjects;
	osg::ref_ptr<Experiment> _experiment;
	std::string _filename;

	osg::Vec3d _alignPoint;
	osg::ref_ptr<osg::MatrixdArray> _saveState;
	osg::ref_ptr<osg::IntArray> _dynamicState;
	bool _replay;
};