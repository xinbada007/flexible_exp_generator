#include "stdafx.h"
#include "loop.h"
#include "halfedge.h"
#include "edge.h"
#include "points.h"
#include "plane.h"

Loop::Loop():
_startHE(NULL), _prev(NULL), _next(NULL), _homeP(NULL),
_switch(true)
{

}

Loop::Loop(const Loop &copy, osg::CopyOp copyop /* = osg::CopyOp::SHALLOW_COPY */) :
osg::Geometry(copy,copyop),
_startHE(copy.getHE()), _prev(copy.getPrev()), _next(copy.getNext()), _homeP(copy.getHomeP()),
_switch(copy.getSwitch())
{

}

Loop::~Loop()
{
	osg::notify(osg::NOTICE) << "deleting Loop..." << std::endl;
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
		//�������������һ���ߣ��������µ���
		//newhe1��ָ��ĵ�Ϊ�´����ĵ�
		//�õ���ָ����κ�һ����߶�û�����뵽����
		//���Ҵ������ı�Ҳû���κ�һ��������뵽�κ�һ������
		if ((!newhe1->getPoint()->getHE()->getLoop()) && (!newhe1->getLoop()) && (!newhe2->getLoop()))
		{
			while (temphe->getPoint() != newhe2->getPoint())
				temphe = temphe->getNext();

			forwardInsertHE(temphe, newhe1);
			forwardInsertHE(newhe1, newhe2);

			//ע�⣬��ǰ������ʼ���ʼ��Ϊnewhe1
			_startHE = newhe1;
		}
		//�������������һ���ߣ������µ���
		//��ǰ�ĳ��󻷽����ѳ�2������һ����ʵ�廷��һ���ǳ���
		//newhe1��ָ��ĵ�Ϊ�Ѿ����ڵĵ�
		//�õ���ָ��İ���Ѿ����뵽���У����Ǵ������ı���û���κ�һ��������뵽�κ�һ������
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
		//�������������һ���߳�Ϊ�����ߣ��������뵽����
		//��ʱ���������ı������ҽ���һ��������뵽һ������
		else if ((newhe1->getLoop()) || (newhe2->getLoop()))
		{
			while (temphe->getPoint() != newhe2->getPoint())
				temphe = temphe->getNext();

			forwardInsertHE(temphe, newhe1);
		}
	}

	else
	{
		//���������Ϊ����ʵ��ʱ�ĳ�ʼ���
		forwardInsertHE(newhe1, newhe2);
		//ע�⣬��ǰ������ʼ���ʼ��Ϊnewhe1
		_startHE = newhe1;
	}
}

void Loop::splitLoop(Edge *refE, Loop *newL)
{
	HalfEdge *newhe1 = refE->getHE1();
	HalfEdge *newhe2 = refE->getHE2();

	HalfEdge *temphe1 = newhe1;
	HalfEdge *temphe2 = newhe2;

	//���ڡ�����������ͬʱ������
	//Ŀ�ģ�1����֪����һ�����ǳ��󻷣����󻷱���������Ҫ�߹��İ�߸��ࣩ
	do
	{
		temphe1 = temphe1->getNext();
		temphe2 = temphe2->getNext();
	} while (temphe1 != newhe1 && temphe2 != newhe2);

	//TEMPHE1(��ʼ��ʼ��ָ����Žϴ�ĵ�)���ȱ�����
	//����ȷ��TMEPHE1���ڵĻ����ǳ���
	//�����γɵ����е��⻷ָ������Žϴ�������Ϊ��ʼ��İ��
	//�������Ա�֤ƽ�������˳���봫�����Ĳ���˳��һ��

	//ֻҪ��֤��һ�����γɵ�˳��������˳��һ�£������������˳��Ҳ��Ψһȷ��
	HalfEdge *tempHE1 = (temphe1 == newhe1) ? temphe1 : temphe2;
	HalfEdge *setupHE = (temphe1 == newhe1) ? newhe1 : newhe2;

	newL->setHE(setupHE);
	do
	{
		tempHE1->setLoop(newL);
		tempHE1 = tempHE1->getNext();
	} while (tempHE1 != setupHE);

	_startHE = setupHE->getMate();
	HalfEdge *tempHE2 = _startHE;
	do
	{
		tempHE2->setLoop(this);
		tempHE2 = tempHE2->getNext();
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