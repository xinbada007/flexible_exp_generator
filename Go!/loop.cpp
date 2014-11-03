#include "stdafx.h"
#include "loop.h"
#include "halfedge.h"
#include "edge.h"
#include "points.h"
#include "plane.h"
#include "solid.h"

Loop::Loop() :
_startHE(NULL), _prev(NULL), _next(NULL), _homeP(NULL),
_switch(true), _numHE(0)
{

}

Loop::Loop(const Loop &copy, osg::CopyOp copyop /* = osg::CopyOp::SHALLOW_COPY */) :
osg::Geometry(copy, copyop),
_startHE(copy.getHE()), _prev(copy.getPrev()), _next(copy.getNext()), _homeP(copy.getHomeP()),
_switch(copy.getSwitch()), _numHE(copy._numHE)
{

}

Loop::~Loop()
{
	//osg::notify(osg::NOTICE) << "deleting Loop..." << std::endl;
}

void Loop::draw()
{
	HalfEdge *refHE = _startHE;
	osg::ref_ptr<osg::Vec3dArray> varray = new osg::Vec3dArray;
	do
	{
		varray->push_back(refHE->getPoint()->getPoint());
		refHE = refHE->getNext();
	} while (refHE != _startHE);

	this->setVertexArray(varray.release());
}

bool Loop::isValid() const
{
	if ((!_startHE) || (!_homeP))
	{
		osg::notify(osg::FATAL) << "Loop is not valid" << std::endl;
		return false;
	}

	HalfEdge *refHE = _startHE;
	bool valid(false);
	do
	{
		if (refHE->getLoop() != this)
		{
			osg::notify(osg::FATAL) << "Loop and HE is inconsistent" << std::endl;
			valid = false;
			break;
		}
		valid = refHE->isValid();
		refHE = refHE->getNext();
	} while (valid && (refHE != _startHE));

	return valid;
}

void Loop::forwardInsertHE(HalfEdge *insertHE, HalfEdge *locHE)
{
	insertHE->getPrev()->setNext(locHE);
	locHE->setPrev(insertHE->getPrev());
	insertHE->setPrev(locHE);
	locHE->setNext(insertHE);

	insertHE->setLoop(this);
	locHE->setLoop(this);
}

void Loop::addHEtoLoop(Edge *refE)
{
	HalfEdge *temphe = _startHE;
	HalfEdge *newhe1 = refE->getHE1();
	HalfEdge *newhe2 = refE->getHE2();

	if (temphe)
	{
		//这种情况，加入一条边，不构成新的面
		//newhe1所指向的点为新创建的点
		//该点所指向的任何一个半边都没有链入到环中
		//并且传进来的边也没有任何一条半边链入到任何一个环中
		if ((!newhe1->getPoint()->getHE()->getLoop()) && (!newhe1->getLoop()) && (!newhe2->getLoop()))
		{
			while (temphe->getPoint() != newhe2->getPoint())
				temphe = temphe->getNext();

			forwardInsertHE(temphe, newhe1);
			forwardInsertHE(newhe1, newhe2);

			//注意，当前环的起始半边始终为newhe1
			_startHE = newhe1;
		}
		//这种情况，加入一条边，构成新的面
		//当前的抽象环将分裂成2个环，一个是实体环，一个是抽象环
		//newhe1所指向的点为已经存在的点
		//该点所指向的半边已经链入到环中，但是传进来的边中没有任何一条半边链入到任何一个环中
		else if ((newhe1->getPoint()->getHE()->getLoop()) && (!newhe1->getLoop()) && (!newhe2->getLoop()))
		{
			HalfEdge *temphe1, *temphe2;
			temphe1 = temphe2 = temphe;

			while (temphe1->getPoint() != newhe2->getPoint())
				temphe1 = temphe1->getNext();
			while (temphe2->getPoint() != newhe1->getPoint())
				temphe2 = temphe2->getNext();

			temphe1->getPrev()->setNext(newhe2);
			newhe2->setPrev(temphe1->getPrev());
			newhe2->setNext(temphe2);
			temphe1->setPrev(newhe1);

			temphe2->getPrev()->setNext(newhe1);
			newhe1->setPrev(temphe->getPrev());
			newhe1->setNext(temphe1);
			temphe2->setPrev(newhe2);
		}
		//这种情况，分裂一条边成为两条边，重新链入到环中
		//此时，传进来的边中有且仅有一条半边链入到一个环中
		else if ((newhe1->getLoop()) || (newhe2->getLoop()))
		{
			while (temphe->getPoint() != newhe2->getPoint())
				temphe = temphe->getNext();

			forwardInsertHE(temphe, newhe1);
		}
	}

	else
	{
		//这种情况，为创建实体时的初始情况
		forwardInsertHE(newhe1, newhe2);
		//注意，当前环的起始半边始终为newhe1
		_startHE = newhe1;
	}
}

