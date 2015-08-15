#include "stdafx.h"
#include "opticFlow.h"
#include <math.h>

#include <osg/ShapeDrawable>
#include <osg/Billboard>
#include <osgUtil/Optimizer>

OpticFlow::OpticFlow() :
_frameCounts(0), _pointsColorArray(NULL), _points(NULL), _polyNumbers(0)
{
}

OpticFlow::~OpticFlow()
{
	_pointsColorArray = NULL;
	_points = NULL;
}

void OpticFlow::createOpticFlow(osg::Array *points, const int mode /* = 0 */, const double size /* = 0 */, const int segments /* = 0 */)
{
	osg::Vec3Array *p = dynamic_cast<osg::Vec3Array*>(points);
	if (p)
	{
		if (!mode)
		{
			createGLPOINTS(p);
		}
		else if (mode == 1)
		{
			osg::ref_ptr<osg::Vec3Array> solid = generateSphereSolidVertex(p, size, segments);
			osg::ref_ptr<osg::Vec3Array> vertex = generateSphereVertex(p, solid);
			createSpherePoly(p, vertex, solid, segments);

// 			createSPhereSolid(p, size);
		}
		else if (mode == 2)
		{
			createCubePoly(p, size);
		}
	}
}

void OpticFlow::createGLPOINTS(osg::Vec3Array *p)
{
	osg::ref_ptr<osg::Vec3Array> vertex = new osg::Vec3Array(p->begin(), p->end());

	this->setSolidType(Solid::solidType::GL_POINTS_BODY);
	this->setTag(ROADTAG::RT_UNSPECIFIED);

	osg::ref_ptr<osg::Geode> GLP = new osg::Geode;

	osg::ref_ptr<osg::Geometry> GLgeomtry = new osg::Geometry;
	GLgeomtry->setVertexArray(vertex);
	osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
	color->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
	if (_pointsColorArray)
	{
		color = _pointsColorArray;
	}
	GLgeomtry->setColorArray(color.release());
	GLgeomtry->setColorBinding(osg::Geometry::BIND_OVERALL);

	GLgeomtry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertex->getNumElements()));

	//TEST
	const int Num = (double) 1 / 0.005f + 0.5f;
	const int STEP = (double) vertex->getNumElements() / Num + 0.5f;
	_primList.clear();
	for (int i = 0; i < Num; i++)
	{
		_primList.push_back(new osg::DrawArrays(GL_POINTS, i * STEP, STEP));
	}
	//TEST

	GLP->addDrawable(GLgeomtry.release());
	this->addChild(GLP.release());

	_points = vertex;
	p = NULL;
}

void OpticFlow::createSpherePoly(osg::Vec3Array *p, osg::Vec3Array *vertex, osg::Vec3Array *solid, const int segments)
{
	osg::ref_ptr<osg::Geometry> gmtry = new osg::Geometry;
	gmtry->setVertexArray(vertex);
	osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
	color->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
	if (_pointsColorArray)
	{
		color = _pointsColorArray;
	}
	gmtry->setColorArray(color.release());
	gmtry->setColorBinding(osg::Geometry::BIND_OVERALL);
	gmtry->setDataVariance(osg::Object::STATIC);

	osg::Vec3Array::const_iterator i = p->begin();
	while (i != p->end())
	{
		unsigned j = 0;
		const int START = (i - p->begin()) * solid->getNumElements();
		while (j != solid->getNumElements() / segments)
		{
			const int index = j;
			const int FIRST = index * segments + START;
			const int COUNT = segments;
			gmtry->addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, FIRST, COUNT));

			++j;
		}
		++i;
	}

	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	geode->addDrawable(gmtry.release());
	geode->setDataVariance(osg::Object::STATIC);

	this->addChild(geode.release());
	this->setDataVariance(osg::Object::STATIC);

	osgUtil::Optimizer op;
	op.optimize(this, osgUtil::Optimizer::ALL_OPTIMIZATIONS ^ osgUtil::Optimizer::TRISTRIP_GEOMETRY);

	_polyNumbers = solid->getNumElements();
	_points = new osg::Vec3Array(p->begin(), p->end());
	p = NULL;
}

osg::ref_ptr<osg::Vec3Array> OpticFlow::generateSphereVertex(osg::Vec3Array *p,  osg::Vec3Array *solid)
{
	osg::ref_ptr<osg::Vec3Array> vertex = new osg::Vec3Array;
	osg::Vec3Array::const_iterator i = p->begin();
	while (i != p->end())
	{
		osg::ref_ptr<osg::Vec3Array> c = new osg::Vec3Array(solid->begin(), solid->end());
		arrayByMatrix(c, osg::Matrix::translate(*i));

		copy(c->begin(), c->end(), std::back_inserter(*vertex));
		++i;
	}

	return vertex.release();
}

