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

private:
	void createGLPOINTS(osg::Vec3Array *p);
	void createPolygon(osg::Vec3Array *p, const double radius, const int segments);
	void createBalls(osg::Vec3Array *p, const double radius);
	osg::ref_ptr<osg::Vec4Array> _pointsColorArray;
	unsigned _frameCounts;
};