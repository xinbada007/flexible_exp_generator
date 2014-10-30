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
	inline void setCar(osg::ref_ptr<Car> c) { _car = c; };
	inline const osg::ref_ptr<Car> getCar() const { return _car.get(); };
	inline const solidList getObstacle() const { return _obs; };
	inline const solidList getRoad() const { return _rd; };
	const solidList getWall() const;
	static CollVisitor *instance();
private:
	CollVisitor();
	virtual ~CollVisitor();
	void pushLR(osg::ref_ptr<LogicRoad> refLR);
	solidList _obs;
	solidList _rd;
	solidList _lwall;
	solidList _rwall;
	mutable solidList _wall;
	osg::ref_ptr<Car> _car;
	ROADTAG _mode;
};