osg::ref_ptr<osg::Vec3Array> OpticFlow::generateSphereSolidVertex(osg::Vec3Array *p, const double radius, const int segments)
{
	const double R(radius);
	osg::ref_ptr<osg::Vec3Array> temp = new osg::Vec3Array;
	for (int i = 0; i < segments; i++)
	{
		const double theta = (double(i) / double(segments)) * 2 * PI;
		const double x = R * cos(theta);
		const double y = 0.0f;
		const double z = R * sin(theta);
		temp->push_back(osg::Vec3(x, y, z));
	}

	osg::ref_ptr<osg::Vec3Array> solid = new osg::Vec3Array;

	for (int i = 0; i < segments; i++)
	{
		const double theta = (double(i) / double(segments)) * 2 * PI;
		osg::ref_ptr<osg::Vec3Array> t = new osg::Vec3Array(temp->begin(), temp->end());
		arrayByMatrix(t, osg::Matrix::rotate(theta, osg::Vec3d(0.0f, 0.0f, 1.0f)));
		copy(t->begin(), t->end(), std::back_inserter(*solid));
	}

//  const int SEG(segments / 2);
// 	for (int i = 1; i <= SEG; i++)
// 	{
// 		const double H = (double(i) / double(SEG)) * R;
// 		const double r = sqrt(R*R - H*H);
// 		const double ratio = r / R;
// 	
// 		osg::ref_ptr<osg::Vec3Array> t = new osg::Vec3Array(temp->begin(), temp->end());
// 		osg::Matrix m;
// 		m = osg::Matrix::translate(osg::Vec3(0.0f, 0.0f, H)) * osg::Matrix::scale(osg::Vec3(ratio, ratio, 1.0f));
// 		arrayByMatrix(t, m);
// 		copy(t->begin(), t->end(), std::back_inserter(*solid));
// 	
// 		m = osg::Matrix::translate(osg::Vec3(0.0f, 0.0f, -2*H));
// 		arrayByMatrix(t, m);
// 		copy(t->begin(), t->end(), std::back_inserter(*solid));
// 	}
// 	copy(temp->begin(), temp->end(), std::back_inserter(*solid));

	return solid.release();
}

void OpticFlow::createSPhereSolid(osg::Vec3Array *p, const double radius)
{
	osg::ref_ptr<osg::Geode> geode = new osg::Geode;

	osg::Vec3Array::const_iterator i = p->begin();
	while (i != p->end())
	{
		osg::ref_ptr<osg::Sphere> sp = new osg::Sphere(*i, radius);
		osg::ref_ptr<osg::ShapeDrawable> spDrawable = new osg::ShapeDrawable(sp);

		geode->addDrawable(spDrawable.release());

		++i;
	}

	this->addChild(geode.release());

	_points = new osg::Vec3Array(p->begin(), p->end());
	p = NULL;
}

void OpticFlow::createCubePoly(osg::Vec3Array *p, const double radius)
{
	const osg::Vec3 X(radius, 0.0f, 0.0f);
	const osg::Vec3 Y(0.0f, radius, 0.0f);
	const osg::Vec3 Z(0.0f, 0.0f, radius);

	const osg::Vec3 Origin(0.0f, 0.0f, 0.0f);
	const osg::Vec3 RB = Origin + (X*0.5f - Y*0.5f);
	const osg::Vec3 RT = Origin + (X*0.5f + Y*0.5f);
	const osg::Vec3 LB(RB - X);
	const osg::Vec3 LT(RT - X);

	osg::ref_ptr<osg::Vec3Array> temp = new osg::Vec3Array;
	temp->push_back(RB); temp->push_back(LB); temp->push_back(LT); temp->push_back(RT);
	temp->push_back(RT + Z); temp->push_back(LT + Z);
	temp->push_back(LT + Z - Y); temp->push_back(RT + Z - Y);

	osg::ref_ptr<osg::Vec3Array> vertex = new osg::Vec3Array;
	osg::Vec3Array::const_iterator i = p->begin();
	while (i != p->end())
	{
		osg::ref_ptr<osg::Vec3Array> t = new osg::Vec3Array(temp->begin(), temp->end());
		arrayByMatrix(t, osg::Matrix::translate(*i));
		
		copy(t->begin(), t->end(), std::back_inserter(*vertex));

		++i;
	}

	osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
	geom->setVertexArray(vertex);
	osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
	color->push_back(osg::Vec4(1.0f, 1.0f, 1.0f, 1.0f));
	if (_pointsColorArray)
	{
		color = _pointsColorArray;
	}
	geom->setColorArray(color.release());
	geom->setColorBinding(osg::Geometry::BIND_OVERALL);
	geom->setDataVariance(osg::Object::STATIC);

	i = p->begin();
	while (i != p->end())
	{
		const int START = (i - p->begin()) * temp->getNumElements();

		osg::ref_ptr<osg::DrawElementsUInt> de = new osg::DrawElementsUInt(GL_QUAD_STRIP);
// 		de->push_back(0 + START); de->push_back(1 + START); 
// 		de->push_back(3 + START); de->push_back(2 + START);
// 		de->push_back(4 + START); de->push_back(5 + START);
// 		de->push_back(7 + START); de->push_back(6 + START);
// 		de->push_back(0 + START); de->push_back(1 + START);
		de->push_back(6 + START); de->push_back(1 + START);
		de->push_back(7 + START); de->push_back(0 + START);
		de->push_back(4 + START); de->push_back(3 + START);
		de->push_back(5 + START); de->push_back(2 + START);
		de->push_back(6 + START); de->push_back(1 + START);

		geom->addPrimitiveSet(de);

		++i;
	}

	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	geode->addDrawable(geom.release());
	geode->setDataVariance(osg::Object::STATIC);

	this->addChild(geode.release());
	this->setDataVariance(osg::Object::STATIC);

	osgUtil::Optimizer op;
	op.optimize(this, osgUtil::Optimizer::ALL_OPTIMIZATIONS ^ osgUtil::Optimizer::TRISTRIP_GEOMETRY);

	_polyNumbers = temp->getNumElements();
	_points = new osg::Vec3Array(p->begin(), p->end());
	p = NULL;
}