#pragma once
#include <osg/NodeCallback>
#include <osg/NodeVisitor>
#include <osg/Switch>
#include <osg/MatrixTransform>

struct CarState;

class RoadSwitcher :
	public osg::NodeCallback
{
public:
	RoadSwitcher();
	virtual ~RoadSwitcher();
	void operator()(osg::Node* node, osg::NodeVisitor* nv);
	void setCarState(CarState *cars) { _carState = cars; };
private:
	CarState *_carState;
};

class RoadSwitchVisitor :
	public osg::NodeVisitor
{
public:
	RoadSwitchVisitor(int ON = -1, int OFF = -1, bool ONALL = false, bool OFFALL = false) :_solidIndexON(ON), _solidIndexOFF(OFF), _AllON(ONALL), _AllOFF(OFFALL), osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN){};
	virtual ~RoadSwitchVisitor(){};

	void apply(osg::Switch &sw);

private:
	int _solidIndexON;
	int _solidIndexOFF;

	bool _AllON;
	bool _AllOFF;
};

class OBSSwitchVisitor :
	public osg::NodeVisitor
{
public:
	OBSSwitchVisitor(bool ONALL = false, bool OFFALL = false) :_AllON(ONALL), _AllOFF(OFFALL), osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN){};
	virtual ~OBSSwitchVisitor(){};

	void apply(osg::MatrixTransform &mt);

private:
	bool _AllON;
	bool _AllOFF;
};