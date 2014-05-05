#include "stdafx.h"
#include "switchVisitor.h"
#include <osg/Switch>
#include "logicRoad.h"

SwitchVisitor::SwitchVisitor():
osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
_swValue(ROAD)
{
}

SwitchVisitor::SwitchVisitor(ROADTAG sw):
osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
_swValue(sw)
{
}

SwitchVisitor::~SwitchVisitor()
{
}

void SwitchVisitor::apply(osg::Switch &sw)
{
	int numChildren(sw.getNumChildren());
	if (numChildren)
	{
		for (int i = 0; i < numChildren;i++)
		{
			LogicRoad * lr = dynamic_cast<LogicRoad*> (sw.getChild(i));
			if (lr)
			{
				if (lr->getTag() == _swValue)
				{
					sw.setChildValue(lr, !sw.getChildValue(lr));
				}
			}
		}
	}

	traverse(sw);
}