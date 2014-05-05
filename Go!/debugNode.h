#pragma once
#include <osg/NodeCallback>
#include <osg/Array>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/Switch>
#include <osg/NodeVisitor>

#include <vector>

#include "road.h"

class Solid;

class DebugNode :
	public osg::NodeCallback
{
public:
	DebugNode();
	virtual ~DebugNode();

	void operator()(osg::Node *node, osg::NodeVisitor *nv);

private:
	osg::ref_ptr<osg::Group> _root;

	osg::ref_ptr<osg::Switch> _carDebugSwitch;
	osg::ref_ptr<osg::Geode> _carDebugGde;
	std::vector<osg::ref_ptr<osg::Vec3Array>> _carDebugArray;

	osg::Switch *_road;

	void addCarDebugger();
	osg::ref_ptr<osg::Geometry> addCarDebuggerDrawables(osg::Vec3Array *refArray);

	osg::ref_ptr<osg::Vec4Array> _colorIndex;
	osg::Vec4Array::const_iterator _i_color;

	double _z_deepth;
	void setDeepth(std::vector<osg::ref_ptr<osg::Vec3Array>> &refVector);
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
private:
	osg::Switch *_road;
};