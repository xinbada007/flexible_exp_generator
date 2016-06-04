#include "stdafx.h"
#include "obstacle.h"
#include "halfedge.h"
#include "points.h"
#include "renderVistor.h"
#include "textureVisitor.h"

#include <osgDB/ReadFile>
#include <osgDB/WriteFile>
#include <osg/MatrixTransform>
#include <osgUtil/Optimizer>
#include <osg/Point>

#include <algorithm>

Obstacle::Obstacle():
_objNode(NULL)
{
	_tag = ROADTAG::OBS;
}

Obstacle::Obstacle(const Obstacle &copy, osg::CopyOp copyop /* = osg::CopyOp::SHALLOW_COPY */):
LogicRoad(copy, copyop), _objNode(copy._objNode)
{

}

Obstacle::~Obstacle()
{
	_objNode = NULL;
}

void Obstacle::genBoxTexture()
{
	osg::ref_ptr<osg::Vec2Array> tex = new osg::Vec2Array;
	Plane::reverse_iterator i = this->getLastPlane();
	while (*i)
	{
		tex->push_back(osg::Vec2(0.0f, 0.0f));
		tex->push_back(osg::Vec2(1.0f, 0.0f));
		tex->push_back(osg::Vec2(1.0f, 1.0f));
		tex->push_back(osg::Vec2(0.0f, 1.0f));
		i++;
	}
	this->setTexCoord(tex);
}

void Obstacle::genTextureNoZ(const bool perGmtry /* = false */)
{
	Plane::reverse_iterator i = this->getLastPlane();
	const osg::Vec3d zN1(0.0f, 0.0f, 1.0f);
	const osg::Vec3d zN2(0.0f, 0.0f, -1.0f);
	
	double startI = 0.0f;
	double step = 0.0f;
	double endI = 1.0f;
	if (!perGmtry)
	{
		unsigned total(0);
		while (*i)
		{
			planeEQU pe = (*i)->getLoop()->getPlaneEQU();
			const osg::Vec3d normal(pe.at(0), pe.at(1), pe.at(2));

			if (!isEqual(normal, zN1) && !isEqual(normal, zN2))
			{
				++total;
			}
			i++;
		}

		startI = 0.0f;
		step = 1.0f / double(total);
		endI = startI + step;
	}

	i = this->getLastPlane();
	while (*i)
	{
		planeEQU pe = (*i)->getLoop()->getPlaneEQU();
		const osg::Vec3d normal(pe.at(0), pe.at(1), pe.at(2));
		if (!isEqual(normal, zN1) && !isEqual(normal, zN2))
		{
			osg::ref_ptr<osg::Vec2Array> tex = new osg::Vec2Array;
			tex->push_back(osg::Vec2(startI, 0.0f));
			tex->push_back(osg::Vec2(endI, 0.0f));
			tex->push_back(osg::Vec2(endI, 1.0f));
			tex->push_back(osg::Vec2(startI, 1.0f));
			(*i)->setTexCoord(0, tex.release());
			
			startI += step;
			endI += step;
		}
		else
		{
			(*i)->getLoop()->setSwitch(false);
		}
		++i;
	}

	if (this->getTexCoord())
	{
		this->setTexCoord(NULL);
	}
}

void Obstacle::createCylinder(const osg::Vec3d &center, const double &radius, const double &height)
{
//	const unsigned segment = (double(radius) / 1.0f) * 96;
	const unsigned segment = 96;
	const double R = radius*0.5f;
	osg::ref_ptr<osg::Vec3dArray> cylinder = new osg::Vec3dArray;

	for (unsigned i = 0; i < segment;i++)
	{
		const double theta = (double(i) / double(segment)) * 2 * PI;
		const double x = R * cos(theta);
		const double y = R * sin(theta);
		const double z = 0.0f;

		osg::Vec3d p(x, y, z);
		p = p * osg::Matrix::translate(center);
		cylinder->push_back(p);
	}

	osg::Vec3dArray::const_iterator iter = cylinder->begin();
	while (iter != cylinder->end())
	{
		osg::Vec3dArray::const_iterator pre_iter = iter++;
		if (pre_iter != cylinder->end() && iter != cylinder->end())
		{
			link(*pre_iter, *iter);
		}
	}
	link(cylinder->front(), cylinder->back());

	sweep(height);

	Plane *abs = this->getAbstract();
	if (abs)
	{
		abs->setAbstract(false);
	}

	this->absoluteTerritory.center.set(center.x(), center.y(), center.z() + height*0.5f);
	this->absoluteTerritory._detectR = 2 * std::max(radius, height);
	this->absoluteTerritory._refuseR = 0.0f;

	genTextureNoZ();

	RenderVistor rv;
	rv.setBeginMode(GL_QUADS);
	this->accept(rv);

	TextureVisitor tv;
	this->accept(tv);
}

