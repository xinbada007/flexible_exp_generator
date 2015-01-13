#include "stdafx.h"
#include "solid.h"
#include "plane.h"
#include "edge.h"
#include "points.h"
#include "halfedge.h"
#include "loop.h"

#include <osg/MatrixTransform>

Solid::Solid():
_next(NULL), _prev(NULL), _updated(false), _numPoints(0), _numPlanes(0),
_startPlane(NULL), _startE(NULL), _startP(NULL), _texCoord(NULL), _imgTexture(NULL),
_lastPlane(NULL), _index(0), _texMode(false), _ccw(true), _ccwupdated(false), _maxAnisotropy(1.0f), _solidType(Solid::solidType::SD_UNDEFINED)
{

}

Solid::Solid(const Solid &copy, osg::CopyOp copyop /* = osg::CopyOp::SHALLOW_COPY */) :
osg::Switch(copy,copyop),
_next(copy._next), _prev(copy._prev), _updated(copy._updated), _numPoints(copy._numPoints), _numPlanes(copy._numPlanes),
_startPlane(copy._startPlane), _startE(copy._startE), _startP(copy._startP), _texCoord(copy._texCoord), _imgTexture(copy._imgTexture),
_lastPlane(copy._lastPlane), _index(copy._index), _texMode(copy._texMode), _ccw(copy._ccw), _ccwupdated(copy._ccwupdated),
_maxAnisotropy(copy._maxAnisotropy), _solidType(copy._solidType)
{

}

Solid::~Solid()
{
	osg::notify(osg::DEBUG_INFO) << "deleting Solid...\t" << std::endl;
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

Edge * Solid::findEdge(const osg::Vec3d &p1, const osg::Vec3d &p2)
{
	Edge *e = _startE;

	for (; e; e = e->getNext())
	{
		HalfEdge *he1 = e->getHE1();
		HalfEdge *he2 = e->getHE2();

		const osg::Vec3d &phe1 = he1->getPoint()->getPoint();
		const osg::Vec3d &phe2 = he2->getPoint()->getPoint();

		if (phe1 == p1 && phe2 == p2)
		{
			return e;
		}
		else if (phe1 == p2 && phe2 == p1)
		{
			return e;
		}
	}

	return NULL;
}

Loop * Solid::findLoop(const Edge *e1, const Edge *e2)
{
	Loop *le1he1 = e1->getHE1()->getLoop();
	Loop *le1he2 = e1->getHE2()->getLoop();

	Loop *le2he1 = e2->getHE1()->getLoop();
	Loop *le2he2 = e2->getHE2()->getLoop();

	Loop *shared(NULL);
	if (le1he1 == le2he1 || le1he1 == le2he2)
	{
		shared = le1he1;
	}
	else if (le1he2 == le2he1 || le1he2 == le2he2)
	{
		shared = le1he2;
	}

	return shared;
}

Loop * Solid::findLoop(const osg::Vec3d &p1, const osg::Vec3d &p2)
{
	Plane *p = _startPlane;
	Loop *l = p->getLoop();

	for (; p; p = p->getNext())
	{
		if (l->ifPointinLoop(p1) && l->ifPointinLoop(p2))
		{
			return l;
		}
		l = p->getLoop();
	}

	return NULL;
}

bool Solid::ifPointinSolid(const osg::Vec3d p) const
{
	if (getAbstract())
	{
		return false;
	}

	if (!_ccwupdated)
	{
		caclCCW();
	}

	Plane::reverse_iterator i = _lastPlane;
	while (*i)
	{
		double sign = (*i)->getLoop()->substituePointinEQU(p);
		if (_ccw && sign > 0)
		{
			return false;
		}
		if (!_ccw && sign < 0)
		{
			return false;
		}
		i++;
	}

	return true;
}

void Solid::caclCCW() const
{
	planeEQU &e = getLastPlane()->getLoop()->getPlaneEQU();
	osg::Vec3d v(e[0], e[1], e[2]);
	v.normalize();

	bool outside(false);

	if (isEqual(v*X_AXIS, 1.0f))
	{
		if (v.x() > 0)
		{
			outside = true;
		}
	}
	else if (isEqual(v*Y_AXIS, 1.0f))
	{
		if (v.y() > 0)
		{
			outside = true;
		}
	}
	else if (isEqual(v*Z_AXIS, 1.0f))
	{
		if (v.z() > 0)
		{
			outside = true;
		}
	}
	else
	{
		outside = (v*Z_AXIS > 0) ? true : false;
	}

	_ccw = outside;
	_ccwupdated = true;
}

void Solid::multiplyMatrix(const osg::Matrixd &m)
{
	osg::MatrixTransform *mt = dynamic_cast<osg::MatrixTransform*>(this->getParent(0));
	if (mt)
	{
		mt->setMatrix(m);

		Points *p = _startP;
		while (p)
		{
			p->setPoint(p->getPoint() * m);
			p = p->getNext();
		}
	}

	else
	{
		Plane *pl = _startPlane;
		while (pl)
		{
			pl->multiplyMatrix(m);
			pl = pl->getNext();
		}
	}

	absoluteTerritory.center = absoluteTerritory.center * m;
}

void Solid::createGLPOINTS(const osg::Vec3d &p)
{
	this->solidType = GL_POINTS_BODY;

	osg::ref_ptr<osg::Geode> GLP = new osg::Geode;
	
	osg::ref_ptr<osg::Geometry> GLgeomtry = new osg::Geometry;
	osg::ref_ptr<osg::Vec3dArray> v = new osg::Vec3dArray;
	v->push_back(p);
	GLgeomtry->setVertexArray(v.release());
	osg::ref_ptr<osg::Vec4dArray> color = new osg::Vec4dArray;
	color->push_back(osg::Vec4d(1.0f,1.0f,1.0f,1.0f));
	GLgeomtry->setColorArray(color.release());
	GLgeomtry->setColorBinding(osg::Geometry::BIND_OVERALL);
	GLgeomtry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, 1));
	osg::BoundingBox bbox(-.1f, -.1f, -.1f, .1f, .1f, .1f);
	GLgeomtry->setInitialBound(bbox);
	
	GLP->addDrawable(GLgeomtry.release());
	this->addChild(GLP.release());
}

void Solid::createGLPOINTS(osg::ref_ptr<osg::Vec3dArray> p)
{
	osg::ref_ptr<osg::Vec3dArray> vertex = new osg::Vec3dArray(p->begin(), p->end());

	this->solidType = GL_POINTS_BODY;

	osg::ref_ptr<osg::Geode> GLP = new osg::Geode;

	osg::ref_ptr<osg::Geometry> GLgeomtry = new osg::Geometry;
	GLgeomtry->setVertexArray(vertex);
	osg::ref_ptr<osg::Vec4dArray> color = new osg::Vec4dArray;
	color->push_back(osg::Vec4d(1.0f, 1.0f, 1.0f, 1.0f));
	GLgeomtry->setColorArray(color.release());
	GLgeomtry->setColorBinding(osg::Geometry::BIND_OVERALL);
	
	GLgeomtry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, GLgeomtry->getVertexArray()->getNumElements()));

	GLgeomtry->setDataVariance(osg::Object::DYNAMIC);

	GLP->addDrawable(GLgeomtry.release());
	this->addChild(GLP.release());
}