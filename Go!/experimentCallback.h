#pragma once
#include <osg/NodeCallback>
#include <osg/Camera>
#include <osgText/Text>

#include "car.h"
#include "readConfig.h"

class ExperimentCallback :
	public osg::NodeCallback
{
public:
	ExperimentCallback(const ReadConfig *rc);
	virtual ~ExperimentCallback();
	void operator()(osg::Node* node, osg::NodeVisitor* nv);
	void setHUDCamera(osg::Camera *cam);
private:
	const Experiment *_expSetting;
	CarState *_carState;
	double _expTime;
	osg::ref_ptr<osg::UIntArray> _dynamic;
	bool _dynamicUpdated;
	double _thisMomentDynamic;

	osg::ref_ptr<osg::IntArray> _obstacle;

	osg::Camera *_cameraHUD;
	osg::ref_ptr<osgText::Text> _textHUD;
	osg::ref_ptr<osg::Geode> _geodeHUD;
	osg::ref_ptr<osg::Group> _root;
	osg::ref_ptr<osg::Switch> _road;
	void dynamicChange();
	void showText();
	void createObstacle();
};

