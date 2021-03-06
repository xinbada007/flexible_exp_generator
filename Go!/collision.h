#pragma once
#include <osg/NodeCallback>

#include "solid.h"
#include "car.h"
#include "obstacle.h"

class Collision:public osg::NodeCallback
{
public:
	Collision();
	void operator()(osg::Node *node, osg::NodeVisitor *nv);
	virtual ~Collision();

private:
	quadList listCollsion(const Car *refC, const quadList obs);

	obstacleList listObsCollsion(const Car *refC, const obstacleList obs);
	bool ifObsCollide(const Car *refC, const Obstacle *obs);
	bool detectObsDistance(const Car *refC, const Obstacle *obs, double *distance);

	Plane * ifCollide(const osg::ref_ptr<osg::Vec3dArray> planeMove , const quadList refObs);
	bool detectFirst(const osg::ref_ptr<osg::Vec3dArray> planeMove, const Plane *planeObS);
	bool detectSecond(const osg::ref_ptr<osg::Vec3dArray> planeMove, const Plane *planeObs);
	bool detectFinal(const osg::ref_ptr<osg::Vec3dArray> planeMove, Plane *planeObs);

	quadList listRoadQuad(const Car *refC, const solidList road);
	Plane * locateCar(Plane::reverse_across_iterator begin, Plane::reverse_across_iterator end, const Point pMove);
	bool detectLocation(const Point pMove , const Plane *planeRoad);

	solidList _wall;
	quadList _wall_reduced;
};

