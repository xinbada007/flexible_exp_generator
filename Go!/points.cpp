#include "stdafx.h"
#include "points.h"

#include <osg/Notify>

Points::Points(osg::Vec3d newP) :
_p(newP), _homeHE(NULL), _next(NULL), _prev(NULL), _index(0)
{

}

Points::~Points()
{
	//osg::notify(osg::NOTICE) << "deleting Points..." << std::endl;
}

bool Points::isEqual(osg::Vec3d ref) const
{
	return (_p == ref) ? true : false;
}