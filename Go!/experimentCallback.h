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

class CollVisitor;

class ExperimentCallback :
	public osg::NodeCallback
{
public:
	ExperimentCallback(const ReadConfig *rc);
	virtual ~ExperimentCallback();
	void operator()(osg::Node* node, osg::NodeVisitor* nv);
	void setHUDCamera(osg::Camera *cam);
	inline void setMultiViewer(MulitViewer *mv) { _mv = mv; };
	inline void setCar(Car *car) { _car = car;};
private:
	Experiment *_expSetting;
	Car *_car;
	CollVisitor *_cVisitor;

	double _expTime;
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

	osg::ref_ptr<osgAudio::SoundState> _siren;
	osg::ref_ptr<osgAudio::SoundState> _coin;
	osg::ref_ptr<osgAudio::Sample> _coinSample;

	osg::Camera *_cameraHUD;
	osg::ref_ptr<osgText::Text> _textHUD;
	osg::ref_ptr<osg::Geode> _geodeHUD;
	osg::ref_ptr<osg::Group> _root;
	osg::ref_ptr<osg::Switch> _road;
	osg::ref_ptr<MulitViewer> _mv;
	const double _roadLength;

	osg::ref_ptr<osg::AnimationPathCallback> _anmCallback;
	osg::ref_ptr<osg::Vec3dArray> _centerList;
	double _timeBuffer;
	double _timeLastRecored;

	bool _switchOpticFlow;

	const double _fovX;
	unsigned _frameNumber;

	void dynamicChange();
	void showText();
	void showObstacle();
	void showOpticFlow();
	void dynamicFlow(osg::ref_ptr<Obstacle> obs, const unsigned depth);
	void deviationCheck();

	void createObstacles();
	void createOpticFlow();
	void dealCollision();

	void trigger();

	void removeNodefromRoad(osg::Node *n);
};

