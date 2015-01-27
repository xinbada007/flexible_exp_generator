#pragma once
#include <osg/NodeCallback>
#include <osg/Array>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/Switch>
#include <osg/NodeVisitor>

#include <vector>

#include "road.h"
struct CarState;

class Solid;

class DebugNode :
	public osg::NodeCallback
{
public:
	DebugNode();
	virtual ~DebugNode();

	void operator()(osg::Node *node, osg::NodeVisitor *nv);
	void setCarState(CarState *carS){ _carState = carS; };

private:
	CarState *_carState;

	osg::ref_ptr<osg::Group> _root;

	osg::ref_ptr<osg::Switch> _carDebugSwitch;
	osg::ref_ptr<osg::Geode> _carDebugGde;
	std::vector<osg::ref_ptr<osg::Vec3dArray>> _carDebugArray;

	osg::Switch *_road;

	void addCarDebugger();
	osg::ref_ptr<osg::Geometry> addCarDebuggerDrawables(osg::Vec3dArray *refArray);

	osg::ref_ptr<osg::Vec4Array> _colorIndex;
	osg::Vec4Array::const_iterator _i_color;

	double _z_deepth;
	void setDeepth(std::vector<osg::ref_ptr<osg::Vec3dArray>> &refVector);
};

class deTexture :
	public osg::NodeVisitor
{
public:
	deTexture();
	virtual ~deTexture();

	virtual void reset();
	virtual void apply(osg::Group &refNode);

private:
	void setTexture(Solid *refS);
};

class switchRoad :
	public osg::NodeVisitor
{
public:
	switchRoad();
	virtual ~switchRoad();

	virtual void reset();
	virtual void apply(osg::Group &refNode);
	osg::Switch * getSwRoad() { return _road; };
//	osg::Switch * getSwWall() { return _wall; };
private:
	osg::Switch *_road;
//	osg::Switch *_wall;
};