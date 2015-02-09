#include "stdafx.h"
#include "renderVistor.h"
#include "road.h"
#include "plane.h"
#include "edge.h"

#include <osg/Notify>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/PolygonMode>
#include <osg/linewidth>
#include <osg/Point>

RenderVistor::RenderVistor():
osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
	_beginMode = GL_POINTS;
	_numLim = 0;
	_candidate = GL_POINTS;
}


RenderVistor::~RenderVistor()
{
}

void RenderVistor::reset()
{
	/*
	Method to call to reset visitor.

	Useful if your visitor accumulates state during a traversal, and you plan to reuse the visitor. 
	To flush that state for the next traversal: call reset() prior to each traversal.

	*/

	_drawableList.clear();
	_beginMode = GL_POINTS;
	_numLim = 0;
	_candidate = GL_POINTS;
}

void RenderVistor::apply(osg::Group &refNode)
{
	//osg::notify(osg::NOTICE) << refNode.libraryName() << "::" << refNode.className() << std::endl;

	osg::ref_ptr<Solid> refS = dynamic_cast<Solid*> (&refNode);
	if (refS)
	{
		if (!refS->isValid()) { osg::notify(osg::FATAL) << "Solid is invalid!" << std::endl; };
		refS->traverse();

		//set stateset
		osg::ref_ptr<osg::StateSet> stateSet = refS->getOrCreateStateSet();
		setStateset(stateSet);
	}

	//continuing traversing sub-scene
	traverse(refNode);
}

// void RenderVistor::apply(osg::Geode &refGeode)
// {
// 	//osg::notify(osg::NOTICE) << refGeode.libraryName() << "::" << refGeode.className() << std::endl;
// 
// 	//pre-process before traverse
// 	osg::ref_ptr<Plane> refP = dynamic_cast<Plane*> (&refGeode);
// 	if (refP)
// 	{
// 		refP->traverse();
// 
// 		//actually render the objects 
// 		_drawableList = refP->getDrawableList();
// 		draw();
// 	}
// 
// 	traverse(refGeode);
// }

void RenderVistor::apply(osg::Geode &refGeode)
{
	//pre-process before traverse
	_drawableList = refGeode.getDrawableList();
	draw();

	traverse(refGeode);
}

void RenderVistor::setStateset(osg::ref_ptr<osg::StateSet> stateSet)
{
	osg::ref_ptr<osg::StateSet> ss = stateSet;

	ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);
	osg::ref_ptr<osg::PolygonMode> pm = new osg::PolygonMode(osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE);
	ss->setAttribute(pm);
}

void RenderVistor::draw() const
{
	if (_drawableList.empty())
	{
		return;
	}

	osg::Geode::DrawableList::const_iterator i = _drawableList.begin();
	
	do 
	{
		render(*i);
		i++;
	} while (i != _drawableList.end());
}

void RenderVistor::render(osg::Drawable *refD) const
{
	osg::Geometry *refG = dynamic_cast<osg::Geometry*>(refD);

	if (refG)
	{
		osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
		color->push_back(osg::Vec4(1.0, 1.0f, 1.0f, 1.0f));
		refG->setColorArray(color);
		refG->setColorBinding(osg::Geometry::BIND_OVERALL);

		osg::ref_ptr<osg::DrawArrays> drawArray;
		if (!_numLim || refG->getVertexArray()->getNumElements() <= _numLim)
		{
			drawArray = new osg::DrawArrays(_beginMode, 0, refG->getVertexArray()->getNumElements());
		}
		else
		{
			if (refG->getVertexArray()->getNumElements() > _numLim)
			{
				drawArray = new osg::DrawArrays(_candidate, 0, refG->getVertexArray()->getNumElements());
			}
		}
		
		if (_beginMode == GL_POINTS)
		{
			osg::ref_ptr<osg::Point> psize =new osg::Point(10.0f);
			refG->getOrCreateStateSet()->setAttribute(psize);
		}
		if (!refG->getNumPrimitiveSets())
		{
			refG->addPrimitiveSet(drawArray);
		}
		else
		{
			refG->setPrimitiveSet(0, drawArray);
		}
	}
}