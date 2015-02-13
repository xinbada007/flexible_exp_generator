#include "stdafx.h"
#include "collVisitor.h"
#include "logicRoad.h"
#include "car.h"
#include "edge.h"
#include "obstacle.h"

CollVisitor::CollVisitor():
osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), _mode(ROAD)
{

}


CollVisitor::~CollVisitor()
{
	std::cout << "Deconstruct CollVisitor" << std::endl;

	obstacleList::iterator obs_i = _obs.begin();
	while (obs_i != _obs.end())
	{
		*obs_i = NULL;
		++obs_i;
	}

	solidList::iterator sod_i = _rd.begin();
	while (sod_i != _rd.end())
	{
		*sod_i = NULL;
		++sod_i;
	}
	sod_i = _lwall.begin();
	while (sod_i != _lwall.end())
	{
		*sod_i = NULL;
		++sod_i;
	}
	sod_i = _rwall.begin();
	while (sod_i != _rwall.end())
	{
		*sod_i = NULL;
		++sod_i;
	}
	sod_i = _wall.begin();
	while (sod_i != _wall.end())
	{
		*sod_i = NULL;
		++sod_i;
	}

	_obs.clear();
	_rd.clear();
	_lwall.clear();
	_rwall.clear();
	_wall.clear();
}

void CollVisitor::reset()
{
	obstacleList::iterator obs_i = _obs.begin();
	while (obs_i != _obs.end())
	{
		*obs_i = NULL;
		++obs_i;
	}

	solidList::iterator sod_i = _rd.begin();
	while (sod_i != _rd.end())
	{
		*sod_i = NULL;
		++sod_i;
	}
	sod_i = _lwall.begin();
	while (sod_i != _lwall.end())
	{
		*sod_i = NULL;
		++sod_i;
	}
	sod_i = _rwall.begin();
	while (sod_i != _rwall.end())
	{
		*sod_i = NULL;
		++sod_i;
	}
	sod_i = _wall.begin();
	while (sod_i != _wall.end())
	{
		*sod_i = NULL;
		++sod_i;
	}

	_obs.clear();
	_rd.clear();
	_lwall.clear();
	_rwall.clear();
	_wall.clear();

	_mode = ROAD;
}

CollVisitor *CollVisitor::instance()
{
	static osg::ref_ptr<CollVisitor> cv = new CollVisitor;
	return cv.get();
}

const solidList CollVisitor::getWall() const
{
	_wall.clear();

	std::copy(_rwall.cbegin(), _rwall.cend(), std::back_inserter(_wall));
	std::copy(_lwall.cbegin(), _lwall.cend(), std::back_inserter(_wall));

	return _wall;
}

void CollVisitor::setMode(ROADTAG _ref)
{
	_mode = _ref;

	switch (_mode)
	{
	case MIDQUAD:
		break;
	case CTRL:
		break;
	case LWALL:
		_lwall.clear();
		break;
	case RWALL:
		_rwall.clear();
		break;
	case ROAD:
		_rd.clear();
		break;
	case OBS:
		_obs.clear();
		break;
	default:
		break;
	}
}

void CollVisitor::pushLR(osg::ref_ptr<LogicRoad> refLR)
{
	if (refLR->getTag() != _mode)
	{
		return;
	}

	switch (_mode)
	{
	case MIDQUAD:
		break;
	case CTRL:
		break;
	case LWALL:
		_lwall.push_back(dynamic_cast<Solid*>(refLR.get()));
		break;
	case RWALL:
		_rwall.push_back(dynamic_cast<Solid*>(refLR.get()));
		break;
	case ROAD:
		_rd.push_back(dynamic_cast<Solid*>(refLR.get()));
		break;
	case OBS:
		_obs.push_back(dynamic_cast<Obstacle*>(refLR.get()));
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
		pushLR(refLR);
	}

	traverse(node);
}