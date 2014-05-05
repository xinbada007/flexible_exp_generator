#pragma once
#include <osg/NodeVisitor>
#include <osg/Geode>
#include <vector>

#include "car.h"

class CollVisitor :
	public osg::NodeVisitor
{
public:
	CollVisitor();
	virtual ~CollVisitor();

	virtual void reset();
	virtual void apply(osg::Group& node);
	inline const osg::ref_ptr<Car> getCar() const { return _car.get(); };
	inline const solidList getObstacle() const { return _obs; };
	inline const solidList getRoad() const { return _rd; };
	inline const solidList getWall() const { return _wall; };
private:
	solidList _obs;
	solidList _rd;
	solidList _wall;
	osg::ref_ptr<Car> _car;
};

