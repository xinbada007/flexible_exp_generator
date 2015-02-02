#include "stdafx.h"

#include "DeConstructerVisitor.h"
#include "logicRoad.h"

#include <osg/Group>

DeConstructerVisitor::DeConstructerVisitor():
osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
}


DeConstructerVisitor::~DeConstructerVisitor()
{
}

void DeConstructerVisitor::apply(osg::Node& node)
{
	node.setEventCallback(NULL);
	node.setUpdateCallback(NULL);

	traverse(node);

	osg::Group *group = dynamic_cast<osg::Group*>(&node);
	if (group)
	{
		while (group->getNumChildren())
		{
			group->removeChild(0,group->getNumChildren());
		}
	}
	LogicRoad *lr = dynamic_cast<LogicRoad*>(&node);
	if (lr)
	{
		lr->setPrev(NULL);
	}
	if (lr)
	{
		lr->setNext(NULL);
	}

	Solid *solid = dynamic_cast<Solid*>(&node);
	if (solid)
	{
		solid->setPrev(NULL);
	}
	if (solid)
	{
		solid->setNext(NULL);
	}
}