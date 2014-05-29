#include "stdafx.h"
#include "roadSwitcher.h"
#include "collVisitor.h"
#include "logicRoad.h"
#include "edge.h"


RoadSwitcher::RoadSwitcher()
{
}


RoadSwitcher::~RoadSwitcher()
{
}


void RoadSwitcher::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
	const CollVisitor *refCV = dynamic_cast<CollVisitor*>(this->getUserData());

	if (refCV)
	{
		_carState = refCV->getCar()->getCarState();
	}
	if (_carState)
	{
		Plane::reverse_across_iterator i = *_carState->_OQuad;
		if (*i)
		{
			const unsigned numPlanes = (*i)->getHomeS()->getNumPlanes();
			Plane::reverse_across_iterator ON = i + numPlanes * 0.5f;
			Plane::reverse_across_iterator OFF = i - numPlanes * 1.5f;

			int solidON(-1), solidOFF(-1);
			if (*ON)	solidON = (*ON)->getHomeS()->getIndex();
			if (*OFF)	solidOFF = (*OFF)->getHomeS()->getIndex();
			int solidCurrent = (*i)->getHomeS()->getIndex();
			node->accept(*new RoadSwitchVisitor(solidON,solidOFF));
			node->accept(*new RoadSwitchVisitor(solidCurrent));
		}
	}

	traverse(node, nv);
}

void RoadSwitchVisitor::apply(osg::Switch &sw)
{
	int numChildren(sw.getNumChildren());
	if (numChildren)
	{
		for (int i = 0; i < numChildren; i++)
		{
			LogicRoad * lr = dynamic_cast<LogicRoad*> (sw.getChild(i));
			if (lr)
			{
				if (lr->getTag() == ROAD || lr->getTag() == LWALL || lr->getTag() == RWALL)
				{
					if (lr->getIndex() == _solidIndexON)
					{
						sw.setChildValue(lr, true);
					}
					if (lr->getIndex() == _solidIndexOFF)
					{
						sw.setChildValue(lr, false);
					}
				}
			}
		}
	}

	traverse(sw);
}