void Obstacle::createBox(osg::Vec3d center, osg::Vec3d radius)
{
	osg::Vec3d left_bottom = center * osg::Matrix::translate(-radius*0.5f);
	left_bottom.z() = center.z();

	osg::Vec3d right_bottom = left_bottom;
	right_bottom.x() += radius.x();

	osg::Vec3d right_top = right_bottom;
	right_top.y() += radius.y();

	osg::Vec3d left_top = right_top;
	left_top.x() -= radius.x();

	link(left_bottom, right_bottom);
	link(right_bottom, right_top);
	link(right_top, left_top);
	link(left_top, left_bottom);

	sweep(radius.z());

	Plane *abs = this->getAbstract();
	if (abs)
	{
		abs->setAbstract(false);
	}

	this->absoluteTerritory.center.set(center.x(),center.y(),center.z() + radius.z()*0.5f);
	this->absoluteTerritory._detectR = pow(std::max(radius.x(), radius.y()), 2) + pow((radius.z()*0.5f), 2);
	this->absoluteTerritory._detectR = 2*sqrt(this->absoluteTerritory._detectR);
	this->absoluteTerritory._refuseR = 0.0f;//std::min(radius.x(), radius.y());

	genBoxTexture();

	RenderVistor rv;
	rv.setBeginMode(GL_QUADS);
	this->accept(rv);

	TextureVisitor tv;
	this->accept(tv);
}

void Obstacle::createSphere(const osg::Vec3d &centre, const double &radius)
{
//	const unsigned segment = (double(radius) / 1.0f) * 24;
	const osg::Vec3d center(centre.x(), centre.y(), centre.z() + radius*0.5f);
	const unsigned segment = 32;
	const int L = 16;
	const double R = radius*0.5f;
	std::vector<osg::ref_ptr<osg::Vec3dArray>> sphereList;

	for (int i = -(L-1); i < L; i++)
	{
		const double r = sqrt(R*R - (pow(((double(abs(i)) / double(L))*R), 2)));
		osg::ref_ptr<osg::Vec3dArray> circle = new osg::Vec3dArray;
		for (unsigned j = 0; j < segment; j++)
		{
			const double theta = (double(j) / double(segment)) * 2 * PI;
			const double x = r * cos(theta);
			const double y = r * sin(theta);
			const double z = R *((double)i / double(L));

			osg::Vec3d p(x, y, z);
			p = p * osg::Matrix::translate(center);
			circle->push_back(p);
		}
		if (!circle->empty())
		{
			sphereList.push_back(circle.release());
		}
	}

	//Create Cylinder first
	line1D(sphereList.front());
	link(sphereList.front()->front(), sphereList.front()->back());
	sweep(sphereList.back());
	Plane *abs = this->getAbstract();
	if (abs)
	{
		abs->setAbstract(false);
	}

	//Then stretch it until it becomes a Sphere
	std::vector<osg::ref_ptr<osg::Vec3dArray>>::const_iterator iter_begin = sphereList.cbegin();
	const std::vector<osg::ref_ptr<osg::Vec3dArray>>::const_iterator iter_end = sphereList.cend() - 1;
	std::vector<osg::ref_ptr<osg::Vec3dArray>>::const_iterator iter_next = iter_begin + 1;
	osg::Vec3dArray::const_iterator pIter;
	osg::Vec3dArray::const_iterator pNext;
	osg::Vec3dArray::const_iterator pNIter;

	while (iter_next != iter_end)
	{
		pIter = (*iter_begin)->begin();
		pNext = (*iter_next)->begin();
		pNIter = (*iter_end)->begin();
		while (pIter != (*iter_begin)->end())
		{
			Edge *mEdge = findEdge(*pIter, *pNIter);
			insPt(mEdge, *pNext);
			pIter++;
			pNIter++;
			pNext++;
		}		
		++iter_begin;
		++iter_next;
	}

	//Finally subdivide big Polygon into small polygons
	abs = this->getAbstract();
	if (abs)
	{
		abs->setAbstract(false);
	}

	iter_begin = sphereList.cbegin() + 1;
	while (iter_begin != iter_end)
	{
		pIter = (*iter_begin)->begin() + 1;
		while (pIter != (*iter_begin)->end())
		{
			osg::Vec3dArray::const_iterator pPre = pIter - 1;
			Loop *l = findLoop(*pPre, *pIter);
			if (l)
			{
				if (!findEdge(*pPre, *pIter))
				{
					l->getHomeP()->setAbstract(true);
					link(*pPre, *pIter);
					l->getHomeP()->setAbstract(false);
				}
			}
			pIter++;
		}

		Loop *l = findLoop((*iter_begin)->front(), (*iter_begin)->back());
		if (l)
		{
			if (!findEdge((*iter_begin)->front(), (*iter_begin)->back()))
			{
				l->getHomeP()->setAbstract(true);
				link((*iter_begin)->front(), (*iter_begin)->back());
				l->getHomeP()->setAbstract(false);
			}
		}
		iter_begin++;
	}

	abs = this->getAbstract();
	if (abs)
	{
		abs->setAbstract(false);
	}

	this->absoluteTerritory.center = center;
	this->absoluteTerritory._detectR = R * 2.0f;
	this->absoluteTerritory._refuseR = R;

	genTextureNoZ(true);

	RenderVistor rv;
	rv.setBeginMode(GL_QUADS);
	this->accept(rv);

	TextureVisitor tv;
	this->accept(tv);
}

