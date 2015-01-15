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
struct  Nurbs;
typedef std::vector<osg::ref_ptr<Nurbs>> nurbsList;

typedef struct Experiment :public osg::Referenced
{
	Experiment()
	{
		_startLane = 0;
		_laneOffset = 0.0f;

		_textTime = new osg::UIntArray;
		_textPeriod = new osg::DoubleArray;
		_dynamicChange = new osg::UIntArray;
		_dynamicChangeCondition = 0;
		_dynamicChangeLasting = 0;
		_imgDynamic = NULL;

		_obstaclesTime = new osg::UIntArray;
		_obstacleRange = new osg::DoubleArray;
		_obstaclePos = new osg::IntArray;
		_obsPosOffset = new osg::DoubleArray;
		_obsSize.set(1.0f, 1.0f, 1.0f);
		_obsArraySize = 0.25f;
		_obsShape = 1;
		_obsVisible = 300.0f;
		_imgOBS = NULL;
		_imgObsArray = NULL;
		_imgAnisotropy = 1.0f;
		_numObsinArray = 100;
		_animationMode = false;
		_GLPOINTSMODE = false;
		_obsLength = 50.0f;
		_alignment = 0.0f;
		_speed = 60.0f / 3.6f;

		_opticFlow = false;
		_opticFlowRange = 100;
		_depthDensity = 1.0f;
		_opticFlowWidth = 100;
		_opticFlowWDensity = 250;
		_opticFlowHeight = 40;
		_opticFlowHDensity = 100;
		_opticFlowFrameCounts = 0;
		_opticFlowVersions = 0;

		_offset = 0.0f;

		_deviation = 0.0f;
		_deviationBaseline = 0.0f;
	};
	int _startLane;
	double _laneOffset;

	osg::ref_ptr <osg::UIntArray> _textTime;
	osg::ref_ptr <osg::DoubleArray> _textPeriod;
	stringList _textContent;

	osg::ref_ptr <osg::UIntArray> _dynamicChange;
	unsigned _dynamicChangeCondition;
	unsigned _dynamicChangeLasting;
	std::string _dynamicPic;
	osg::ref_ptr<osg::Image> _imgDynamic;

	osg::ref_ptr<osg::UIntArray> _obstaclesTime;
	osg::ref_ptr<osg::DoubleArray> _obstacleRange;
	osg::ref_ptr<osg::IntArray> _obstaclePos;
	osg::ref_ptr<osg::DoubleArray> _obsPosOffset;
	osg::Vec3d _obsSize;
	double _obsArraySize;
	std::string _obsArrayPic;
	osg::ref_ptr<osg::Image> _imgObsArray;
	int _obsShape;
	double _obsVisible;
	std::string _obsPic;
	osg::ref_ptr<osg::Image> _imgOBS;
	double _imgAnisotropy;
	stringList _obsArray;
	unsigned _numObsinArray;
	nurbsList _nurbs;
	bool _animationMode;
	bool _GLPOINTSMODE;
	double _obsLength;
	double _alignment;
	double _speed;

	bool _opticFlow;
	int _opticFlowRange;
	double _depthDensity;
	double _opticFlowWidth;
	int _opticFlowWDensity;
	double _opticFlowHeight;
	int _opticFlowHDensity;
	unsigned int _opticFlowFrameCounts;
	unsigned int _opticFlowVersions;

	double _offset;

	double _deviation;
	double _deviationBaseline;
	std::string _deviationWarn;
	std::string _deviationSiren;

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

		_radius = new osg::DoubleArray;
		_radiusR = new osg::DoubleArray;
		_radiusL = new osg::DoubleArray;

		_scale.set(1.0f, 1.0f, 1.0f);

		_order = 0;
	};
	osg::ref_ptr <osg::Vec3dArray> _path;
	osg::ref_ptr <osg::Vec3dArray> _path_left;
	osg::ref_ptr <osg::Vec3dArray> _path_right;

	osg::ref_ptr<osg::Vec3dArray> _ctrl_left;
	osg::ref_ptr<osg::Vec3dArray> _ctrl_right;

	osg::ref_ptr <osg::Vec3dArray> _ctrlPoints;
	osg::ref_ptr <osg::DoubleArray> _knotVector;

	osg::ref_ptr<osg::DoubleArray> _radius;
	osg::ref_ptr<osg::DoubleArray> _radiusR;
	osg::ref_ptr<osg::DoubleArray> _radiusL;

	osg::Vec3d _scale;

	unsigned _order;

