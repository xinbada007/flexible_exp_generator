#pragma once
#include "eulerPoly.h"
#include "road.h"

class Obstacle :
	public EulerPoly
{
public:
	Obstacle();
	virtual ~Obstacle();

	void createBox(osg::Vec3d center, osg::Vec3d radius);

private:
	void sweep(const int height);
	ROADTAG _tag;
};

