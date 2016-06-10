#pragma once
#include <osg/NodeCallback>
#include <osg/Camera>
#include <osgText/Text>
#include <osg/AnimationPath>

#include <osgAudio/SoundState.h>

#include "car.h"
#include "readConfig.h"
#include "mulitViewer.h"
#include "obstacle.h"
#include "opticFlow.h"

class CollVisitor;

class ExperimentCallback :
	public osg::NodeCallback
{
public:
	ExperimentCallback(const ReadConfig *rc);
	virtual ~ExperimentCallback();
	void operator()(osg::Node* node, osg::NodeVisitor* nv);
	void setHUDCamera(osg::Camera *cam);
	void setHUDCameraLeft(osg::Camera *camLeft);
	void setHUDCameraRight(osg::Camera *camRight);

	inline void setViewer(MulitViewer *mv)
	{ 
		_mv = (mv) ? mv : NULL;
		_fovX = (_mv) ? _mv->getHorizontalFov() : 0.0f;
		_clearColor = (_mv) ? _mv->getMainView()->getCamera()->getClearColor() : _clearColor;
	};
	inline void setCar(Car *car) 
	{
		_car = NULL;
		if (car)
		{
			_car = car;
			if (car->getVehicle()->_carInsideLight)
			{
				_carLightColorAmbient = car->getVehicle()->_carInsideLight->getLight()->getAmbient();
				_carLightColorDiffuse = car->getVehicle()->_carInsideLight->getLight()->getDiffuse();
			}
			_memorisedCarTime = _car->getCarState()->_startTime;
		}
	};
private:
	Experiment *_expSetting;
	Car *_car;
	CollVisitor *_cVisitor;

	double _expTime;
	double _memorisedExpTime;
	double _memorisedCarTime;
	bool _insertTrigger;
	osg::ref_ptr<osg::UIntArray> _dynamic;
	bool _dynamicUpdated;
	bool _deviationWarn;
	bool _deviationLeft;
	double _thisMomentDynamic;

	std::vector<osg::ref_ptr<Obstacle>> _obstacleList;
	std::vector<osg::ref_ptr<Obstacle>> _collisionOBSList;
	bool _obsListDrawn;

	osg::ref_ptr<osg::Switch> _opticFlowPoints;
	std::vector<std::pair<std::vector<osg::ref_ptr<osg::Vec3Array>>,unsigned>> _opticFlowVersions;
	bool _opticFlowDrawn;
	std::vector<int> _opticFlowDynamicIndex;
	std::vector<int> _forward_Vec;
	std::vector<int> _backward_Vec;

	osg::ref_ptr<osgAudio::SoundState> _siren;
	osg::ref_ptr<osgAudio::Sample> _sirenSample;
	osg::ref_ptr<osgAudio::SoundState> _coin;
	osg::ref_ptr<osgAudio::Sample> _coinSample;

	osg::Camera *_cameraHUD;
	osg::Camera *_cameraHUDLeft;
	osg::Camera *_cameraHUDRight;
	osg::ref_ptr<osg::Geode> _geodeHUDLeft;
	osg::ref_ptr<osg::Geode> _geodeHUDRight;

	osg::ref_ptr<osgText::Text> _textHUD;
	osg::ref_ptr<osg::Geode> _geodeHUD;
	osg::ref_ptr<osg::Group> _root;
	osg::ref_ptr<osg::Switch> _road;
	osg::ref_ptr<MulitViewer> _mv;
	osg::Vec4d _clearColor;
	osg::Vec4d _otherClearColor;

	osg::Vec4d _carLightColorAmbient;
	osg::Vec4d _carLightColorDiffuse;
	bool _speedColor;

	const double _roadLength;

	osg::ref_ptr<osg::AnimationPathCallback> _anmCallback;
	osg::ref_ptr<osg::Vec3dArray> _centerList;
	double _timeBuffer;
	double _timeLastRecored;

	bool _switchOpticFlow;

	double _fovX;
	const double _zNear;
	const double _zFar;
	unsigned _frameNumber;

	unsigned _rdNumber;
	std::vector<std::vector<unsigned>> _recorder;
	bool _switchRoad;
	bool _switchOBS;
	bool _switchDynamicFlow;
	osg::ref_ptr<osg::Vec3dArray> _HUDObs;

	void dynamicChange();
	void showText();
	void positionCar();
	void showObstacle();
	void showOpticFlow();
	void opticFlowRange();
	void dynamicFlow(osg::ref_ptr<OpticFlow> obs, const unsigned depth);
	void foregroundFlow(const std::vector<int> &forevec);
	void deviationCheck();

	void createObstacles();
	void createOpticFlow();
	void dealCollision();
	void moveObstaclewithCar();
	void moveObstacleManually();
	
	void createHUDObstacles(const osg::Vec3d &center, const osg::Vec4d &color = osg::Vec4d(1.0, 0.0, 0.0, 1.0f));
	osg::Vec3d converttoHUD(const osg::Vec3d& input, unsigned *CAMERA = NULL);

	void trigger();

	void removeNodefromRoad(osg::Node *n);
};

class GeometryVistor :
	public osg::NodeVisitor
{
public:
	GeometryVistor();
	void setColor(const double &r, const double &g, const double &b);
	virtual void apply(osg::Geometry &geom);
private:
	double _r;
	double _g;
	double _b;
};