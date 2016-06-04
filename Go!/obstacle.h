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
	void createNode(const std::string file, const osg::Vec3d &CENTER = H_POINT);
	void createPoint(const osg::Vec3d &center, const double &PS = 10.0f);
	inline void setImage(osg::ref_ptr<osg::Image> img) { _imgTexture = img; };

	inline osg::Vec3d getDistancetoCar() const { return _distanceVector; };
	inline void setDistancetoCar(const osg::Vec3d &ref) const { _distanceVector = ref; };

	void multiplyMatrix(const osg::Matrixd &m, const bool substitute = true);
protected:
	virtual ~Obstacle();

private:
	void sweep(const double height);
	void sweep(osg::ref_ptr<osg::Vec3dArray> swArray);
	void genBoxTexture();
	void genTextureNoZ(const bool perGmtry = false);
//	void genUnifiedTexture();

	osg::ref_ptr<osg::Node> _objNode;
	mutable osg::Vec3d _distanceVector;
};