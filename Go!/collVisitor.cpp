#include "stdafx.h"
#include "collVisitor.h"
#include "logicRoad.h"
#include "car.h"

CollVisitor::CollVisitor():
osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN),
_car(NULL)
{
}


CollVisitor::~CollVisitor()
{
}

void CollVisitor::reset()
{

}

void CollVisitor::apply(osg::Group& node)
{
	osg::ref_ptr<LogicRoad> refLR = dynamic_cast<LogicRoad*>(&node);
	osg::ref_ptr<Car> refCar = dynamic_cast<Car*>(&node);

	if (refLR)
	{
		osg::notify(osg::NOTICE) << "caught\t" << node.className() << std::endl;
		const ROADTAG refTag = refLR->getTag();

		switch (refTag)
		{
		case LWALL:
			_wall.push_back(dynamic_cast<Solid*>(refLR.get()));
			break;
		case RWALL:
			_wall.push_back(dynamic_cast<Solid*>(refLR.get()));
			break;
		case ROAD:
			_rd.push_back(dynamic_cast<Solid*>(refLR.get()));
			break;
		default:
			break;
		}
	}

	if (refCar)
	{
		osg::notify(osg::NOTICE) << "caught\t" << node.className() << std::endl;
		_car = refCar.get();
	}

	traverse(node);
}