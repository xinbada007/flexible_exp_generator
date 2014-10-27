#include "stdafx.h"
#include "math.h"

#include <algorithm>

extern double z_deepest = eps_1000;

osg::Vec3dArray * project_Line(const osg::Vec3dArray *refV, const double width)
{
	osg::ref_ptr <osg::Vec3dArray> results = new osg::Vec3dArray;

	const osg::Vec3dArray::const_iterator refV_begin = refV->begin();
	const osg::Vec3dArray::const_iterator refV_end = refV->end();

	//初始化第一条线段的平移
	const osg::Vec3d *p0 = refV_begin._Ptr;
	const osg::Vec3d *p1 = p0 + 1;
	osg::Vec3d project_0 = find_Prependicular(*p0, *p1, width);

	osg::Vec3d a = project_0 + *p0;
	osg::Vec3d b = project_0 + *p1;
	results->push_back(a);
	results->push_back(b);

	for (osg::Vec3dArray::const_iterator i = refV_begin + 1; (i + 1) != refV_end; i++)
	{
		const osg::Vec3d *p2 = (i + 1)._Ptr;
		p1 = p2 - 1;
		p0 = p1 - 1;

		const osg::Vec3d v2_p2p1 = *p2 - *p1;
		const osg::Vec3d v1_p1p0 = *p1 - *p0;
		const double dot_v2v1 = v2_p2p1*v1_p1p0;

		const osg::Vec3d cross_v2v1 = v2_p2p1^v1_p1p0;
		if (cross_v2v1.length() <= eps)
		{
			//此时两线段共线，投影致相同的点上
			b = project_0 + *p2;
			results->push_back(b);
			continue;
		}

		const osg::Vec3d project_1 = find_Prependicular(*p1, *p2, width);
		const osg::Vec3d project_2 = -project_1;
		const double dot_pro2pro1 = project_2*project_0;
		const double dot_pro1pro0 = project_1*project_0;
		osg::Vec3d project(0.0f, 0.0f, 0.0f);

		if (abs(dot_v2v1) <= eps)
		{
			//此时两线段垂直
			osg::notify(osg::NOTICE) << "Perpendicular Line Found!" << std::endl;
			project = project_1*v1_p1p0 > 0 ? project_1 : project_2;
			project = project_0*v2_p2p1 < 0 ? project : -project;
		}
		else
		{
			//此时，两线段既不共线，也不垂直，属于一般情况
			project = dot_pro1pro0*dot_v2v1 > 0 ? project_1 : project_2;
		}
		a = project + *p1;
		b = project + *p2;

		//此时取得了正确的平移向量，开始寻找交点
		const osg::Vec3dArray::iterator i_pro_now = results->end() - 1;
		const double &x_a = i_pro_now._Ptr->x();
		const double &y_a = i_pro_now._Ptr->y();
		const double &x_b = a.x();
		const double &y_b = a.y();
		const double &x_c = p1->x();
		const double &y_c = p1->y();

		const osg::Vec3d a_b = (a - *i_pro_now._Ptr);
		if (a_b.length() <= eps)
		{
			//此时两线段共线，投影致相同的点上
			b = project_0 + (*p2);
			results->push_back(b);
			continue;
		}

		const double &d = width;
		const double M = (d*d + (x_a*x_a + y_a*y_a) - (x_c*x_c + y_c*y_c));
		const double N = ((x_b*x_b + y_b*y_b) - (x_a*x_a + y_a*y_a));
		const double A = 2 * (x_a - x_c)*(y_a - y_b) - 2 * (x_b - x_a)*(y_c - y_a);

		const double y = (M*(x_b - x_a) - N*(x_a - x_c)) / A;
		const double x = (M*(y_a - y_b) - N*(y_c - y_a)) / A;

		if (abs(A) <= eps)
		{
			osg::notify(osg::WARN) << "A is very small!" << std::endl;
			osg::notify(osg::NOTICE) << "A == " << A << std::endl;
			osg::notify(osg::NOTICE) << "y == " << y << std::endl;
			osg::notify(osg::NOTICE) << "x == " << x << std::endl;
		}

		//如果交点落在反向延长线上，则该投影无几何意义
		const osg::Vec3d *p0_1 = (i_pro_now - 1)._Ptr;
		const osg::Vec3d p1_reset(x, y, 0.0f);
		const osg::Vec3d v_p1reset_p0 = p1_reset - *p0_1;
		const double direction_p0p1 = v_p1reset_p0*v1_p1p0;
		if (direction_p0p1 < 0)
		{
			//Cannot Projection
			//Reduce width
			//Or Resize Polygon
			osg::notify(osg::NOTICE) << "Cannot sweep, adjust width or check points" << std::endl;
			osg::notify(osg::NOTICE) << "(" << p0->x() << "," << p0->y() << ")\t(" << p1->x() << "," << p1->y() << ")\t(" << p2->x() << "," << p2->y() << ")" << std::endl;
			getchar();
		}
		i_pro_now._Ptr->x() = x;
		i_pro_now._Ptr->y() = y;
		results->push_back(b);
		project_0 = project;
	}

	return results.release();
}