void Obstacle::createPoint(const osg::Vec3d &center, const double &PS /* = 10.0f */)
{
	osg::ref_ptr<osg::Geode> geode = new osg::Geode;

	osg::ref_ptr<osg::Geometry> gemtry = new osg::Geometry;
	osg::ref_ptr<osg::Vec3dArray> vertex = new osg::Vec3dArray;
	vertex->push_back(center);
	gemtry->setVertexArray(vertex);
	gemtry->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, vertex->getNumElements()));
	osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
	color->push_back(osg::Vec4(1.0f, 0.0f, 0.0f, 1.0f));
	gemtry->setColorArray(color);
	gemtry->setColorBinding(osg::Geometry::BIND_OVERALL);

	osg::ref_ptr<osg::Point> psize = new osg::Point(PS);
	osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
	ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::OVERRIDE);
	ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
	ss->setAttribute(psize);
	gemtry->setStateSet(ss);

	osg::BoundingBoxd bb = gemtry->getBoundingBox();
	bb.expandBy(center);
	osg::Vec3d A(center);
	A.x() += 10.0f;
	bb.expandBy(A);
	osg::Vec3d B(center);
	B.x() -= 10.0f;
	bb.expandBy(B);
	gemtry->setInitialBound(bb);

	geode->addDrawable(gemtry);

	this->absoluteTerritory.center = center;
	this->absoluteTerritory._detectR = 1e-6;
	this->absoluteTerritory._refuseR = 1e-6;

	this->addChild(geode);
}

void Obstacle::sweep(osg::ref_ptr<osg::Vec3dArray> swArray)
{
	Solid *refS = dynamic_cast<Solid*> (this);
	if (!refS)
	{
		osg::notify(osg::FATAL) << "Cannot Sweep! Solid is not exist!" << std::endl;
		return;
	}

	if (!refS->getNumPlanes())
	{
		osg::notify(osg::FATAL) << "Cannot Sweep! Solid is not created yet!" << std::endl;
		return;
	}

	if (swArray->getNumElements() != refS->getNumPoints())
	{
		osg::notify(osg::FATAL) << "Cannot Sweep! Points are inconsistent!" << std::endl;
		return;
	}

	Plane *p = refS->getPlane();
	Loop *l = p->getLoop();
	HalfEdge *startHE = l->getHE();
	const HalfEdge *const endHE = startHE;

	osg::Vec3d firstP = swArray->at(startHE->getPoint()->getIndex() - 1);
	osg::Vec3d thisP = firstP;
	osg::Vec3d lastP = thisP;

	do
	{
		thisP = swArray->at(startHE->getPoint()->getIndex() - 1);
		link(startHE->getPoint()->getPoint(), thisP);

		if (lastP != thisP)
		{
			link(lastP, thisP);
		}

		lastP = thisP;
		startHE = startHE->getNext();
	} while (startHE != endHE);

	link(firstP, lastP);
}