protected:
	virtual ~Nurbs(){ std::cout << "Deconstruct Nurbs" << std::endl; };
}Nurbs;

typedef struct RoadSet:public osg::Referenced
{
	RoadSet():
	_laneWidth(3.75)
	{
		_roadLane = 2;
		_width = _laneWidth * _roadLane;
		_length = 0.0f;

		_wallHeight = 0.50f;
		_density = 1000;

		_scale.set(1.0f,1.0f,1.0f);

		_textureWidth = 8.0f;
		_imgRoad = NULL;
		_imgRoadAnisotropy = 1.0f;
		_imgWall = NULL;
		_imgWallAnisotropy = 1.0f;

		_nurbsMethod = 1;

		_visible = 1;
	}
	
	const double _laneWidth;	//laneWidth = 3.75 Meters
	int _roadLane;
	double _width;
	double _length;
	double _wallHeight;

	unsigned _density;
	
	osg::Vec3d _scale;
	
	std::string _texture;
	double _textureWidth;
	osg::ref_ptr<osg::Image> _imgRoad;
	double _imgRoadAnisotropy;
	
	std::string _textureWall;
	osg::ref_ptr<osg::Image> _imgWall;
	double _imgWallAnisotropy;

	stringList _roadTxt;
	nurbsList _nurbs;

	int _nurbsMethod;
	int _visible;

protected:
	virtual ~RoadSet(){ std::cout << "Deconstruct RoadSet" << std::endl; };
}RoadSet;

typedef struct Vehicle:public osg::Referenced
{
	Vehicle() :_MAXSPEED(280 / 3.6f)
	{
		_width = 1.7;
		_length = 4.7;
		_height = 0.15;

		_speed = 60 / 3.6f;
		_speedincr = 20 / 3.6f;
		_rotate = 42.5 * TO_RADDIAN;
		_rotationAccl = _rotate;
		_acceleration = 1;
		_dynamicSensitive = 1.0f;

		_startDelay = 1.0f;

		_baseline = 0.0f;

		_visibility = true;

		_carReset = 0;
		_resetMode = 0;

		_V = new osg::Vec3dArray;
		this->_O.set(O_POINT);
	}
	void increaseMaxSpeed() {_speed += _speedincr; _speed = (_speed > _MAXSPEED) ? _MAXSPEED : _speed;};
	void decreaseMaxSpeed() {_speed -= _speedincr; _speed = (_speed > 0) ? _speed : 0;};

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
	double _dynamicSensitive;
	std::string _texture;

	double _startDelay;

	double _baseline;

	bool _visibility;

	int _carReset;
	int _resetMode;

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
		_imgBg = NULL;
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
	osg::ref_ptr<osg::Image> _imgBg;

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
	void setTrialName(const std::string ref){ _trialName = ref; };

	std::string getName() const { return _name; };
	int getAge() const { return _age; };
	std::string getGender() const { return _gender; };
	double getDriving() const { return _driving; };
	std::string getFilePath() const { return _filePath; };
	std::string getRecPath() const { return _recTxt; };
	bool getRandomRoads() const { return _randomRoads; };
	std::string getTrialName() const { return _trialName; };

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
	std::string _trialName;

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
	osg::MatrixdArray *getSaveState() const { return _saveState.get(); };
	osg::IntArray *getDynamicState() const { return _dynamicState.get(); };
	bool isReplay() const { return _replay; };

	osg::Geode* measuer();
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
	Nurbs * readNurbs(const double &customLength = 0.0f);
	void scaleCtrlPoints();
	void alignCtrlPoints(Nurbs *refNurbs, const double &offset = 0.0f);
	void updateNurbs(osg::ref_ptr<NurbsCurve> refNB, const unsigned &density, const double &width = 0.0f);
	void updateNurbs(const unsigned &density,const double &width);
	
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