osg::Vec3d find_Prependicular(const osg::Vec3d p0, const osg::Vec3d p1, const double width)
{
	//Watch Out!!!
	//Only for 2D(Z MUST = 0) Projection
	const double &x0 = p0.x();
	const double &y0 = p0.y();
	const double &x1 = p1.x();
	const double &y1 = p1.y();
	const double &d = width;

	double X(0.0);
	X = d*d * (y0 - y1)*(y0 - y1);
	X /= ((y0 - y1)*(y0 - y1) + (x1 - x0)*(x1 - x0));
	X = sqrt(X);

	double Y(0.0);
	Y = d*d * (x1 - x0)*(x1 - x0);
	Y /= ((y0 - y1)*(y0 - y1) + (x1 - x0)*(x1 - x0));
	if ((y0 - y1)*(x1 - x0) > 0)
	{
		//Y = X>0?sqrt(Y):(-sqrt(Y));
		Y = sqrt(Y);
	}
	else
	{
		//Y = X>0?(-sqrt(Y)):sqrt(Y);
		Y = -sqrt(Y);
	}

	osg::Vec3d P(X, Y, 0.0f);
	const osg::Vec3d p1_p0 = p1 - p0;

	if (abs(P*p1_p0) > eps)
	{
		osg::notify(osg::WARN) << "Projecting Fatal Error(No Perpendicular Bisector)" << std::endl;
		osg::notify(osg::WARN) << "dot_product = \t" << abs(P*p1_p0) << std::endl;
	}

	if (((P) ^ (p1_p0)).z()*width < 0.0f)
	{
		P = -P;
	}

	return P;
}

osg::Matrix rotate(const osg::Vec3dArray *source, const osg::Vec3dArray *des)
{
	osg::Vec3dArray::const_iterator i_source_end = source->end();
	osg::Vec3d srcp1 = *(--i_source_end);
	osg::Vec3d srcp2 = *(--i_source_end);
	osg::Vec3d srcv = srcp1 - srcp2;

	osg::Vec3dArray::const_iterator i_des_end = des->begin();
	osg::Vec3d desp1 = *(i_des_end++);
	osg::Vec3d desp2 = *(i_des_end);
	osg::Vec3d desv = desp2 = desp1;

	osg::Matrixd resluts(osg::Matrix::identity());
	resluts.makeRotate(desv, srcv);

	osg::Vec3d distance;
	desp1 = desp1 * resluts;
	distance = srcp1 - desp1;
	resluts.makeTranslate(distance);

	desp1 = desp1*resluts;
	osg::Vec3d srcv_desv_v = desp1 - srcp1;
	if (srcv * srcv_desv_v < 0)
	{
		resluts.makeTranslate((-srcv_desv_v)*1.1f*eps);
	}

	return resluts;
}

void arrayByMatrix(osg::Vec3dArray *source, const osg::Matrix refM)
{
	osg::Vec3dArray::iterator i = source->begin();
	for (; i != source->end(); i++)
	{
		(*i) = (*i) * (refM);
	}
}

