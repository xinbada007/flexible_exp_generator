#include "stdafx.h"
#include "obstacle.h"
#include "halfedge.h"
#include "points.h"

Obstacle::Obstacle()
{
	_tag = OBS;
}

Obstacle::Obstacle(const Obstacle &copy, osg::CopyOp copyop /* = osg::CopyOp::SHALLOW_COPY */):
LogicRoad(copy,copyop)
{

}

Obstacle::~Obstacle()
{
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

void Obstacle::genTextureNoZ()
{
	Plane::reverse_iterator i = this->getLastPlane();
	const osg::Vec3d zN1(0.0f, 0.0f, 1.0f);
	const osg::Vec3d zN2(0.0f, 0.0f, -1.0f);
	
	unsigned total(0);
	while (*i)
	{
		planeEQU pe = (*i)->getLoop()->getPlaneEQU();
		const osg::Vec3d normal(pe.at(0), pe.at(1), pe.at(2));

		if (!isEqual(normal,zN1) && !isEqual(normal,zN2))
		{
			++total;
		}
		i++;
	}

	double startI = 0.0f;
	double step = 1.0f / double (total);
	double endI = startI + step;
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
			(*i)->getLoop()->setTexCoordArray(0, tex.release());
			
			startI += step;
			endI += step;
		}
		++i;
	}

	if (this->getTexCoord())
	{
		this->setTexCoord(NULL);
	}
}

void Obstacle::genUnifiedTexture()
{
	Plane::reverse_iterator i = this->getLastPlane();
	const osg::Vec3d zN1(0.0f, 0.0f, 1.0f);
	const osg::Vec3d zN2(0.0f, 0.0f, -1.0f);

	while (*i)
	{
		planeEQU pe = (*i)->getLoop()->getPlaneEQU();
		const osg::Vec3d normal(pe.at(0), pe.at(1), pe.at(2));
		if (!isEqual(normal, zN1) && !isEqual(normal, zN2))
		{
			osg::ref_ptr<osg::Vec2Array> tex = new osg::Vec2Array;
			tex->push_back(osg::Vec2(0.0f, 0.0f));
			tex->push_back(osg::Vec2(1.0f, 0.0f));
			tex->push_back(osg::Vec2(1.0f, 1.0f));
			tex->push_back(osg::Vec2(0.0f, 1.0f));
			(*i)->getLoop()->setTexCoordArray(0, tex.release());
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

	genTextureNoZ();
}

void Obstacle::createBox(osg::Vec3d center, osg::Vec3d radius)
{
	osg::Vec3d left_bottom = center * osg::Matrix::translate(-radius*0.5f);
	left_bottom.z() = 0.0f;

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

	genBoxTexture();
}

void Obstacle::createSphere(const osg::Vec3d &centre, const double &radius)
{
//	const unsigned segment = (double(radius) / 1.0f) * 24;
	const unsigned segment = 32;
	const int L = 18;
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
			p = p * osg::Matrix::translate(centre);
			circle->push_back(p);
		}
		if (!circle->empty())
		{
			sphereList.push_back(circle.release());
		}
	}

	//Create Cylinder first
	const double height = R*(double(L - 1) / double(L));
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
	std::vector<osg::ref_ptr<osg::Vec3dArray>>::const_iterator iter_next;
	osg::Vec3dArray::const_iterator pIter;
	osg::Vec3dArray::const_iterator pNext;
	osg::Vec3dArray::const_iterator pNIter;

	std::vector<Loop *> loopList;
	loopList.resize(iter_end - sphereList.cbegin() + 1);
	int test(1);

	while (iter_begin != iter_end)
	{
		iter_next = iter_begin + 1;
		pIter = (*iter_begin)->begin();
		pNext = (*iter_next)->begin();
		pNIter = (*iter_end)->begin();
		Loop *l(NULL);
		while (pIter != (*iter_begin)->end())
		{
			Edge *mEdge = findEdge(*pIter, *pNIter);
			Edge *newE = insPt(mEdge, *pNext);
			pIter++;
			pNIter++;
			pNext++;
		}		
		iter_begin++;
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
				l->getHomeP()->setAbstract(true);
				link(*pPre, *pIter);
				l->getHomeP()->setAbstract(false);
			}
			pIter++;
		}

		Loop *l = findLoop((*iter_begin)->front(), (*iter_begin)->back());
		if (l)
		{
			l->getHomeP()->setAbstract(true);
			link((*iter_begin)->front(), (*iter_begin)->back());
			l->getHomeP()->setAbstract(false);
		}
		iter_begin++;
	}

	abs = this->getAbstract();
	if (abs)
	{
		abs->setAbstract(false);
	}

	genUnifiedTexture();
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