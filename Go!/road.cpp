#include "stdafx.h"
#include "road.h"
#include "edge.h"

Road::Road() :
_LEFTWALL(1), _RIGHTWALL(-1), _roadSet(NULL)
{
}

Road::Road(const Road &copy, osg::CopyOp copyop /* = osg::CopyOp::SHALLOW_COPY */):
osg::Switch(copy, copyop), _LEFTWALL(copy._LEFTWALL), _RIGHTWALL(copy._RIGHTWALL), _roadSet(copy._roadSet),
_roadList(copy._roadList), _lWallList(copy._lWallList), _rWallList(copy._rWallList), _ctrlList(copy._ctrlList), _midList(copy._midList)
{

}

Road::~Road()
{
}

osg::ref_ptr<osg::Vec2Array> Road::universalTexture(const int i,const double size)
{
	const osg::ref_ptr<osg::Vec3dArray> midline = _roadSet->_nurbs.at(i)->_path;
	osg::Vec3dArray::const_iterator j = midline->begin();
	osg::ref_ptr<osg::Vec2Array> texcoord = new osg::Vec2Array;
	const double &std_distance = size;
	double tot_distance(0.0f);
	while (j != midline->end())
	{
		const osg::Vec3d mid = *(j + 1) - *(j);
		const double cur_distance = mid.length();
		const double ratio = tot_distance / std_distance;

		osg::Vec2 a(ratio, 0.0f);
		osg::Vec2 b(ratio + cur_distance / std_distance, 0.0f);
		osg::Vec2 c(ratio + cur_distance / std_distance, 1.0f);
		osg::Vec2 d(ratio, 1.0f);

		texcoord->push_back(d); texcoord->push_back(c);
		texcoord->push_back(b); texcoord->push_back(a);

		tot_distance += cur_distance;
		j++;
	}

	return texcoord.release();
}

osg::ref_ptr<LogicRoad> Road::universalLogicRoad(const int i, ROADTAG refTag)
{
	osg::ref_ptr<LogicRoad> newLogic = NULL;
	osg::ref_ptr<Nurbs> refNurbs = _roadSet->_nurbs.at(i);

	osg::ref_ptr<osg::Vec3dArray> path = NULL;
	osg::ref_ptr<osg::Vec3dArray> pathOffset = NULL;
	double wh(0.0);

	edgeFlag *pathFlag = NULL;
	FlagEdgeArrayList eFlagArray;

	if (refTag == LWALL || refTag == RWALL)
	{
		path = (refTag == LWALL) ? refNurbs->_path_left : refNurbs->_path_right;

		//Flag the Edge for collision detect
		{
			pathFlag = new edgeFlag;
			pathFlag->_collsionEdge = true;

			eFlagArray.push_back(path);
		}

		pathOffset = new osg::Vec3dArray(path->begin(), path->end());
		wh = _roadSet->_wallHeight;
		osg::Matrix m = osg::Matrix::translate(osg::Vec3d(0.0f, 0.0f, wh));
		arrayByMatrix(pathOffset, m);
	}
	else if (refTag == ROAD)
	{
		path = refNurbs->_path_left;
		pathOffset = refNurbs->_path_right;
		wh = _roadSet->_width;
		
		//Flag the Edge for navigating vehicle
		{
			pathFlag = new edgeFlag;
			pathFlag->_navigateEdge = true;

			eFlagArray.push_back(path);
			eFlagArray.push_back(pathOffset);
		}
	}
	else if (refTag == CTRL)
	{
		path = refNurbs->_ctrl_left;
		pathOffset = refNurbs->_ctrl_right;
	}
	else if (refTag == MIDQUAD)
	{
		wh = _roadSet->_width * 0.0025;
		path = arrayLinearTransform(refNurbs->_path_right, refNurbs->_path_left, 0.5f + wh);
		pathOffset = arrayLinearTransform(refNurbs->_path_right, refNurbs->_path_left, 0.5f - wh);
		wh = _roadSet->_width;
	}

	if (path && pathOffset)
	{
		newLogic = new LogicRoad;

		//setup the flag
		{
			newLogic->setEFlag(pathFlag);
			newLogic->setEdgeFlagArray(eFlagArray);
		}

		newLogic->line1D(path);
		newLogic->line1D(path, pathOffset);
		newLogic->line1D(pathOffset);
		newLogic->setTag(refTag);
		newLogic->setRawInformation(_roadSet, i);
		
		if (wh)
		{
			osg::ref_ptr<osg::Vec2Array> tex = universalTexture(i, wh);
			const int numTixel = 4;
			if (tex->size() / numTixel > newLogic->getNumGeometry())
			{
				newLogic->setTexCoord(tex.release());
			}
		}
	}

	return (newLogic)?newLogic.release():NULL;
}

