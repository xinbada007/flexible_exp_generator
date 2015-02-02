#include "stdafx.h"
#include "eulerPoly.h"
#include "points.h"
#include "plane.h"
#include "loop.h"
#include "edge.h"
#include "halfedge.h"

EulerPoly::EulerPoly():
_ccwupdated(false)
{
}

EulerPoly::EulerPoly(const EulerPoly &copy, osg::CopyOp copyop /* = osg::CopyOp::SHALLOW_COPY */):
Solid(copy, copyop), _ccwupdated(copy._ccwupdated)
{

}

EulerPoly::~EulerPoly()
{
	osg::notify(osg::DEBUG_INFO) << "Deconstruct EulerPoly" << std::endl;
}

void EulerPoly::mvfs(osg::Vec3d ref)
{
	Points *newP = new Points(ref);

	Loop *newL = new Loop();

	Plane *newPl = new Plane(newL);
	newPl->setAbstract(true);
	
	osg::ref_ptr<Solid> newS = dynamic_cast<Solid*> (this);
	newS->addPointtoList(newP);
	newS->addPlanetoList(newPl);
}

void EulerPoly::mev(Points *oldP, osg::Vec3d newP)
{
	Solid *refS = dynamic_cast<Solid*> (this); 
	Loop *refL = refS->getAbstract()->getLoop();

	Points *newPn = new Points(newP);
	refS->addPointtoList(newPn);
	Edge *newE = new Edge();
	refS->addEdgetoList(newE);

	HalfEdge *newHE1 = new HalfEdge(newPn);
	HalfEdge *newHE2 = new HalfEdge(oldP);
	newE->addHEtoEdge(newHE1, newHE2);
	refL->addHEtoLoop(newE);
}

void EulerPoly::mef(Points *oldP, Points *newP)
{
	//Note: assume index of P2 > index of P1, that is P2 is more recently created
	Solid *refS = dynamic_cast<Solid*> (this);
	Loop *refL = refS->getAbstract()->getLoop();

	Loop *newL = new Loop();
	Plane *newPL = new Plane(newL);
	refS->addPlanetoList(newPL);
	Edge *newE = new Edge();
	refS->addEdgetoList(newE);

	HalfEdge *newHE1 = new HalfEdge(newP);
	HalfEdge *newHE2 = new HalfEdge(oldP);
	newE->addHEtoEdge(newHE1, newHE2);
	refL->addHEtoLoop(newE);
	refL->splitLoop(newE, newL);

	//set plane's normal and equation
	calcPlaneEQU(newL);
	calcPlaneEQU(refL);

//	Loop *norL = (refL->getHomeP()->getAbstract()) ? newL : refL;
}

Edge * EulerPoly::semv(Edge *mEdge, const osg::Vec3d &newP)
{
	Loop *mLoop = mEdge->getHE1()->getLoop();
	Solid *refS = mLoop->getHomeP()->getHomeS();

	Loop *mHe1L = mEdge->getHE1()->getLoop();
	Loop *mHe2L = mEdge->getHE2()->getLoop();

	Points *newPn = new Points(newP);
	refS->addPointtoList(newPn);
	Edge *newE = new Edge;
	refS->addEdgetoList(newE);

	HalfEdge *newHE1 = new HalfEdge(newPn);
	newE->addHEtoEdge(newHE1, mEdge->getHE1());
	mHe2L->addHEtoLoop(newE);
	HalfEdge *newHE1_1 = new HalfEdge(newPn);
	mEdge->addHEtoEdge(newHE1_1,mEdge->getHE2());
	mHe1L->addHEtoLoop(mEdge);

	return newE;
}

Edge * EulerPoly::insPt(Edge *mEdge, const osg::Vec3d &newP)
{
	if (!mEdge)
	{
		osg::notify(osg::WARN) << "Error when inserting Points" << std::endl;
		return NULL;
	}

	return semv(mEdge, newP);
}

void EulerPoly::calcPlaneEQU(Loop *refL)
{
	if (!refL)
	{
		return;
	}

	osg::Vec3d p1, p2, p3, p;
	HalfEdge *he1 = refL->getHE();
	p1 = he1->getPoint()->getPoint();
	p = p1;
	HalfEdge *he2 = he1->getNext();
	p2 = he2->getPoint()->getPoint();
	HalfEdge *he3 = he2->getNext();
	p3 = he3->getPoint()->getPoint();

	p3 -= p2;
	p2 -= p1;
	p1 = p2^p3;
	p1.normalize();

	planeEQU equ;
	equ.push_back(p1.x()); equ.push_back(p1.y()); equ.push_back(p1.z());
	double d = -(p1.x()*p.x() + p1.y()*p.y() + p1.z()*p.z());
	equ.push_back(d);

	refL->setPlaneEQU(equ);
}

void EulerPoly::link(osg::Vec3d P1, osg::Vec3d P2)
{
	Solid *refS = dynamic_cast<Solid*> (this);
	if (!refS->getNumPoints())
	{
		mvfs(P1);
	}
	Points *refP1 = refS->findPoint(P1);
	Points *refP2 = refS->findPoint(P2);

	if (!refP1 && !refP2)
	{
		osg::notify(osg::FATAL) << "Cannot create line from 2 undefined points!" << std::endl;
		return;
	}

	if (refP1 && refP2)
	{
		if (refP1 == refP2)
		{
			osg::notify(osg::FATAL) << "Cannot create line from 2 same points!" << std::endl;
			return;
		}
		Points *oldP = (refP1->getIndex() > refP2->getIndex()) ? refP2 : refP1;
		Points *newP = (refP1->getIndex() > refP2->getIndex()) ? refP1 : refP2;
		return mef(oldP, newP);
	}

	{
		Points *oldP = (refP1) ? refP1 : refP2;
		osg::Vec3d newP = (refP1) ? P2 : P1;
		return mev(oldP, newP);
	}
}