#include "stdafx.h"
#include "obstacle.h"
#include "halfedge.h"
#include "points.h"

Obstacle::Obstacle()
{
}


Obstacle::~Obstacle()
{
}


void Obstacle::createBox(osg::Vec3d center, osg::Vec3d radius)
{
	osg::Vec3d left_bottom = center * osg::Matrix::translate(-radius);
	left_bottom.z() = 0.0f;

	osg::Vec3d right_bottom = left_bottom;
	right_bottom.x() += 2 * radius.x();

	osg::Vec3d right_top = right_bottom;
	right_top.y() += 2 * radius.y();

	osg::Vec3d left_top = right_top;
	left_top.x() -= 2 * radius.x();

	link(left_bottom, right_bottom);
	link(right_bottom, right_top);
	link(right_top, left_top);
	link(left_top, left_bottom);

	sweep(radius.z());
}

void Obstacle::sweep(const int height)
{
	Solid *refS = dynamic_cast<Solid*> (this);
	if (!refS)
	{
		osg::notify(osg::FATAL) << "Cannot Sweep! Solid is not exist!" << std::endl;
		return;
	}

	if (!refS->getNumPlanes())
	{
		osg::notify(osg::FATAL) << "Cannot Sweep! Solid is not created yet!" << std::endl;
		return;
	}

	Plane *p = refS->getPlane();
	Loop *l = p->getLoop();
	HalfEdge *startHE = l->getHE();
	const HalfEdge *const endHE = startHE;

	osg::Vec3d firstP = startHE->getPoint()->getPoint();
	firstP.z() += height;
	osg::Vec3d thisP = firstP;
	osg::Vec3d lastP = thisP;

	do 
	{
		thisP = startHE->getPoint()->getPoint();
		thisP.z() += height;
		link(startHE->getPoint()->getPoint(), thisP);

		if (lastP != thisP)
		{
			link(lastP, thisP);
		}
	
		lastP = thisP;
		startHE = startHE->getNext();
	} while (startHE != endHE);

	link(firstP, lastP);

	Plane *abs = this->getAbstract();
	if (abs)
	{
		abs->setAbstract(false);
	}
}