void Loop::splitLoop(Edge *refE, Loop *newL)
{
	HalfEdge *newhe1 = refE->getHE1();
	HalfEdge *newhe2 = refE->getHE2();

	HalfEdge *temphe1 = newhe1;
	HalfEdge *temphe2 = newhe2;

	//从内、外两个方向同时遍历环
	//目的（1）：知道哪一个环是抽象环（抽象环遍历环所需要走过的半边更多）
	do
	{
		temphe1 = temphe1->getNext();
		temphe2 = temphe2->getNext();
	} while (temphe1 != newhe1 && temphe2 != newhe2);

	//TEMPHE1(起始点始终指向序号较大的点)首先遍历完
	//可以确定TMEPHE1所在的环不是抽象环
	//让新形成的面中的外环指向以序号较大的这个点为起始点的半边
	//这样可以保证平面给出的顺序与传进来的参数顺序一致

	//只要保证第一个面形成的顺序跟输入的顺序一致，其他所有面的顺序也就唯一确定
	HalfEdge *tempHE1 = (temphe1 == newhe1) ? temphe1 : temphe2;
	HalfEdge *setupHE = (temphe1 == newhe1) ? newhe1 : newhe2;

	newL->setHE(setupHE);
	newL->_numHE = 0;
	do
	{
		tempHE1->setLoop(newL);
		tempHE1 = tempHE1->getNext();
		++newL->_numHE;
	} while (tempHE1 != setupHE);

	_startHE = setupHE->getMate();
	HalfEdge *tempHE2 = _startHE;
	this->_numHE = 0;
	do
	{
		tempHE2->setLoop(this);
		tempHE2 = tempHE2->getNext();
		++this->_numHE;
	} while (tempHE2 != _startHE);
}

const osg::ref_ptr<osg::Vec3dArray> Loop::getPoints()
{
	osg::ref_ptr<osg::Vec3dArray> points = new osg::Vec3dArray;

	HalfEdge * he = _startHE;
	do
	{
		points->push_back(he->getPoint()->getPoint());
		he = he->getNext();
	} while (he != _startHE);

	return points.release();
}

const edgelist Loop::getFlagEdge() const
{
	const HalfEdge *he = _startHE;
	edgelist list;
	do
	{
		if (he->getEdge()->getEdgeFlag())
		{
			list.push_back(he->getEdge());
		}
		he = he->getNext();
	} while (he != _startHE);

	return list;
}

const osg::ref_ptr<osg::Vec3dArray> Loop::getNavigationEdge() const
{
	const HalfEdge *he = _startHE;
	osg::ref_ptr<osg::Vec3dArray> navigation = new osg::Vec3dArray;
	osg::ref_ptr<osg::Vec3dArray> edgeA = new osg::Vec3dArray;
	osg::ref_ptr<osg::Vec3dArray> edgeB = new osg::Vec3dArray;

	do
	{
		if (he->getEdge()->getEdgeFlag())
		{
			if (he->getEdge()->getEdgeFlag()->_navigateEdge)
			{
				osg::ref_ptr<osg::Vec3dArray> tempArray = (edgeA->empty()) ? edgeA : edgeB;
				*tempArray = *(he->getEdge()->getEdgeFlag()->_navigationArray);
			}
		}
		he = he->getNext();
	} while (he != _startHE);

	navigation->push_back(edgeA->front()*0.5f + edgeB->front()*0.5f);
	navigation->push_back(edgeA->back()*0.5f + edgeB->back()*0.5f);

	return navigation.release();
}

bool Loop::ifPointinLoop(const osg::Vec3d &p)
{
	const HalfEdge *he = _startHE;

	do 
	{
		if (p == he->getPoint()->getPoint())
		{
			return true;
		}

		he = he->getNext();
	} while (he != _startHE);

	return false;
}

void Loop::setTexCoord(unsigned int unit, osg::Array* array, osg::Array::Binding binding /* = osg::Array::BIND_UNDEFINED */)
{
	_homeP->getHomeS()->setTexMode(true);
	setTexCoordArray(unit, array, binding);
}

void Loop::multiplyMatrix(const osg::Matrixd &m)
{
	HalfEdge *he = _startHE;
	const HalfEdge * const start = _startHE;
	do 
	{
		osg::Vec3 p = he->getPoint()->getPoint();
		p = p * m;
		he->getPoint()->setPoint(p);
		he = he->getNext();
	} while (he != start);

	draw();

	dirtyBound();
	dirtyDisplayList();	
}