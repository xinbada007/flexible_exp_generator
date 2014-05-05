#pragma once
#include <osg/NodeCallback>

#include "solid.h"
#include "car.h"

class Collision:public osg::NodeCallback
{
public:
	Collision();
	void operator()(osg::Node *node, osg::NodeVisitor *nv);
	virtual ~Collision();
private:
	quadList listCollsion(const Car *refC, const quadList obs);
	Plane * ifCollide(const osg::ref_ptr<osg::Vec3Array> planeMove , const quadList refObs);
	bool detectFirst(const osg::ref_ptr<osg::Vec3Array> planeMove, const Plane *planeObS);
	bool detectSecond(const osg::ref_ptr<osg::Vec3Array> planeMove, const Plane *planeObs);
	bool detectFinal(const osg::ref_ptr<osg::Vec3Array> planeMove, Plane *planeObs);

	quadList listRoadQuad(const Car *refC, const solidList road);
	Plane * locateCar(Plane::reverse_across_iterator begin, Plane::reverse_across_iterator end, const Point pMove);
	bool detectLocation(const Point pMove , const Plane *planeRoad);

	solidList _wall;
	quadList _wall_reduced;
};

