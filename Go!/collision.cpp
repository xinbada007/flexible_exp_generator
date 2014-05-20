#include "stdafx.h"
#include "collision.h"
#include "collVisitor.h"
#include "car.h"
#include "road.h"
#include "plane.h"
#include "math.h"

#include "halfedge.h"
#include "points.h"

#include <osg/Group>
#include <osg/Notify>
#include <osg/BoundingBox>
#include <osg/BoundingSphere>
#include <osg/PolygonMode>
#include <osg/LineWidth>

#include <string>
#include <algorithm>

Collision::Collision()
{
}

Collision::~Collision()
{
}


bool Collision::detectFirst(const osg::ref_ptr<osg::Vec3dArray> planeMove, const Plane *planeObS)
{
	if (!planeMove || !planeObS)
	{
		return false;
	}

	osg::Vec3dArray::const_iterator i = planeMove->begin();
	const planeEQU &equOBS = planeObS->getLoop()->getPlaneEQU();
	
	double sign_cur(0), sign_prv(0);
	while (i != planeMove->end())
	{
		sign_cur = ifPoints_ON_Plane(*i++, equOBS);
		if (sign_cur*sign_prv < 0)
		{
			return true;
		}
		sign_prv = sign_cur;
	}

	return false;
}

bool Collision::detectSecond(const osg::ref_ptr<osg::Vec3dArray> planeMove, const Plane *planeObs)
{
	osg::BoundingBox bbMove;
	osg::Vec3dArray::const_iterator i = planeMove->begin();
	while (i != planeMove->end())
	{
		bbMove.expandBy(*i++);
	}

	return bbMove.intersects(planeObs->getBoundingBox());
}

bool Collision::detectFinal(const osg::ref_ptr<osg::Vec3dArray> planeMove, Plane *planeObs)
{
	const edgelist &list = planeObs->getLoop()->getFlagEdge();
	
	if (list.empty())
	{
		//try another method here
	}

	//simpler method if has list
	{
		edgelist::const_iterator i = list.cbegin();
		edgelist::const_iterator END = list.cend();
		while (i != END)
		{
			if ((*i)->getEdgeFlag()->_collsionEdge)
			{
				osg::Vec3d a = (*i)->getHE1()->getPoint()->getPoint();
				osg::Vec3d b = (*i)->getHE2()->getPoint()->getPoint();
				osg::ref_ptr<osg::Vec3dArray> edge = new osg::Vec3dArray;
				edge->push_back(a); edge->push_back(b);
				osg::ref_ptr<osg::Vec3dArray> rectObs = linetoRectangle(edge);

				osg::Vec3dArray::const_iterator j = planeMove->begin();
				osg::Vec3dArray::const_iterator END = planeMove->end();
				osg::ref_ptr<osg::Vec3dArray> lineMove = new osg::Vec3dArray;
				lineMove->push_back(planeMove->front()); lineMove->push_back(planeMove->back());
				do
				{
					osg::ref_ptr<osg::Vec3dArray> rectMove = linetoRectangle(lineMove);
					if (ifRectangleOverlap(rectMove, rectObs))
					{
						if (ifLineIntersects(lineMove, edge))
						{
							return true;
						}
					}

					lineMove->clear();
					lineMove->push_back(*j); lineMove->push_back(*(++j));
				} while (j != END);
			}
			i++;
		}
	}

	return false;
}

Plane * Collision::ifCollide(const osg::ref_ptr<osg::Vec3dArray> planeMove, const quadList refObs)
{
	if (!planeMove)
	{
		return NULL;
	}

	quadList::const_iterator i = refObs.cbegin();
	while (i != refObs.cend())
	{
		//detectFirst is fast and should accurate enough
		if (detectFirst(planeMove, *i))
		{
			//detectSecond is provided by OSG and it's inaccurate
//			if (detectSecond(planeMove,*i))
			{
				//dectectFinal is the most accurate one and may the slowest one as well
				if (detectFinal(planeMove,*i))
					return *i;
			}
		}
		++i;
	}

	return NULL;
}

quadList Collision::listCollsion(const Car *refC, const quadList obs)
{
	quadList list;

	if (refC && !obs.empty())
	{
		const CarState *refCS = refC->getCarState();
		const osg::ref_ptr<osg::Vec3dArray> carArray = refCS->_carArray;

		Plane *pl = ifCollide(carArray, obs);
		if (pl) list.push_back(pl);
	}

	return list;
}

bool Collision::detectLocation(const Point pMove, const Plane *planeRoad)
{
	if (!planeRoad)
	{
		return false;
	}

	osg::ref_ptr<osg::Vec3dArray> pRoad = planeRoad->getLoop()->getPoints();
	planeEQU pEQU = planeRoad->getLoop()->getPlaneEQU();
	
	return (ifPoint_IN_Polygon(pMove, pRoad, pEQU));
}

