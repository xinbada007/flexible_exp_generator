#pragma once
#include "eulerPoly.h"
#include "logicroad.h"

class Obstacle :
	public LogicRoad
{
public:
	Obstacle();
	Obstacle(const Obstacle &copy, osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY);
	META_Node(Obstacle, Obstacle);
	void createBox(osg::Vec3d center, osg::Vec3d radius);
	inline void setImage(osg::ref_ptr<osg::Image> img) { _imgTexture = img; };

private:
	virtual ~Obstacle();
	void sweep(const int height);
	void genBoxTexture();
};

