#include "stdafx.h"
#include "DeConstructerVisitor.h"

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
	while (node.getEventCallback())
	{
		node.removeEventCallback(node.getEventCallback());
	}
	while (node.getUpdateCallback())
	{
		node.removeUpdateCallback(node.getUpdateCallback());
	}

	traverse(node);

	osg::Group *group = dynamic_cast<osg::Group*>(&node);
	if (group)
	{
		while (group->getNumChildren())
		{
			group->removeChild(0,group->getNumChildren());
		}
	}
}