#pragma once
#include "obstacle.h"
class OpticFlow :
	public Obstacle
{
public:
	OpticFlow();
	virtual ~OpticFlow();
	void createOpticFlow(osg::Array *points, const int mode = 0, const double size = 0, const int segments = 0);
	inline void setPointsColorArray(osg::ref_ptr<osg::Vec4Array> color) { _pointsColorArray = color.release(); };

	inline void setFrameCounts(const unsigned &ref) { _frameCounts = ref; };
	inline unsigned getFrameCounts() const { return _frameCounts; };

	inline osg::ref_ptr<osg::Vec3Array> getPointsArray() { return _points; };
	inline const unsigned & getPolyNumber() const { return _polyNumbers; };

	inline const osg::Geometry::PrimitiveSetList & getPrimSetList() const { return _primList; };

	static inline osg::ref_ptr<osg::Vec3Array> getSpherePointsArray(osg::Vec3Array *p, const double radius, const int segments)
	{
		osg::ref_ptr<osg::Vec3Array> solid = generateSphereSolidVertex(p, radius, segments);
		osg::ref_ptr<osg::Vec3Array> points = generateSphereVertex(p, solid);

		return points.release();
	}

private:
	void createGLPOINTS(osg::Vec3Array *p);
	void createSpherePoly(osg::Vec3Array *p, osg::Vec3Array *vertex, osg::Vec3Array *solid, const int segments);
	static osg::ref_ptr<osg::Vec3Array> generateSphereSolidVertex(osg::Vec3Array *p, const double radius, const int segments);
	static osg::ref_ptr<osg::Vec3Array> generateSphereVertex(osg::Vec3Array *p, osg::Vec3Array *solid);
	void createCubePoly(osg::Vec3Array *p, const double radius);
	void createSPhereSolid(osg::Vec3Array *p, const double radius);
	osg::ref_ptr<osg::Vec4Array> _pointsColorArray;
	osg::ref_ptr<osg::Vec3Array> _points;
	unsigned _frameCounts;
	unsigned _polyNumbers;

	osg::Geometry::PrimitiveSetList _primList;
};