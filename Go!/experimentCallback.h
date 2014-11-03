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
	inline void setCarState(CarState *cars) { _carState = cars; };
private:
	const Experiment *_expSetting;
	CarState *_carState;
	CollVisitor *_cVisitor;

	double _expTime;
	osg::ref_ptr<osg::UIntArray> _dynamic;
	bool _dynamicUpdated;
	bool _deviationWarn;
	bool _deviationLeft;
	double _thisMomentDynamic;

	std::vector<osg::ref_ptr<Obstacle>> _obstacleList;
	bool _obsListDrawn;

	osg::ref_ptr<osgAudio::SoundState> _siren;
	osg::ref_ptr<osgAudio::SoundState> _coin;
	osg::ref_ptr<osgAudio::Sample> _coinSample;

	osg::ref_ptr<osg::IntArray> _obstacle;

	osg::Camera *_cameraHUD;
	osg::ref_ptr<osgText::Text> _textHUD;
	osg::ref_ptr<osg::Geode> _geodeHUD;
	osg::ref_ptr<osg::Group> _root;
	osg::ref_ptr<osg::Switch> _road;
	osg::ref_ptr<MulitViewer> _mv;

	osg::ref_ptr<osg::AnimationPathCallback> _anmCallback;
	osg::ref_ptr<osg::Vec3dArray> _centerList;
	double _timeBuffer;
	double _timeLastRecored;

	void dynamicChange();
	void showText();
	void showObstacle();
	void deviationCheck();

	void createObstacles();
	void dealCollision();

	void removeNodefromRoad(osg::Node *n);
};

