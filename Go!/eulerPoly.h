#pragma once
#include "solid.h"

class EulerPoly :
	public Solid
{
public:
	EulerPoly();
	EulerPoly(const EulerPoly &copy, osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY);
	META_Node(EulerPoly, EulerPoly);
protected:
	void link(osg::Vec3d P1, osg::Vec3d P2);
	Edge *insPt(Edge *mEdge,const osg::Vec3d &newP);
	void calcPlaneEQU(Loop *refL);
	~EulerPoly();
private:
	bool _ccwupdated;

	void mev(Points *oldP, osg::Vec3d newP);
	void mef(Points *oldP, Points *newP);
	void mvfs(osg::Vec3d ref);
	Edge * semv(Edge *mEdge, const osg::Vec3d &newP);
};