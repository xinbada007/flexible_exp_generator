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
typedef std::pair<int, enum TRIGGER_COM> triggerEnablePair;

typedef struct Experiment :public osg::Referenced
{
	Experiment():
	_NUMTRIGGERCOM(8)
	{
		_timer = 0;
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
		_obsCollision = new osg::UIntArray;
		_obsSize.set(1.0f, 1.0f, 1.0f);
		_obsShape = 1;
		_obsVisible = 300.0f;
		_imgOBS = NULL;
		_imgObsArray = NULL;
		_imgAnisotropy = 1.0f;
		_numObsinArray = 100;
		_obsArrayOFFSET = 0.0f;
		_animationMode = false;
		_animationLoop = NO_LOOPING;
		_obsArrayNurbsMethod = 0;
		_GLPOINTSMODE = false;
		_obsArrayLength = 0.0f;
		_speed = 60.0f / 3.6f;

		_opticFlow = false;
		_opticFlowRange = 100;
		_depthDensity = 1.0f;
		_opticFlowWidth = 100;
		_opticFlowHeight = 10;
		_opticFlowDensity = 100;
		_opticFlowFrameCounts = 0;
		_opticFlowVersions = 0;

		_triggerTimer = new osg::DoubleArray;

		_offset = 0.0f;

		_deviation = 0.0f;
		_deviationBaseline = 0.0f;
	};
	unsigned _timer;
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
	osg::ref_ptr<osg::UIntArray> _obsCollision;
	osg::Vec3d _obsSize;
	osg::Vec3d _obsArraySize;
	std::string _obsArrayPic;
	osg::ref_ptr<osg::Image> _imgObsArray;
	int _obsShape;
	double _obsVisible;
	std::string _obsPic;
	osg::ref_ptr<osg::Image> _imgOBS;
	double _imgAnisotropy;

	stringList _obsArray;
	osg::Vec3d _obsArrayAlign;
	double _obsArrayOFFSET;
	unsigned _numObsinArray;
	nurbsList _nurbs;
	bool _animationMode;
	int _animationLoop;
	unsigned _obsArrayNurbsMethod;
	enum ANIMELOOP
	{
		SWING,
		LOOP,
		NO_LOOPING
	};
	bool _GLPOINTSMODE;
	double _obsArrayLength;
	double _speed;

	bool _opticFlow;
	int _opticFlowRange;
	int _depthDensity;
	double _opticFlowWidth;
	double _opticFlowHeight;
	int _opticFlowDensity;
	unsigned int _opticFlowFrameCounts;
	unsigned int _opticFlowVersions;

	std::vector<triggerEnablePair> _triggerEnable;
	osg::ref_ptr<osg::DoubleArray> _triggerTimer;
	enum TRIGGER_COM
	{
		ROAD,
		FLOW,
		CRASHPERMIT,
		SOUNDALERT,
		NEAR_FAR_COMPUTE,
		CARRESET,
		JOYSTICK,
		QUIT
	};
	const unsigned _NUMTRIGGERCOM;

	double _offset;

	double _deviation;
	double _deviationBaseline;
	std::string _deviationWarn;
	std::string _deviationSiren;

protected:
	virtual ~Experiment()
	{
		std::cout << "Deconstruct Experiment" << std::endl;
		_textTime = NULL;
		_textPeriod = NULL;
		_dynamicChange = NULL;
		_imgDynamic = NULL;
		_obstaclesTime = NULL;
		_obstacleRange = NULL;
		_obstaclePos = NULL;
		_obsPosOffset = NULL;
		_obsCollision = NULL;
		_imgObsArray = NULL;
		_imgOBS = NULL;
		_triggerTimer = NULL;
	};
}Experiment;

