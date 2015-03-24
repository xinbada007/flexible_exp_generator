#include "stdafx.h"
#include "opticFlow.h"
#include <math.h>

#include <osg/ShapeDrawable>
#include <osg/Billboard>
#include <osgUtil/Optimizer>

OpticFlow::OpticFlow() :
_frameCounts(0), _pointsColorArray(NULL)
{
}

OpticFlow::~OpticFlow()
{
	_pointsColorArray = NULL;
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

void OpticFlow::createPolygon(osg::Vec3Array *p, const double radius, const int segments)
{
	const double R(radius);
	osg::ref_ptr<osg::Vec3Array> temp = new osg::Vec3Array;
	for (int i = 0; i < segments; i++)
	{
		const double theta = (double(i) / double(segments)) * 2 * PI;
		const double x = R * cos(theta);
		const double z = 0.0f;
		const double y = R * sin(theta);
		temp->push_back(osg::Vec3(x, y, z));
	}

 	osg::ref_ptr<osg::Vec3Array> solid = new osg::Vec3Array;

// 	for (int i = 0; i < segments; i++)
// 	{
// 		const double theta = (double(i) / double(segments)) * 2 * PI;
// 		osg::ref_ptr<osg::Vec3Array> t = new osg::Vec3Array(temp->begin(), temp->end());
// 		arrayByMatrix(t, osg::Matrix::rotate(theta, osg::Vec3d(0.0f, 0.0f, 1.0f)));
// 		copy(t->begin(), t->end(), std::back_inserter(*solid));
// 	}

	const int SEG(segments / 2);
	for (int i = 1; i <= SEG; i++)
	{
		const double H = (double(i) / double(SEG)) * R;
		const double r = sqrt(R*R - H*H);
		const double ratio = r / R;

		osg::ref_ptr<osg::Vec3Array> t = new osg::Vec3Array(temp->begin(), temp->end());
		osg::Matrix m;
		m = osg::Matrix::translate(osg::Vec3(0.0f, 0.0f, H)) * osg::Matrix::scale(osg::Vec3(ratio, ratio, 1.0f));
		arrayByMatrix(t, m);
		copy(t->begin(), t->end(), std::back_inserter(*solid));

		m = osg::Matrix::translate(osg::Vec3(0.0f, 0.0f, -2*H));
		arrayByMatrix(t, m);
		copy(t->begin(), t->end(), std::back_inserter(*solid));
	}
	copy(temp->begin(), temp->end(), std::back_inserter(*solid));

	osg::ref_ptr<osg::Vec3Array> vertex = new osg::Vec3Array;
	osg::Vec3Array::const_iterator i = p->begin();
	while (i != p->end())
	{
		osg::ref_ptr<osg::Vec3Array> c = new osg::Vec3Array(solid->begin(), solid->end());
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
		unsigned j = 0;
		const int START = (i - p->begin()) * solid->getNumElements();
		while (j != solid->getNumElements() / segments)
		{
			const int index = j;
			const int FIRST = index * segments + START;
			const int COUNT = segments;
			gmtry->addPrimitiveSet(new osg::DrawArrays(GL_LINE_LOOP, FIRST, COUNT));

			++j;
		}
		++i;
	}

	osg::ref_ptr<osg::Geode> geode = new osg::Geode;
	geode->addDrawable(gmtry.release());

	this->addChild(geode.release());
	this->setDataVariance(osg::Object::STATIC);

	osgUtil::Optimizer op;
	op.optimize(this, osgUtil::Optimizer::ALL_OPTIMIZATIONS);
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