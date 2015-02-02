#pragma once
#include "logicRoad.h"
#include "readConfig.h"

#include <osg/Switch>
#include <osg/NodeVisitor>

#include <vector>

typedef std::vector<LogicRoad *> logicRoadList;

class Road :
	public osg::Switch
{
public:
	Road();
	Road(const Road &copy, osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY);
	META_Node(Road, Road);

	void genRoad(osg::ref_ptr<ReadConfig> refRC);
	const osg::ref_ptr<RoadSet> getRoadSet() const { return _roadSet; };
private:
	osg::ref_ptr<osg::Vec2Array> universalTexture(const int i,const double size);
	osg::ref_ptr<LogicRoad> universalLogicRoad(const int i, ROADTAG refTag);
	osg::Group * addLogRoadtoList(LogicRoad *refLR);

	const int _LEFTWALL;
	const int _RIGHTWALL;

	osg::ref_ptr<RoadSet> _roadSet;

	logicRoadList _roadList;
	logicRoadList _lWallList;
	logicRoadList _rWallList;

	logicRoadList _ctrlList;
	logicRoadList _midList;
protected:
	virtual ~Road();
};


class ListRoadVisitor :
	public osg::NodeVisitor
{
public:
	ListRoadVisitor() :osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN){};
	virtual ~ListRoadVisitor(){};

	void apply(osg::Group &node);
};
