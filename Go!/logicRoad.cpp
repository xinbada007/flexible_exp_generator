#include "stdafx.h"
#include "LogicRoad.h"
#include "points.h"
#include "halfedge.h"
#include "edge.h"

#include <osg/Notify>
#include <osg/Switch>
#include <osg/PolygonMode>

#include<iterator>

LogicRoad::LogicRoad():
_next(NULL), _prev(NULL), _tag(ROAD),
_roadTxt(""), _width(-1), _density(-1), _eFlag(NULL)
{
	_project_LineV = new osg::Vec3dArray;
}

LogicRoad::LogicRoad(const LogicRoad &copy, osg::CopyOp copyop /* = osg::CopyOp::SHALLOW_COPY */):
EulerPoly(copy, copyop), _next(copy._next), _prev(copy._prev), _tag(copy._tag), _project_LineV(copy._project_LineV),
_roadTxt(copy._roadTxt), _width(copy._width), _density(copy._density), _eFlag(copy._eFlag),
_eFlagArray(copy._eFlagArray)
{

}

LogicRoad::~LogicRoad()
{
	osg::notify(osg::DEBUG_INFO) << "Deconstruct logicRoad:\t" << this->_tag << std::endl;

	_eFlag = NULL;
	std::vector<osg::ref_ptr<osg::Vec3dArray>>::iterator edge_i = _eFlagArray.begin();
	while (edge_i != _eFlagArray.end())
	{
		*edge_i = NULL;
		++edge_i;
	}

	_project_LineV = NULL;
	_next = NULL;
	_prev = NULL;
}

void LogicRoad::line1D(const osg::Vec3dArray *V1, const osg::Vec3dArray *V2)
{
	if (V1->size() != V2->size())
	{
		osg::notify(osg::FATAL) << "Cannot Linke V1 and V2!" << std::endl;
		return;
	}

	osg::Vec3dArray::const_iterator i = V1->begin();
	osg::Vec3dArray::const_iterator j = V2->begin();
	while (i != V1->end() && j != V2->end())
	{
		link(*i++, *j++);
	}
}

void LogicRoad::line1D(const osg::Vec3dArray *refV)
{
	bool flag = false;

	if (!_eFlagArray.empty())
	{
		FlagEdgeArrayList::const_iterator j = _eFlagArray.cbegin();
		while (j != _eFlagArray.cend())
		{
			if (refV == (*j).get())
			{
				flag = true;
				break;
			}
			j++;
		}
	}

	osg::Vec3dArray::const_iterator i = refV->begin();
	while (++i != refV->end())
	{
		link(*(i - 1),*i);

		if (flag)
		{
			setUpFlag();
		}
	}
}

void LogicRoad::sweep1D(const osg::Vec3dArray *refV)
{
	Solid *refS = dynamic_cast<Solid*> (this);
	if (!refS)
	{
		osg::notify(osg::FATAL) << "Cannot Sweep1D! Solid is not exist!" << std::endl;
		return;
	}
	if (refV->size() != refS->getNumPoints())
	{
		osg::notify(osg::FATAL) << "Cannot Sweep1D! Points are inconsistent!" << std::endl;
		return;
	}

	osg::Vec3dArray::const_iterator i = refV->begin();
	Points *P = refS->getPoint();
	osg::Vec3d refP = P->getPoint();
	link(refP, *i);
	while (++i != refV->end())
	{
		P = P->getNext();
		refP = P->getPoint();
		link(*i, *(i - 1));
		link(*i, refP);
	}
}

void LogicRoad::setUpFlag()
{
	if (_eFlag)
	{
		osg::ref_ptr<edgeFlag> newEFlag = new edgeFlag(*_eFlag);
		this->getEdge()->setEdgeFlag(newEFlag);
		if (newEFlag->_navigateEdge)
		{
			//HE1 always points to a later point
			newEFlag->_navigationArray = new osg::Vec3dArray;
			newEFlag->_navigationArray->push_back(this->getEdge()->getHE1()->getPoint()->getPoint());
			newEFlag->_navigationArray->push_back(this->getEdge()->getHE2()->getPoint()->getPoint());
		}
	}
}

bool LogicRoad::texture()
{
	if (!_solidchildGeode || !_solidchildGeode->getNumDrawables())
	{
		return false;
	}

	osg::Geometry *gmtry = _solidchildGeode->getDrawable(0)->asGeometry();
	if (!gmtry)
	{
		return false;
	}

	gmtry->setTexCoordArray(0,this->getTexCoord());

	return true;
}

void LogicRoad::traverse()
{
	if (_solidchildGeode || _updated)
	{
		return;
	}

	_solidchildGeode = new osg::Geode;
	this->addChild(_solidchildGeode);
	osg::ref_ptr<osg::Geometry> gmtNode = new osg::Geometry;
	_solidchildGeode->addDrawable(gmtNode);

	osg::ref_ptr<osg::Vec3dArray> vArray = new osg::Vec3dArray;

	Plane *drawPL = getPlane();
	while (drawPL && !drawPL->getAbstract())
	{
		Loop *drawLoop = drawPL->getLoop();
		while (drawLoop)
		{
			const osg::ref_ptr<osg::Vec3dArray> v = drawLoop->getPoints();
			std::copy(v->begin(), v->end(), std::back_inserter(*vArray));
			drawLoop = drawLoop->getNext();
		}
		drawPL = drawPL->getNext();
	}

	gmtNode->setVertexArray(vArray);

	this->setDataVariance(osg::Object::STATIC);
	_solidchildGeode->setDataVariance(osg::Object::STATIC);
	gmtNode->setDataVariance(osg::Object::STATIC);
	_updated = true;
}