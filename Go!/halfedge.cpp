#include "stdafx.h"
#include "halfedge.h"
#include "points.h"
#include "loop.h"
#include "edge.h"

#include <osg/Notify>

HalfEdge::HalfEdge(Points *newP) :
_next(this), _prev(this), _homeE(NULL), _startP(newP), _homeL(NULL)
{
	if (!_startP->getHE())
	{
		_startP->setHE(this);
	}
}

HalfEdge::~HalfEdge()
{
	//osg::notify(osg::NOTICE) << "deleting HE..." << std::endl;
}

bool HalfEdge::isValid() const
{
	if ((!_startP) || (!_homeL) || (!_homeE))
	{
		osg::notify(osg::FATAL) << "HE is not valid" << std::endl;

		osg::notify(osg::FATAL) << "_startP:\t" << _startP << std::endl;
		osg::notify(osg::FATAL) << "_homeL:\t" << _homeL << std::endl;
		osg::notify(osg::FATAL) << "_homeE:\t" << _homeE << std::endl;
		osg::notify(osg::FATAL) << "_next:\t" << _next << std::endl;
		osg::notify(osg::FATAL) << "_prev:\t" << _prev << std::endl;

		return false;
	}

	bool valid(false);
	valid = (this->getEdge()->isValid()) && (this->getPoint()->isValid());

	if (!valid)
	{
		osg::notify(osg::FATAL) << "Edge OR POINT is not valid" << std::endl;
	}
	return valid;
}