void Obstacle::sweep(const double height)
{
	Solid *refS = dynamic_cast<Solid*> (this);
	if (!refS)
	{
		osg::notify(osg::FATAL) << "Cannot Sweep! Solid is not exist!" << std::endl;
		return;
	}

	if (!refS->getNumPlanes())
	{
		osg::notify(osg::FATAL) << "Cannot Sweep! Solid is not created yet!" << std::endl;
		return;
	}

	Plane *p = refS->getPlane();
	Loop *l = p->getLoop();
	HalfEdge *startHE = l->getHE();
	const HalfEdge *const endHE = startHE;

	osg::Vec3d firstP = startHE->getPoint()->getPoint();
	firstP.z() += height;
	osg::Vec3d thisP = firstP;
	osg::Vec3d lastP = thisP;

	do 
	{
		thisP = startHE->getPoint()->getPoint();
		thisP.z() += height;
		link(startHE->getPoint()->getPoint(), thisP);

		if (lastP != thisP)
		{
			link(lastP, thisP);
		}
	
		lastP = thisP;
		startHE = startHE->getNext();
	} while (startHE != endHE);

	link(firstP, lastP);
}

void Obstacle::createNode(const std::string file, const osg::Vec3d &CENTER /* = H_POINT */)
{
	osg::ref_ptr<osg::Node> node = osgDB::readNodeFile(file);
	if (!node)
	{
		osg::notify(osg::WARN) << "cannot find node file!" << std::endl;
		return;
	}
	BoundingboxVisitor bbV;
	node->accept(bbV);
	const osg::BoundingBoxd &bb = bbV.getBoundingBox();

 	osg::Vec3d minP(bb.xMin(), bb.yMin(), bb.zMin());
	osg::Vec3d maxP(bb.xMax(), bb.yMax(), bb.zMax());
	osg::Vec3d center = (minP + maxP)*0.5f;
	osg::Matrix m;
	m *= osg::Matrix::translate(CENTER);

	_objNode = node.release();
	osg::ref_ptr<osg::MatrixTransform> mT = new osg::MatrixTransform;
	mT->addChild(_objNode);
	mT->setMatrix(m);
	this->addChild(mT);

	mT->setDataVariance(osg::Object::STATIC);
	_objNode->setDataVariance(osg::Object::STATIC);
	osgUtil::Optimizer op;
	op.optimize(this, osgUtil::Optimizer::FLATTEN_STATIC_TRANSFORMS);
	op.optimize(_objNode, osgUtil::Optimizer::ALL_OPTIMIZATIONS);

	mT->removeChild(_objNode);
	this->removeChild(mT);
	this->addChild(_objNode);

	osg::ref_ptr<osg::Vec3dArray> a = new osg::Vec3dArray;
	osg::ref_ptr<osg::Vec3dArray> b = new osg::Vec3dArray;
	getCCWCubefromLBfromBBox(bb, a, b);
	arrayByMatrix(a, m);
	arrayByMatrix(b, m);
	center = center * m;
	this->absoluteTerritory.center.set(center);
	this->absoluteTerritory._detectR = bb.radius2();
	this->absoluteTerritory._refuseR = 0.0f;

	line1D(a);
	link(a->front(), a->back());
	sweep(b);
	Plane *abs = this->getAbstract();
	if (abs)
	{
		abs->setAbstract(false);
	}

// 	RenderVistor rv;
// 	rv.setBeginMode(GL_QUADS);
// 	this->accept(rv);
}

void Obstacle::multiplyMatrix(const osg::Matrixd &m, const bool substitute /* = true */)
{
	osg::ref_ptr<osg::MatrixTransform> mt = dynamic_cast<osg::MatrixTransform*>(this->getParent(0));
	if (!mt)
	{
		mt = new osg::MatrixTransform;
		this->addParent(mt);
	}
	else
	{
		substitute ? mt->setMatrix(m) : mt->setMatrix(m * mt->getMatrix());
	}

	Points *p = this->getPoint();
	while (p)
	{
		p->setPoint(p->getPoint() * m);
		p = p->getNext();
	}

	Plane *pl = this->getPlane();
	while (pl)
	{
		Loop *lp = pl->getLoop();
		while (lp)
		{
			calcPlaneEQU(lp);
			lp = lp->getNext();
		}
		pl = pl->getNext();
	}

	absoluteTerritory.center = absoluteTerritory.center * m;
	osg::Vec3d test = absoluteTerritory.center;
}