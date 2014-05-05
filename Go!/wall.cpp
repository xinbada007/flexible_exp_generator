#include "stdafx.h"
#include "wall.h"
#include "road.h"

#include <osg/Notify>

Wall::Wall()
{
}


Wall::~Wall()
{
}

/*
void Wall::build(Road *refRd)
{
	if (!refRd)
	{
		osg::notify(osg::NOTICE) << "couldn't find a Road node" << std::endl;
		return;
	}

	LogicRoad *refLR = dynamic_cast<Road*>(refRd);
	if (!refLR)
	{
		osg::notify(osg::NOTICE) << "couldn't find a LogicRoad node" << std::endl;
		return;
	}

	while (refLR && refLR->getTag() == ROAD)
	{
		osg::notify(osg::NOTICE) << "Building wall..." << std::endl;
		generate(dynamic_cast<Solid*>(refLR));
		refLR = refLR->getNext();
	}
}
*/

void Wall::build(Solid *refRd)
{
	if (!refRd)
	{
		osg::notify(osg::NOTICE) << "couldn't find a Solid node" << std::endl;
		return;
	}
	osg::notify(osg::NOTICE) << "Building wall..." << std::endl;


}