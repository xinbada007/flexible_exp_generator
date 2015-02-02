#pragma once
#include <osg/NodeVisitor>

class DeConstructerVisitor :
	public osg::NodeVisitor
{
public:
	DeConstructerVisitor();

	virtual void apply(osg::Node& node);

	virtual ~DeConstructerVisitor();
};

