#include "stdafx.h"
#include "opticFlow.h"
#include <math.h>

#include <osg/ShapeDrawable>
#include <osg/Billboard>

OpticFlow::OpticFlow() :
_frameCounts(0), _pointsColorArray(NULL)
{
}

OpticFlow::~OpticFlow()
{
	_pointsColorArray = NULL;
}

void OpticFlow::createOpticFlow(osg::Array *points, const int mode /* = 0 */, const double size /* = 0 */, const double segments /* = 0 */)
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
			createPolygon(p, size, segments);
		}
		else if (mode == 2)
		{
			createBalls(p, size);
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

	GLP->addDrawable(GLgeomtry.release());
	this->addChild(GLP.release());
}

void OpticFlow::createPolygon(osg::Vec3Array *p, const double radius, const double segments)
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

	osg::ref_ptr<osg::Vec3Array> vertex = new osg::Vec3Array;
	osg::Vec3Array::const_iterator i = p->begin();
	while (i != p->end())
	{
		osg::ref_ptr<osg::Vec3Array> c = new osg::Vec3Array(temp->begin(), temp->end());
		arrayByMatrix(c, osg::Matrix::translate(*i));
		
		copy(c->begin(), c->end(), std::back_inserter(*vertex));
		++i;
	}

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

	i = p->begin();
	while (i != p->end())
	{
		const int index = i - p->begin();
		const int FIRST = index * segments;
		const int COUNT = segments;
		gmtry->addPrimitiveSet(new osg::DrawArrays(GL_POLYGON, FIRST, COUNT));

		++i;
	}

	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	geode->addDrawable(gmtry.release());

	this->addChild(geode.release());
}

void OpticFlow::createBalls(osg::Vec3Array *p, const double radius)
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
}