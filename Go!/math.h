#pragma once
#include <osg/Array>
#include <osg/Matrix>
#include <osg/BoundingBox>
#include <vector>

#include <math.h>

const double FRUSTUM_Z_MAX = 65535;

const double PI = 3.14159265358979323846;
const double PI_2 = 1.57079632679489661923;
const double PI_4 = 0.78539816339744830962;

const double TO_RADDIAN = 0.01745329251994329577;

const osg::Vec3d X_AXIS(1.0f, 0.0f, 0.0f);
const osg::Vec3d Y_AXIS(0.0f, 1.0f, 0.0f);
const osg::Vec3d Z_AXIS(0.0f, 0.0f, 1.0f);
const osg::Vec3d O_POINT(0.0f, 0.0f, 0.0f);
const osg::Vec3d H_POINT(0.0f, 0.0f, 100.0f);

const osg::Vec3d UP_DIR = Y_AXIS;

const double eps = 1e-8;
const double eps_10 = 10 * eps;
const double eps_100 = 100 * eps;
const double eps_1000 = 1000 * eps;

extern double z_deepest;

double getZDeepth();

osg::Vec3dArray * project_Line(const osg::Vec3dArray *refV, const double width);
osg::Vec3d find_Prependicular(const osg::Vec3d p0, const osg::Vec3d p1, const double width);

osg::Matrix rotate(const osg::Vec3dArray *source, const osg::Vec3dArray *des);
void arrayByMatrix(osg::Vec3dArray *source, const osg::Matrix refM);
void arrayByMatrix(osg::Vec3Array *source, const osg::Matrix refM);
osg::ref_ptr<osg::Vec3dArray> arrayLinearTransform(const osg::Vec3dArray *A, const osg::Vec3dArray *B, const double lamida);
osg::BoundingBox BoundingboxByMatrix(const osg::BoundingBox &refBB, const osg::Matrix &refM);

typedef std::vector<double> planeEQU;
typedef osg::Vec3d Point;

double ifPoints_ON_Plane(const Point refP, const planeEQU refEQU);
bool isEqual(const double &a,const double &p,const double &e = eps);
bool isEqual(const osg::Vec3d &p1, const osg::Vec3d &p2, const double &e = eps);

osg::ref_ptr<osg::Vec3dArray> linetoRectangle(osg::ref_ptr<osg::Vec3dArray> line);
bool ifRectangleOverlap(osg::ref_ptr<osg::Vec3dArray> rectA , osg::ref_ptr<osg::Vec3dArray> rectB);
bool ifLineIntersects(osg::ref_ptr<osg::Vec3dArray> lineA , osg::ref_ptr<osg::Vec3dArray> lineB);

bool ifPoint_IN_Polygon(Point refP, osg::ref_ptr<osg::Vec3dArray> refPoly, const planeEQU refEQU);

bool isletter(const char c);

bool isDouble(const std::string &ref, double *number);

double acosR(double product);
double asinR(double product);

void getCCWCubefromLBfromBBox(const osg::BoundingBoxd &bb, osg::ref_ptr<osg::Vec3dArray> Zmin, osg::ref_ptr<osg::Vec3dArray> Zmax);

template <typename T> unsigned normalizeArrayLength(T *v, const unsigned &S)
{
	const unsigned numPos = v->size();

	if (numPos != S)
	{
		v->resize(S);
		if (S > numPos)
		{
			T::iterator assign_start = v->begin();
			T::iterator assign_end = assign_start + numPos;
			T::iterator i = v->begin() + numPos;
			while (i != v->end())
			{
				if (assign_start == assign_end)
				{
					assign_start = v->begin();
				}
				*i = *assign_start;
				++i;
				++assign_start;
			}
		}
	}

	return numPos < v->size() ? numPos : v->size();
}

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

template <typename T>
struct uniqueTest
{
	uniqueTest(const T uni) :_uni(uni){};
	bool operator()(const T A, const T B)
	{
		return (A == _uni && B == _uni);
	}
private:
	const T _uni;
};

class frameRate:
	public osg::Referenced
{
	friend class MulitViewer;
public:
	static frameRate *instance();
	double getDesignfRate() const { return _designfRate; };
	void setRealfRate(double rate) { _realfRate = rate;};
	double getRealfRate() const { return _realfRate; };
	unsigned getFrameIndex() const { return _frameIndex; };
	void setFrameIndex(unsigned fs) { _frameIndex = fs; };
private:
	void setDesignedfRate(double ref) { _designfRate = ref; _realfRate = _designfRate; };
	frameRate() :_designfRate(60.0f), _realfRate(_designfRate),_frameIndex(0) {};
	~frameRate(){};
	double _designfRate;
	double _realfRate;
	unsigned _frameIndex;
};