osg::BoundingBox BoundingboxByMatrix(const osg::BoundingBox &refBB, const osg::Matrix &refM) /*WILL SET Z to Zero*/
{
	osg::Vec3d xminymin(refBB.xMin(), refBB.yMin(), 0.0f);
	osg::Vec3d xmaxymin(refBB.xMax(), refBB.yMin(), 0.0f);
	osg::Vec3d xmaxymax(refBB.xMax(), refBB.yMax(), 0.0f);
	osg::Vec3d xminymax(refBB.xMin(), refBB.yMax(), 0.0f);

	osg::ref_ptr<osg::Vec3dArray> xy = new osg::Vec3dArray;
	xy->push_back(xminymin); xy->push_back(xmaxymin); xy->push_back(xmaxymax); xy->push_back(xminymax);
	arrayByMatrix(xy, refM);

	osg::BoundingBox bb;
	osg::Vec3dArray::const_iterator i = xy->begin();
	do
	{
		bb.expandBy(*i++);
	} while (i != xy->end());

	return bb;
}

double ifPoints_ON_Plane(const osg::Vec3d refP, const planeEQU refEQU)
{
	if (refEQU.size() < 4)
	{
		return 0;
	}

	const double &x = refP.x();
	const double &y = refP.y();
	const double &z = refP.z();

	planeEQU::const_iterator i = refEQU.cbegin();
	const double &A = *i++;
	const double &B = *i++;
	const double &C = *i++;
	const double &D = *i;

	double S = A*x + B*y + C*z + D;

	if (isEqual(S, 0.0f))
	{
		return 0;
	}
	return S;
}

bool isEqual(const double &a, const double &p, const double &e /* = eps */)
{
	if (a == p)
	{
		return true;
	}

	double max = std::max(a, p);
	double min = std::min(a, p);

	if (min + eps >= max)
	{
		return true;
	}

	return false;
}

bool isEqual(const osg::Vec3d &p1, const osg::Vec3d &p2, const double &e /* = eps */)
{
	if (p1 == p2)
	{
		return true;
	}

	else if ((p1-p2).length() <= eps)
	{
		return true;
	}

	return false;
}

osg::ref_ptr<osg::Vec3dArray> linetoRectangle(osg::ref_ptr<osg::Vec3dArray> line)
{
	//Return a Rectangle under Counterclockwise starting from left-bottom
	//Only applies to 2D
	const double x1 = line->front().x();
	const double x2 = line->back().x();
	const double y1 = line->front().y();
	const double y2 = line->back().y();

	const double xmin = x1 < x2 ? x1 : x2;
	const double xmax = x1 < x2 ? x2 : x1;;
	const double ymin = y1 < y2 ? y1 : y2;
	const double ymax = y1 < y2 ? y2 : y1;

	osg::ref_ptr<osg::Vec3dArray> rect = new osg::Vec3dArray;
	rect->push_back(osg::Vec3d(xmin, ymin, 0.0f));
	rect->push_back(osg::Vec3d(xmax, ymin, 0.0f));
	rect->push_back(osg::Vec3d(xmax, ymax, 0.0f));
	rect->push_back(osg::Vec3d(xmin, ymax, 0.0f));

	return rect.release();
}

bool ifRectangleOverlap(osg::ref_ptr<osg::Vec3dArray> rectA, osg::ref_ptr<osg::Vec3dArray> rectB)
{
	//All Vertex in Rectangle must be given under Counterclockwise starting from left-bottom
	//Only applies to 2D

	//every rectangle should have at least 4 points
	if (rectA->size() < 4 || rectB->size() < 4)
	{
		return false;
	}

	osg::Vec3d A_left_bottom = *rectA->begin();
	osg::Vec3d A_right_bottom = *(rectA->begin() + 1);
	osg::Vec3d A_right_up = *(rectA->begin() + 2);
	osg::Vec3d A_left_up = *(rectA->begin() + 3);

	osg::Vec3d B_left_bottom = *rectB->begin();
	osg::Vec3d B_right_bottom = *(rectB->begin() + 1);
	osg::Vec3d B_right_up = *(rectB->begin() + 2);
	osg::Vec3d B_left_up = *(rectB->begin() + 3);

	if (A_right_bottom.x() < B_left_bottom.x() ||
		A_left_bottom.y() > B_left_up.y() ||
		A_left_bottom.x() > B_right_bottom.x() ||
		A_left_up.y() < B_left_bottom.y())
	{
		return false;
	}

	return true;
}

