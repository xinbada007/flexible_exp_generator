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
	void createCylinder(const osg::Vec3d &center, const double &radius, const double &height);
	void createSphere(const osg::Vec3d &centre, const double &radius);
	inline void setImage(osg::ref_ptr<osg::Image> img) { _imgTexture = img; };

private:
	virtual ~Obstacle();
	void sweep(const double height);
	void sweep(osg::ref_ptr<osg::Vec3dArray> swArray);
	void genBoxTexture();
	void genTextureNoZ();
	void genUnifiedTexture();
};

