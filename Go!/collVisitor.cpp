#include "stdafx.h"
#include "collVisitor.h"
#include "logicRoad.h"
#include "car.h"
#include "edge.h"

CollVisitor::CollVisitor():
osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
_car(NULL), _mode(ROAD), _useMode(false)
{
}


CollVisitor::~CollVisitor()
{
	std::cout << "Deconstruct CollVisitor" << std::endl;
}

void CollVisitor::reset()
{

}

void CollVisitor::pushLR(osg::ref_ptr<LogicRoad> refLR)
{
	switch (refLR->getTag())
	{
	case MIDQUAD:
		break;
	case CTRL:
		break;
	case LWALL:
		_wall.push_back(dynamic_cast<Solid*>(refLR.get()));
		break;
	case RWALL:
		_wall.push_back(dynamic_cast<Solid*>(refLR.get()));
		break;
	case ROAD:
		_rd.push_back(dynamic_cast<Solid*>(refLR.get()));
		break;
	case OBS:
		_obs.push_back(dynamic_cast<Solid*>(refLR.get()));
		break;
	default:
		break;
	}
}

void CollVisitor::apply(osg::Group& node)
{
	osg::ref_ptr<LogicRoad> refLR = dynamic_cast<LogicRoad*>(&node);
	osg::ref_ptr<Car> refCar = dynamic_cast<Car*>(&node);

	if (refLR)
	{
		if (!_useMode)
		{
			pushLR(refLR);
		}
		else if (_useMode)
		{
			if (refLR->getTag() == _mode)
			{
				pushLR(refLR);
			}
		}
	}

	if (refCar)
	{
		osg::notify(osg::NOTICE) << "caught\t" << node.className() << std::endl;
		_car = refCar.get();
	}

	traverse(node);
}