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
	
	unsigned total(0);
	while (*i)
	{
		planeEQU pe = (*i)->getLoop()->getPlaneEQU();
		const osg::Vec3d normal(pe.at(0), pe.at(1), pe.at(2));
		const osg::Vec3d zN1(0.0f, 0.0f, 1.0f);
		const osg::Vec3d zN2(0.0f, 0.0f, -1.0f);

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
		if (!isEqual(abs(pe.at(2)), 1.0f))
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
		i++;
	}

	if (this->getTexCoord())
	{
		this->setTexCoord(NULL);
	}
}

void Obstacle::createCylinder(const osg::Vec3d &center, const double &radius, const double &height)
{
	const unsigned segment = (double(radius) / 1.0f) * 180;
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

	genBoxTexture();
}

void Obstacle::createSphere(const osg::Vec3d &centre)
{
	const double radius = 1.0f;
	const unsigned segment = (double(radius) / 1.0f) * 5;
	const unsigned L = 5;
	const double R = radius*0.5f;
	std::vector<osg::ref_ptr<osg::Vec3dArray>> sphereList;
//	std::vector<osg::ref_ptr<osg::Vec3dArray>> sphereList1;

	for (unsigned i = 0; i < L; i++)
	{
		const double r = sqrt(R*R - (pow(((double(i) / double(L))*R), 2)));
		osg::ref_ptr<osg::Vec3dArray> circle = new osg::Vec3dArray;
//		osg::ref_ptr<osg::Vec3dArray> circle1 = new osg::Vec3dArray;
		for (unsigned j = 0; j < segment; j++)
		{
			const double theta = (double(j) / double(segment)) * 2 * PI;
			const double x = r * cos(theta);
			const double y = r * sin(theta);
			const double z = R *((double)i / double(L));

			osg::Vec3d p(x, y, z);
			p = p * osg::Matrix::translate(centre);
			circle->push_back(p);

// 			if (abs(z) > 0)
// 			{
// 				osg::Vec3d p1(p);
// 				p1.z() = -z;
// 				circle1->push_back(p1);
// 			}
		}
		if (!circle->empty())
		{
			sphereList.push_back(circle.release());
		}
// 		if (!circle1->empty())
// 		{
// 			sphereList1.push_back(circle1.release());
// 		}
	}



	
// 	unsigned linkArray[L];
// 	memset(linkArray, 0, sizeof(linkArray));
// 	std::vector<osg::ref_ptr<osg::Vec3dArray>>::const_iterator iter = sphereList.cbegin();
// 	const std::vector<osg::ref_ptr<osg::Vec3dArray>>::const_iterator startIter = sphereList.cbegin();
// 	while (iter != sphereList.cend())
// 	{
// 		std::vector<osg::ref_ptr<osg::Vec3dArray>>::const_iterator preIter = iter++;
// 
// 		if (preIter != sphereList.cend() && iter != sphereList.cend())
// 		{
// 			const unsigned pre = preIter - startIter;
// 			if (linkArray[pre] == 0)
// 			{
// 				this->line1D(*preIter);
// 				link((*preIter)->front(), (*preIter)->back());
// 				linkArray[pre] = 1;
// 			}
// 			this->line1D(*preIter, *iter);
// 			const unsigned thi = iter - startIter;
// 			if (linkArray[thi] == 0)
// 			{
// 				this->line1D(*iter);
// 				link((*iter)->front(), (*iter)->back());
// 				linkArray[iter - startIter] = 1;
// 			}
// 		}
// 	}

//	this->line1D(sphereList.front(), sphereList1.front());
//	this->line1D(sphereList1.front());

// 	{
// 		unsigned linkArray[L];
// 		memset(linkArray, 0, sizeof(linkArray));
// 		std::vector<osg::ref_ptr<osg::Vec3dArray>>::const_iterator iter = sphereList1.cbegin();
// 		const std::vector<osg::ref_ptr<osg::Vec3dArray>>::const_iterator startIter = sphereList1.cbegin();
// 		this->line1D(sphereList.front(), sphereList1.front());
// 		while (iter != sphereList1.cend())
// 		{
// 			std::vector<osg::ref_ptr<osg::Vec3dArray>>::const_iterator preIter = iter++;
// 
// 			if (preIter != sphereList1.cend() && iter != sphereList1.cend())
// 			{
// 				const unsigned pre = preIter - startIter;
// 				if (linkArray[pre] == 0)
// 				{
// 					this->line1D(*preIter);
// 					linkArray[pre] = 1;
// 				}
// 				this->line1D(*preIter, *iter);
// 				const unsigned thi = iter - startIter;
// 				if (linkArray[thi] == 0)
// 				{
// 					this->line1D(*iter);
// 					linkArray[iter - startIter] = 1;
// 				}
// 			}
// 		}
// 	}

		
// 	{
// 		sphereList.clear();
// 		for (unsigned i = 1; i < L; i++)
// 		{
// 			const double r = sqrt(R*R - (pow(((double(i) / double(L))*R), 2)));
// 			osg::ref_ptr<osg::Vec3dArray> circle = new osg::Vec3dArray;
// 			for (unsigned j = 0; j < segment; j++)
// 			{
// 				const double theta = (double(j) / double(segment)) * 2 * PI;
// 				const double x = r * cos(theta);
// 				const double y = r * sin(theta);
// 				const double z = -R *((double)i / double(L));
// 
// 				osg::Vec3d p(x, y, z);
// 				p = p * osg::Matrix::translate(centre);
// 				circle->push_back(p);
// 			}
// 			sphereList.push_back(circle.release());
// 		}
// 
// 		std::vector<osg::ref_ptr<osg::Vec3dArray>>::const_iterator iter = sphereList.cbegin();
// 		while (iter != sphereList.cend())
// 		{
// 			std::vector<osg::ref_ptr<osg::Vec3dArray>>::const_iterator preIter = iter++;
// 			if (preIter != sphereList.cend() && iter != sphereList.cend())
// 			{
// 				this->line1D(*preIter);
// 				this->line1D(*preIter, *iter);
// 				this->line1D(*iter);
// 			}
// 		}
// 	}
	
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

	Plane *abs = this->getAbstract();
	if (abs)
	{
		abs->setAbstract(false);
	}
}