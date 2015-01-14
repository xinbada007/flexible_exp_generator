#pragma once
#include <osg/NodeVisitor>
#include <osg/Geode>
#include <vector>

#include "car.h"
#include "logicRoad.h"

class CollVisitor :
	public osg::NodeVisitor
{
public:
	virtual void reset();
	void setMode(ROADTAG _ref);
	inline ROADTAG getMode() { return _mode; };
	virtual void apply(osg::Group& node);
	inline const obstacleList getObstacle() const { return _obs; };
	inline const solidList getRoad() const { return _rd; };
	const solidList getWall() const;
	static CollVisitor *instance();

private:
	CollVisitor();
	virtual ~CollVisitor();
	void pushLR(osg::ref_ptr<LogicRoad> refLR);

	obstacleList _obs;
	solidList _rd;
	solidList _lwall;
	solidList _rwall;
	mutable solidList _wall;

	ROADTAG _mode;
};

