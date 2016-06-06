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
//	osg::notify(osg::NOTICE) << "Roadswitcher..Begins..." << std::endl;

	if (!_carState)
	{
		osg::notify(osg::WARN) << "Cannot find Car failed to switch Road" << std::endl;
		return;
	}
	if (_carState)
	{
		Plane::reverse_across_iterator i = *_carState->_OQuad;
		if (*i)
		{
			const int numPlanes = (*i)->getHomeS()->getNumPlanes();
			Plane::reverse_across_iterator ON(*i); 
			ON.add(numPlanes*0.5f);
			Plane::reverse_across_iterator OFF(*i);
			OFF.add(-numPlanes*1.5f);

			int solidON(-1), solidOFF(-1);
			if (*ON)	solidON = (*ON)->getHomeS()->getIndex();
			if (*OFF)	solidOFF = (*OFF)->getHomeS()->getIndex();
			int solidCurrent = (*i)->getHomeS()->getIndex();
			node->accept(*new RoadSwitchVisitor(solidON,solidOFF));
			node->accept(*new RoadSwitchVisitor(solidCurrent));
		}
	}
//	osg::notify(osg::NOTICE) << "Roadswitch..Traverse..." << std::endl;
	traverse(node, nv);
//	osg::notify(osg::NOTICE) << "Roadswitch..END..." << std::endl;
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
					if (lr->getIndex() == _solidIndexON || _AllON)
					{
						sw.setChildValue(lr, true);
					}
					if (lr->getIndex() == _solidIndexOFF || _AllOFF)
					{
						sw.setChildValue(lr, false);
					}
				}
			}
		}
	}

	traverse(sw);
}

void OBSSwitchVisitor::apply(osg::MatrixTransform &mt)
{
	int numChildren(mt.getNumChildren());
	for (int i = 0; i < numChildren; i++)
	{
		LogicRoad *lr = dynamic_cast<LogicRoad*> (mt.getChild(i));
		if (lr)
		{
			if (lr->getTag() == OBS)
			{
				osg::Switch *sw = dynamic_cast<osg::Switch*>(mt.getParent(0));

				if (sw && _AllON)
				{
					sw->setChildValue(&mt, true);
				}
				else if (sw && _AllOFF)
				{
					sw->setChildValue(&mt, false);
				}
			}
		}
	}

	traverse(mt);
}