typedef struct Nurbs :public osg::Referenced
{
	Nurbs()
	{
		_path = new osg::Vec3dArray;
		_path_left = new osg::Vec3dArray;
		_path_right = new osg::Vec3dArray;

		_wall_left = _path_left;
		_wall_right = _path_right;

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
	Nurbs(const Nurbs &copy)
	{
		_path = new osg::Vec3dArray(copy._path->begin(), copy._path->end());
		_path_left = new osg::Vec3dArray(copy._path_left->begin(), copy._path_left->end());
		_path_right = new osg::Vec3dArray(copy._path_right->begin(), copy._path_right->end());

		_wall_left = new osg::Vec3dArray(copy._wall_left->begin(), copy._wall_left->end());
		_wall_right = new osg::Vec3dArray(copy._wall_right->begin(), copy._wall_right->end());

		_ctrl_left = new osg::Vec3dArray(copy._ctrl_left->begin(), copy._ctrl_left->end());
		_ctrl_right = new osg::Vec3dArray(copy._ctrl_right->begin(), copy._ctrl_right->end());
		_ctrlPoints = new osg::Vec3dArray(copy._ctrlPoints->begin(), copy._ctrlPoints->end());

		_knotVector = new osg::DoubleArray(copy._knotVector->begin(), copy._knotVector->end());

		_radius = new osg::DoubleArray(copy._radius->begin(), copy._radius->end());
		_radiusL = new osg::DoubleArray(copy._radiusL->begin(), copy._radiusL->end());
		_radiusR = new osg::DoubleArray(copy._radiusR->begin(), copy._radiusR->end());

		_scale = copy._scale;

		_order = copy._order;
	};
	Nurbs & operator=(const Nurbs & copy)
	{
		if (this != &copy)
		{
			_path = new osg::Vec3dArray(copy._path->begin(), copy._path->end());
			_path_left = new osg::Vec3dArray(copy._path_left->begin(), copy._path_left->end());
			_path_right = new osg::Vec3dArray(copy._path_right->begin(), copy._path_right->end());

			_wall_left = new osg::Vec3dArray(copy._wall_left->begin(), copy._wall_left->end());
			_wall_right = new osg::Vec3dArray(copy._wall_right->begin(), copy._wall_right->end());

			_ctrl_left = new osg::Vec3dArray(copy._ctrl_left->begin(), copy._ctrl_left->end());
			_ctrl_right = new osg::Vec3dArray(copy._ctrl_right->begin(), copy._ctrl_right->end());
			_ctrlPoints = new osg::Vec3dArray(copy._ctrlPoints->begin(), copy._ctrlPoints->end());

			_knotVector = new osg::DoubleArray(copy._knotVector->begin(), copy._knotVector->end());

			_radius = new osg::DoubleArray(copy._radius->begin(), copy._radius->end());
			_radiusL = new osg::DoubleArray(copy._radiusL->begin(), copy._radiusL->end());
			_radiusR = new osg::DoubleArray(copy._radiusR->begin(), copy._radiusR->end());

			_scale = copy._scale;

			_order = copy._order;
		}
	}
	osg::ref_ptr <osg::Vec3dArray> _path;
	osg::ref_ptr <osg::Vec3dArray> _path_left;
	osg::ref_ptr <osg::Vec3dArray> _path_right;

	osg::ref_ptr<osg::Vec3dArray> _wall_left;
	osg::ref_ptr<osg::Vec3dArray> _wall_right;

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
	virtual ~Nurbs()
	{
		std::cout << "Deconstruct Nurbs" << std::endl; 
		_path = NULL;
		_path_left = NULL;
		_path_right = NULL;

		_wall_left = NULL;
		_wall_right = NULL;

		_ctrl_left = NULL;
		_ctrl_right = NULL;
		_ctrlPoints = NULL;

		_knotVector = NULL;

		_radius = NULL;
		_radiusR = NULL;
		_radiusL = NULL;
	};
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
		_wallRoadWidth = 0.0f;
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
	double _roadLane;
	double _width;
	double _length;
	double _wallHeight;
	double _wallRoadWidth;

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
	virtual ~RoadSet()
	{ 
		std::cout << "Deconstruct RoadSet" << std::endl; 

		_imgRoad = NULL;
		_imgWall = NULL;
	};
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

		_carReset = DISABLE;
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
	osg::Matrixd _initialState;

	enum VEHICLE_RESET_TYPE
	{
		DISABLE,AUTO,MANUAL
	};

protected:
	virtual ~Vehicle()
	{ 
		std::cout << "Deconstruct Vehicle" << std::endl; 

		_V = NULL;
	};

}Vehicle;

typedef struct Screens:public osg::Referenced
{
	Screens()
	{
		_HMD = 0;
		_HMD_RefreshRate = 60.0f;

		_zNear = 0.0f;
		_zFar = 0.0f;
		_aspect = 0.0f;
		_fovy = 30.0f;
		_horDistance = .6f;
		_verDistance = 1.2f;
		_verOffset = 0.0f;
		_distortion = 1.0f;
		_fxaa = 4;
		_hFov = 2.0f*atan(_aspect*tan(_fovy*0.5f));	//default unit is degree

		_scrs = new osg::UIntArray;
		_realworld = new osg::DoubleArray;
		_imgBg = NULL;
	}
	
	unsigned _HMD;
	double _HMD_RefreshRate;

	double _zNear;
	double _zFar;
	double _aspect;
	double _fovy;
	double _horDistance;
	double _verDistance;
	double _verOffset;
	double _distortion;
	unsigned _fxaa;
	double _hFov;

	osg::ref_ptr<osg::UIntArray> _scrs;
	osg::ref_ptr<osg::DoubleArray> _realworld;
	std::string _background;
	osg::ref_ptr<osg::Image> _imgBg;

protected:
	virtual ~Screens()
	{ 
		std::cout << "Deconstruct Screens" << std::endl; 

		_scrs = NULL;
		_realworld = NULL;
		_imgBg = NULL;
	};
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
	virtual ~CameraSet()
	{
		std::cout << "Deconstruct CameraSet" << std::endl; 

		_offset = NULL;
	};
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
	void setDirectoryName(const std::string ref){ _directoryName = ref; };

	std::string getName() const { return _name; };
	int getAge() const { return _age; };
	std::string getGender() const { return _gender; };
	double getDriving() const { return _driving; };
	std::string getFilePath() const { return _filePath; };
	std::string getRecPath() const { return _recTxt; };
	bool getRandomRoads() const { return _randomRoads; };
	std::string getTrialName() const { return _trialName; };
	std::string getDirectoryName() const { return _directoryName; };

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
	std::string _directoryName;

}Subjects;

class ReadConfig:public osg::Referenced
{
public:
	ReadConfig(const std::string &filename);
	ReadConfig(const std::string &config, const std::string &replay);

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

	osg::ref_ptr<Nurbs> readNurbs(const double &customLength = 0.0f);
	void scaleCtrlPoints(Nurbs *nurbs);
	void alignCtrlPoints(Nurbs *thisNurbs, Nurbs *prevNurbs, const double &offset = 0.0f);
	void updateNurbs(Nurbs *thisNurbs, osg::ref_ptr<NurbsCurve> refNB, const unsigned &density, const double &width = 0.0f);
	void updateNurbs(Nurbs *thisNurbs, const unsigned &density, const double &width);
	
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