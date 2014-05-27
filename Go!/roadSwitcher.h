#pragma once
#include <osg/NodeCallback>
#include <osg/NodeVisitor>
#include <osg/Switch>

struct CarState;

class RoadSwitcher :
	public osg::NodeCallback
{
public:
	RoadSwitcher();
	virtual ~RoadSwitcher();
	void operator()(osg::Node* node, osg::NodeVisitor* nv);

private:
	CarState *_carState;
};

class RoadSwitchVisitor :
	public osg::NodeVisitor
{
public:
	RoadSwitchVisitor(int ON = -1, int OFF = -1) :_solidIndexON(ON), _solidIndexOFF(OFF), osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN){};
	virtual ~RoadSwitchVisitor(){};

	void apply(osg::Switch &sw);

private:
	int _solidIndexON;
	int _solidIndexOFF;
};