bool ifLineIntersects(osg::ref_ptr<osg::Vec3dArray> lineA, osg::ref_ptr<osg::Vec3dArray> lineB)
{
	//Only applies to 2D
	osg::Vec3d p1 = lineA->front();
	osg::Vec3d p2 = lineA->back();

	osg::Vec3d q1 = lineB->front();
	osg::Vec3d q2 = lineB->back();

	osg::Vec3d p1_q1 = p1 - q1;
	osg::Vec3d p2_q1 = p2 - q1;
	osg::Vec3d q2_q1 = q2 - q1;
	osg::Vec3d temp1 = p1_q1^q2_q1;
	osg::Vec3d temp2 = p2_q1^q2_q1;
	//double temp3 = temp1.z() * temp2.z();

	osg::Vec3d q1_p1 = q1 - p1;
	osg::Vec3d q2_p1 = q2 - p1;
	osg::Vec3d p2_p1 = p2 - p1;
	osg::Vec3d temp4 = q1_p1^p2_p1;
	osg::Vec3d temp5 = q2_p1^p2_p1;
	//double temp6 = temp4.z() * temp5.z();

	if (temp1*temp2 > 0.0f || temp4*temp5 > 0.0f)
	{
		return false;
	}

	return true;
}

bool ifPoint_IN_Polygon(const Point refP, const osg::ref_ptr<osg::Vec3dArray> refPoly, const planeEQU refEQU)
{
	//Only Applies to a 2D Point in a 2D Convex Polygon and this dimension must be Z=0
	//Not for General Polygon
	//All Vertex in Quad must be given under Counterclockwise

	//every polygon is a quad which should have at least 4 points
	if (refEQU.size() < 4)
	{
		return false;
	}

	planeEQU::const_iterator j = refEQU.cbegin();
	osg::Vec3d normal(*(j), *(j + 1), *(j + 2));
	normal.normalize();

	osg::Vec3dArray::const_iterator i = refPoly->begin();
	osg::Vec3d line_ab, line_ap;
	osg::Vec3d ab_ap;
	while ((i + 1) != refPoly->end())
	{
		line_ab = *(i + 1) - *i;
		line_ab.normalize();
		line_ap = refP - *i;
		line_ap.normalize();

		ab_ap = line_ab^line_ap;
		ab_ap.normalize();

		if (!isEqual(ab_ap*normal, 1.0f))
		{
			return false;
		}

		i++;
	}

	line_ab = *refPoly->begin() - *i;
	line_ap = refP - *i;
	ab_ap = line_ab^line_ap;
	ab_ap.normalize();
	if (!isEqual(ab_ap*normal, 1.0f))
	{
		return false;
	}

	return true;
}

double getZDeepth()
{
	double depth = z_deepest;
	z_deepest *= 2.0f;
	return depth;
}

osg::ref_ptr<osg::Vec3dArray> arrayLinearTransform(const osg::Vec3dArray *A, const osg::Vec3dArray *B, const double lamida)
{
	if (A->size() != B->size())
	{
		return NULL;
	}

	const double one_lamida = 1 - lamida;
	osg::ref_ptr<osg::Vec3dArray> dest = new osg::Vec3dArray;
	osg::Vec3dArray::const_iterator i = A->begin();
	osg::Vec3dArray::const_iterator j = B->begin();

	while (i != A->end() && j != B->end())
	{
		dest->push_back((*i) * lamida + (*j) * one_lamida);
		i++, j++;
	}

	return dest.release();
}

bool isletter(const char c)
{
	if (c >= 'a' && c <= 'z')
	{
		return true;
	}

	if (c >= 'A' && c <= 'Z')
	{
		return true;
	}

	return false;
}

bool isNumber(const char c)
{
	if (c >= '0' && c <= '9')
	{
		return true;
	}

	return false;
}

bool isNumber(const std::string &ref)
{
	try
	{
		stod(ref);
	}
	catch (std::invalid_argument&)
	{
		return false;
	}
	return true;
}

double acosR(double product)
{
	int sign = (product > 0) ? 1 : -1;
	product = (abs(product) > 1.0f) ? sign : product;
	return acos(product);
}

double asinR(double product)
{
	int sign = (product > 0) ? 1 : -1;
	product = (abs(product) > 1.0f) ? sign : product;
	return asin(product);
}
