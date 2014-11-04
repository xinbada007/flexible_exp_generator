#include "stdafx.h"
#include "collision.h"
#include "collVisitor.h"
#include "car.h"
#include "road.h"
#include "plane.h"
#include "halfedge.h"
#include "points.h"
#include "math.h"
#include "edge.h"

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
	std::cout << "Deconsturct Collision" << std::endl;
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
		return false;
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
	if (!planeMove || refObs.empty())
	{
		return NULL;
	}

	quadList::const_iterator i = refObs.cbegin();
	const unsigned NUMOFPLANES = (*i)->getHomeS()->getNumPlanes() * 0.05;
	while (i != refObs.cend())
	{
		//detectFirst is very fast but also very inaccurate
		if (detectFirst(planeMove, *i))
		{
			//detectSecond is provided by OSG and it's inaccurate
//			if (detectSecond(planeMove,*i))
			{
				//dectectFinal is the most accurate one and may the slowest one as well
				Plane::reverse_across_iterator collision_i = *i;
				unsigned collision_test_num(0);
				while (collision_test_num < NUMOFPLANES && (*collision_i))
				{
					if (detectFinal(planeMove, *collision_i)) return *i;
					collision_i++;
					collision_test_num++;
				}
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

		//attention! last element of carArray is the original point of car
		//which should be removed when doing collision detection
		const unsigned wheelsofCar(4);
		osg::ref_ptr<osg::Vec3dArray> carRealArray = new osg::Vec3dArray(carArray->begin(), carArray->begin()+wheelsofCar);

		Plane *pl = ifCollide(carRealArray, obs);
		if (pl) list.push_back(pl);
	}

	return list;
}

bool Collision::ifObsCollide(const Car *refC, const Solid *obs)
{
	//attention! last element of carArray is the original point of car
	//which should be removed when doing collision detection

	const unsigned wheelsofCar(4);
	const osg::ref_ptr<osg::Vec3dArray> carArray = refC->getCarState()->_carArray;
	osg::ref_ptr<osg::Vec3dArray> planeMove = new osg::Vec3dArray(carArray->begin(), carArray->begin() + wheelsofCar);
	osg::Vec3dArray::const_iterator j = planeMove->begin();

	const double refuseR = obs->absoluteTerritory._refuseR;
	if (refuseR)
	{
		osg::Vec3d center = obs->absoluteTerritory.center;
		while (j != planeMove->end())
		{
			const double distance = (center - (*j)).length();
			if (distance <= refuseR)
			{
				return true;
			}
			j++;
		}
		center.z() = 0.0f;
		if (ifPoint_IN_Polygon(center, planeMove, refC->getPlane()->getLoop()->getPlaneEQU()))
			return true;
	}

	while (j != planeMove->end())
	{
		if (obs->ifPointinSolid(*j))
			return true;
		j++;
	}

	Points *p = obs->getPoint();
	planeEQU &equ = refC->getPlane()->getLoop()->getPlaneEQU();
	while (p)
	{
		if (!(p->getPoint().z()))
		{
			if (ifPoint_IN_Polygon(p->getPoint(), planeMove, equ))
				return true;
		}
		p = p->getNext();
	}

	return false;
}

bool Collision::detectObsDistance(const Car *refC, const Solid *obs)
{
	const double &R1 = refC->absoluteTerritory._detectR;
	const double &R2 = obs->absoluteTerritory._detectR;
	if (R1 && R2)
	{
		const double distance = (obs->absoluteTerritory.center - refC->absoluteTerritory.center).length();
		if (distance < R1 + R2)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	const osg::Vec3d &center = refC->getCarState()->_carArray->back();
	const double therold = std::max(refC->getVehicle()->_width, refC->getVehicle()->_length)*2;
	Points *p = obs->getPoint();
	while (p)
	{
		if (!(p->getPoint().z()))
		{
			if ((center - p->getPoint()).length() < therold)
				return true;
		}
		p = p->getNext();
	}

	return false;
}

solidList Collision::listObsCollsion(const Car *refC, const solidList obs)
{
	solidList list;

	if (refC && !obs.empty())
	{
		solidList::const_iterator i = obs.cbegin();
		while (i != obs.cend())
		{
			if (detectObsDistance(refC, *i))
			{
				if (ifObsCollide(refC, *i))
				{
					list.push_back(*i);
				}
			}
			i++;
		}
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
			{
				//will cause performance issue so be careful
				step += (*iRoad)->getNumPlanes()*.05f;
			}
		}

		quadList::const_iterator iQuad = lasQuad.cbegin();
		iCar = carArray->begin();
		while (iCar != carArray->end())
		{
			const unsigned iLoc = iCar - carArray->begin();
			const unsigned iIndex = iCar - carArray->begin() + 1;
			Plane *loc, *pl;
			iQuad = lasQuad.cbegin();
			while (iQuad != lasQuad.cend())
			{
				step = (!step) ? .05f * (*iQuad)->getHomeS()->getNumPlanes() : step;
				Plane::reverse_across_iterator location = *iQuad;
				Plane::reverse_across_iterator begin = location - step;
				//Plane::reverse_across_iterator end = location + step;
				Plane::reverse_across_iterator end = location; end.add(step);

				loc = locateCar(begin, end, *iCar);
				location = lasQuad[iLoc];
				pl = (loc) ? loc : *location;
				if (loc)
				{
					if (list.size() < iIndex)
					{
						list.push_back(pl);
					}
					if (curList.size() < iIndex)
					{
						curList.push_back(loc);
					}
					break;
				}
				++iQuad;
			}
			if (list.size() < iIndex)
			{
				list.push_back(pl);
			}
			if (curList.size() < iIndex)
			{
				curList.push_back(loc);
			}
			++iCar;

			
// 			step = (!step) ? .05f * (*iQuad)->getHomeS()->getNumPlanes() : step;
// 			Plane::reverse_across_iterator location = *iQuad;
// 			Plane::reverse_across_iterator begin = location - step;
// 			Plane::reverse_across_iterator end = location + step;
// 
// 			Plane *loc = locateCar(begin, end, *iCar);
// 			Plane *pl = (loc) ? loc : *location;
// 			list.push_back(pl);
// 			curList.push_back(loc);
// 			++iCar;
// 			++iQuad;
		}

		if (list.size() != carArray->size() || curList.size() != carArray->size())
		{
			osg::notify(osg::WARN) << "Failed to detect collision" << std::endl;
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
	const CollVisitor *refCV = CollVisitor::instance();
	const Car *refC = dynamic_cast<Car*>(node);
	if (refCV && refC)
	{
		_wall = refCV->getWall();
		const solidList &road = refCV->getRoad();

		const quadList &listRoad = listRoadQuad(refC, road);
		
		{
			unsigned size_list = refC->getCarState()->_carArray->size();
			if (!refC->getCarState()->_lastQuad.size())
			{
				std::copy(listRoad.cbegin(), listRoad.cbegin() + size_list, std::back_inserter(refC->getCarState()->_lastQuad));
			}
			else
			{
				std::copy(listRoad.cbegin(), listRoad.cbegin() + size_list, refC->getCarState()->_lastQuad.begin());
			}
			if (!refC->getCarState()->_currentQuad.size())
			{
				std::copy(listRoad.cbegin() + size_list, listRoad.cend(), std::back_inserter(refC->getCarState()->_currentQuad));
			}
			else
			{
				std::copy(listRoad.cbegin() + size_list, listRoad.cend(), refC->getCarState()->_currentQuad.begin());
			}
			
			
			//setup the quad for origin point of the car
			refC->getCarState()->_OQuad = listRoad.back();
		}

		//collision against Wall detect
		refC->getCarState()->_collisionQuad = listCollsion(refC, _wall_reduced);;

		//collision against Obstacle detection
		refC->getCarState()->setObsList(listObsCollsion(refC, refCV->getObstacle()));
	}
	
	traverse(node, nv);
}