void Road::genRoad(osg::ref_ptr<ReadConfig> refRC)
{
	_roadSet = refRC->getRoadSet();

	if (!_roadSet)
	{
		osg::notify(osg::FATAL) << "cannot build Road!" << std::endl;
		return;
	}

	nurbsList::const_iterator i = _roadSet->_nurbs.cbegin();
	while (i != _roadSet->_nurbs.cend())
	{
		osg::ref_ptr<Nurbs> nurbs = *i;
		osg::ref_ptr<osg::Switch> sw = new osg::Switch;
		sw->setAllChildrenOn();
		this->addChild(sw);

		const int j = i - _roadSet->_nurbs.begin();

		//Add Road
		osg::ref_ptr<LogicRoad> newR = universalLogicRoad(j, ROAD);
		sw->addChild(addLogRoadtoList(newR.get()));	
		_roadList.push_back(newR.get());

		//Add mid-Line of Road
		osg::ref_ptr<LogicRoad> newMD = universalLogicRoad(j, MIDQUAD);
		sw->addChild(addLogRoadtoList(newMD.get()));
		_midList.push_back(newMD.get());

		//Add ctrlRoad
		osg::ref_ptr<LogicRoad> newLR = universalLogicRoad(j, CTRL);
		sw->addChild(addLogRoadtoList(newLR.get()));
		_ctrlList.push_back(newLR.get());

		//Add Walls
		osg::ref_ptr<LogicRoad> lWall = universalLogicRoad(j,LWALL);
		sw->addChild(addLogRoadtoList(lWall.get()));
		_lWallList.push_back(lWall.get());

		osg::ref_ptr<LogicRoad> rWall = universalLogicRoad(j,RWALL);
		sw->addChild(addLogRoadtoList(rWall.get()));
		_rWallList.push_back(rWall.get());

		i++;
	}
}

osg::Group * Road::addLogRoadtoList(LogicRoad *refLR)
{
	if (refLR)
	{
		osg::ref_ptr<osg::Switch> sw = new osg::Switch;

		switch (refLR->getTag())
		{
		case ROAD:
			if (!_roadList.empty())
			{
				refLR->setPrev(_roadList.back());
				_roadList.back()->setNext(refLR);
			}
			sw->setAllChildrenOn();
			break;
		case LWALL:
			if (!_lWallList.empty())
			{
				refLR->setPrev(_lWallList.back());
				_lWallList.back()->setNext(refLR);
			}
			sw->setAllChildrenOn();
			break;
		case RWALL:
			if (!_rWallList.empty())
			{
				refLR->setPrev(_rWallList.back());
				_rWallList.back()->setNext(refLR);
			}
			sw->setAllChildrenOn();
			break;
		case CTRL:
			if (!_ctrlList.empty())
			{
				refLR->setPrev(_ctrlList.back());
				_ctrlList.back()->setNext(refLR);
			}
			sw->setAllChildrenOff();
			break;
		case MIDQUAD:
			if (!_midList.empty())
			{
				refLR->setPrev(_midList.back());
				_midList.back()->setNext(refLR);
			}
			sw->setAllChildrenOff();
			break;
		default:
			sw->setAllChildrenOff();
			break;
		}		
		sw->addChild(refLR->asGroup());
		return sw.release();
	}

	return NULL;
}