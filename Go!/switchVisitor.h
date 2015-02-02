#pragma once
#include <osg/NodeVisitor>
#include "logicRoad.h"

class SwitchVisitor :
	public osg::NodeVisitor
{
public:
	SwitchVisitor();
	SwitchVisitor(ROADTAG sw);
	virtual ~SwitchVisitor();

	void apply(osg::Switch &sw);

private:
	ROADTAG _swValue;
};

