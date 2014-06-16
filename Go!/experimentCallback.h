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

	osg::Camera *_camera;
	osg::ref_ptr<osg::Switch> _txtSwithcer;
	osg::ref_ptr<osgText::Text> _text;
	osg::ref_ptr<osg::Geode> _geode;
	void laneChange();
	void dynamicChange();
	void showText();
};

