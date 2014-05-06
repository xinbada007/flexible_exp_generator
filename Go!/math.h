#pragma once
#include <osg/Array>
#include <osg/Matrix>
#include <osg/BoundingBox>
#include <vector>

const double PI = 3.14159265358979323846;
const double PI_2 = 1.57079632679489661923;
const double PI_4 = 0.78539816339744830962;

const double TO_RADDIAN = 0.01745329251994329577;

const osg::Vec3 X_AXIS(1.0f, 0.0f, 0.0f);
const osg::Vec3 Y_AXIS(0.0f, 1.0f, 0.0f);
const osg::Vec3 Z_AXIS(0.0f, 0.0f, 1.0f);
const osg::Vec3 O_POINT(0.0f, 0.0f, 0.0f);

const osg::Vec3 UP_DIR = Y_AXIS;

const double eps = 1e-8;
const double eps_10 = 10 * eps;
const double eps_100 = 100 * eps;
const double eps_1000 = 1000 * eps;

const double frameRate = 60.0;

extern double z_deepest;

double getZDeepth();

osg::Vec3Array * project_Line(const osg::Vec3Array *refV, const double width);
osg::Vec3 find_Prependicular(const osg::Vec3 p0, const osg::Vec3 p1, const double width);

osg::Matrix rotate(const osg::Vec3Array *source, const osg::Vec3Array *des);
void arrayByMatrix(osg::Vec3Array *source, const osg::Matrix refM);
osg::ref_ptr<osg::Vec3Array> arrayLinearTransform(const osg::Vec3Array *A, const osg::Vec3Array *B, const double lamida);
osg::BoundingBox BoundingboxByMatrix(const osg::BoundingBox &refBB, const osg::Matrix &refM);

typedef std::vector<double> planeEQU;
typedef osg::Vec3 Point;

double ifPoints_ON_Plane(const Point refP, const planeEQU refEQU);
bool isEqual(double a,double p,const double e = eps);

osg::ref_ptr<osg::Vec3Array> linetoRectangle(osg::ref_ptr<osg::Vec3Array> line);
bool ifRectangleOverlap(osg::ref_ptr<osg::Vec3Array> rectA , osg::ref_ptr<osg::Vec3Array> rectB);
bool ifLineIntersects(osg::ref_ptr<osg::Vec3Array> lineA , osg::ref_ptr<osg::Vec3Array> lineB);

bool ifPoint_IN_Polygon(Point refP, osg::ref_ptr<osg::Vec3Array> refPoly, const planeEQU refEQU);

struct searchIndex
{
	searchIndex(unsigned ref) :_index(ref){};
	template <typename T>
	bool operator()(T *ref)
	{
		return (ref->getIndex() == _index);
	};
private:
	unsigned _index;
};