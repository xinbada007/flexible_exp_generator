#include "stdafx.h"
#include "edge.h"
#include "points.h"
#include "halfedge.h"

#include <osg/Notify>

Edge::Edge():
_next(NULL), _prev(NULL), _he1(NULL), _he2(NULL), _eFlag(NULL)
{
}

Edge::~Edge()
{
	osg::notify(osg::DEBUG_INFO) << "deleting Edge..." << std::endl;
// 	delete _eFlag;
// 	_eFlag = NULL;
}

void Edge::addHEtoEdge(HalfEdge *refHE1, HalfEdge *refHE2)
{
	unsigned refP1 = refHE1->getPoint()->getIndex();
	unsigned refP2 = refHE2->getPoint()->getIndex();

	_he1 = (refP1 > refP2) ? refHE1 : refHE2;
	_he2 = (_he1 == refHE1) ? refHE2 : refHE1;

	_he1->setEdge(this);
	_he2->setEdge(this);
}

bool Edge::isValid() const
{
	if ((!_he1) || (!_he2))
	{
		osg::notify(osg::FATAL) << "Edge is invalid!" << std::endl;
		return false;
	}
	if ((_he1->getEdge() != this) || (_he2->getEdge() != this))
	{
		osg::notify(osg::FATAL) << "Edge and HalfEdge is inconsistent!" << std::endl;
		return false;
	}

	return true;
}