Plane * Collision::locateCar(Plane::reverse_across_iterator begin, Plane::reverse_across_iterator end, const Point pMove)
{
	Plane::reverse_across_iterator i = begin;
	while (i != end)
	{
		if (detectLocation(pMove,*i))
		{
			return *i;
		}
		i++;
	}

	return NULL;
}

quadList Collision::listRoadQuad(const Car *refC, const solidList road)
{
	quadList list;
	quadList curList;

	if (refC && !road.empty())
	{
		const CarState *refCS = refC->getCarState();
		const osg::ref_ptr<osg::Vec3dArray> carArray = refCS->_carArray;
		quadList lasQuad = refCS->_lastQuad;

		//find which polygon the car is in
		osg::Vec3dArray::const_iterator iCar = carArray->begin();
		unsigned int step(0);
		if (lasQuad.empty())
		{
			while (iCar != carArray->end())
			{
				lasQuad.push_back(road.front()->getLastPlane());
				iCar++;
			}
			solidList::const_iterator iRoad = road.cbegin();
			while (iRoad != road.cend())
			{
				step += (*iRoad)->getNumPlanes();
				iRoad++;
			}
		}

		quadList::const_iterator iQuad = lasQuad.cbegin();
		iCar = carArray->begin();
		while (iCar != carArray->end())
		{
			step = (!step) ? .1f * (*iQuad)->getHomeS()->getNumPlanes() : step;
			Plane::reverse_across_iterator location = *iQuad;
			Plane::reverse_across_iterator begin = location - step;
			Plane::reverse_across_iterator end = location + step;

			Plane *loc = locateCar(begin, end, *iCar);
			Plane *pl = (loc) ? loc : *location;
			list.push_back(pl);
			curList.push_back(loc);
			++iCar;
			++iQuad;
		}		
	}

	//reduce the size of possible wall that the car would collide
	_wall_reduced.clear();
	if (!list.empty())
	{
		quadList qList = list;
		quadList::iterator listEnd = std::unique(qList.begin(), qList.end());
		qList.erase(listEnd, qList.end());

		quadList::const_iterator listBegin = qList.cbegin();
		listEnd = qList.end();
		while (listBegin != listEnd)
		{
			unsigned solidIndex = (*listBegin)->getHomeS()->getIndex();
			unsigned planeIndex = (*listBegin)->getIndex();

			solidList obSolid;
			std::copy_if(_wall.cbegin(), _wall.cend(), std::back_inserter(obSolid), searchIndex(solidIndex));
			if (!obSolid.empty())
			{
				solidList::const_iterator obsBegin = obSolid.cbegin();
				solidList::const_iterator obsEnd = obSolid.cend();
				while (obsBegin != obsEnd)
				{
					Plane::reverse_iterator reduced_begin = (*obsBegin)->getLastPlane();
					Plane::reverse_iterator reduced_end = (*obsBegin)->getPlane();
					Plane::reverse_iterator itPlane = std::find_if(reduced_begin, reduced_end, searchIndex(planeIndex));
					if (itPlane != reduced_end) _wall_reduced.push_back(*itPlane);
					obsBegin++;
				}
			}
			listBegin++;
		}
	}

	quadList::const_iterator i = curList.cbegin();
	while (i != curList.cend())
	{
		list.push_back(*i++);
	}
	return list;
}

void Collision::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
	const CollVisitor *refCV = dynamic_cast<CollVisitor*>(this->getUserData());
	const Car *refC = dynamic_cast<Car*>(node);
	if (refCV && refC)
	{
		_wall = refCV->getWall();
		const solidList road = refCV->getRoad();

		quadList listRoad;
		listRoad = listRoadQuad(refC, road);
		
		{
			unsigned size_list = refC->getCarState()->_carArray->size();
			refC->getCarState()->_lastQuad.clear();
			refC->getCarState()->_currentQuad.clear();
			std::copy(listRoad.cbegin(), listRoad.cbegin() + size_list, std::back_inserter(refC->getCarState()->_lastQuad));
			std::copy(listRoad.cbegin() + size_list, listRoad.cend(), std::back_inserter(refC->getCarState()->_currentQuad));
			//setup the quad for origin point of the car
			refC->getCarState()->_OQuad = listRoad.back();
		}

		//collision detect
		quadList listWall;
		listWall = listCollsion(refC, _wall_reduced);
		if (!listWall.empty())
		{
			refC->getCarState()->_collisionQuad = listWall;
			refC->getCarState()->_collide = true;
		}
		else
		{
			refC->getCarState()->_collide = false;
		}
	}
	
	traverse(node, nv);
}