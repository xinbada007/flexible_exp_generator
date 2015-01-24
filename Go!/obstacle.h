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
	void createPOINTS(osg::ref_ptr<osg::Vec3Array> p);
	void createPOINTS(osg::ref_ptr<osg::Vec3dArray> p);
	inline void setPointsColorArray(osg::ref_ptr<osg::Vec4Array> color) { _pointsColorArray = color.release(); };
	inline void setImage(osg::ref_ptr<osg::Image> img) { _imgTexture = img; };

	inline osg::Vec3d getDistancetoCar() const { return _distanceVector; };
	inline void setDistancetoCar(const osg::Vec3d &ref) const { _distanceVector = ref; };
	inline void setFrameCounts(const unsigned &ref) { _frameCounts = ref; };
	inline unsigned getFrameCounts() const { return _frameCounts; };

private:
	virtual ~Obstacle();
	void sweep(const double height);
	void sweep(osg::ref_ptr<osg::Vec3dArray> swArray);
	void genBoxTexture();
	void genTextureNoZ(const bool perGmtry = false);
//	void genUnifiedTexture();

	mutable osg::Vec3d _distanceVector;
	unsigned _frameCounts;
};

