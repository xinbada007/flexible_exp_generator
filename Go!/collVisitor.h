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
	CollVisitor();
	virtual ~CollVisitor();

	virtual void reset();
	void setMode(ROADTAG _ref);
	inline ROADTAG getMode() { return _mode; };
	inline bool ifUseMode() { return _useMode; };
	inline void disableMode() { _useMode = false; };
	virtual void apply(osg::Group& node);
	inline const osg::ref_ptr<Car> getCar() const { return _car.get(); };
	inline const solidList getObstacle() const { return _obs; };
	inline const solidList getRoad() const { return _rd; };
	inline const solidList getWall() const { return _wall; };
private:
	void pushLR(osg::ref_ptr<LogicRoad> refLR);
	solidList _obs;
	solidList _rd;
	solidList _wall;
	osg::ref_ptr<Car> _car;
	ROADTAG _mode;
	bool _useMode;
};

