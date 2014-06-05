#include "stdafx.h"
#include "solid.h"
#include "plane.h"
#include "edge.h"
#include "points.h"
#include "halfedge.h"

Solid::Solid():
_next(NULL), _prev(NULL), _updated(false), _numPoints(0), _numPlanes(0),
_startPlane(NULL), _startE(NULL), _startP(NULL), _texCoord(NULL), _imgTexture(NULL),
_lastPlane(NULL), _index(0)
{

}

Solid::Solid(const Solid &copy, osg::CopyOp copyop /* = osg::CopyOp::SHALLOW_COPY */) :
osg::Group(copy,copyop),
_next(copy._next), _prev(copy._prev), _updated(copy._updated), _numPoints(copy._numPoints), _numPlanes(copy._numPlanes),
_startPlane(copy._startPlane), _startE(copy._startE), _startP(copy._startP), _texCoord(copy._texCoord), _imgTexture(copy._imgTexture),
_lastPlane(copy._lastPlane), _index(copy._index)
{

}

Solid::~Solid()
{
	osg::notify(osg::NOTICE) << "deleting Solid...\t" << std::endl;
	Edge *refE = _startE;
	if (refE)
	{
		Edge *refNXE = refE->getNext();
		do 
		{
			HalfEdge *HE = refE->getHE1();
			delete HE;
			HE = refE->getHE2();
			delete HE;
			delete refE;
			refE = refNXE;
			if (refNXE)
			{
				refNXE = refNXE->getNext();
			}
		} while (refE);
	}
	_startE = NULL;
// 
	Points *refP = _startP;
	if (refP)
	{
		Points *refNXP = refP->getNext();
		do 
		{
			delete refP;
			refP = refNXP;
			if (refNXP)
			{
				refNXP = refNXP->getNext();
			}
		} while (refP);
	}
	_startP = NULL;
}

Plane * Solid::getAbstract() const
{
	Plane *refPL = _startPlane;
	do
	{
		if (refPL->getAbstract())
		{
			return refPL;
		}
		refPL = refPL->getNext();
	} while (refPL);

	return NULL;
}

void Solid::addPlanetoList(Plane *refPL)
{
	bool lastFlag = (_startPlane) ? false : true;

	refPL->setNext(_startPlane);
	refPL->setPrev(NULL);
	if (!lastFlag)
	{
		_startPlane->setPrev(refPL);
	}
	_startPlane = refPL;
	if (lastFlag)
	{
		_lastPlane = _startPlane;
	}
	refPL->setHomeS(this);
	refPL->setIndex(_numPlanes);
	_numPlanes++;
}

void Solid::addEdgetoList(Edge *refE)
{
	refE->setNext(_startE);
	refE->setPrev(NULL);
	if (_startE)
	{
		_startE->setPrev(refE);
	}
	_startE = refE;
}

void Solid::addPointtoList(Points *refP)
{
	refP->setNext(_startP);
	refP->setPrev(NULL);
	if (_startP)
	{
		_startP->setPrev(refP);
	}
	_startP = refP;
	_numPoints++;
	refP->setIndex(_numPoints);
}

void Solid::traverse()
{
	if (_updated)
	{
		return;
	}

	Plane * drawPL = _startPlane;

	while (drawPL && !drawPL->getAbstract())
	{
		this->addChild(drawPL->asGeode());
		drawPL = drawPL->getNext();
	}

	_updated = true;
}

bool Solid::isValid()
{
	if ((!_startPlane)||(!_startE)||(!_startP))
	{
		osg::notify(osg::FATAL) << "Solid is not valid" << std::endl;
		return false;
	}
	if (_startPlane->getHomeS() != this)
	{
		osg::notify(osg::FATAL) << "Solid and Plane is inconsistent!" << std::endl;
		return false;
	}
	if (!(this->getNumPoints()))
	{
		osg::notify(osg::FATAL) << "Solid has no points at all!" << std::endl;
		return false;
	}
	if (getAbstract())
	{
		if (_lastPlane != getAbstract())
		{
			osg::notify(osg::FATAL) << "Solid and Abstarct Plane is inconsistent!" << std::endl;
			return false;
		}
	}

	Plane *refPL = _startPlane;
	bool valid(false);
	do 
	{
		valid = refPL->isValid();
		refPL = refPL->getNext();
	} while (valid && refPL);
	
	return valid;
}

Points * Solid::findPoint(osg::Vec3d refP)
{
	Points * objP = _startP;
	do 
	{
		if (objP->isEqual(refP))
		{
			return objP;
		}
		objP = objP->getNext();
	} while (objP);

	return NULL;
}