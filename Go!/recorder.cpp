#include "stdafx.h"
#include "recorder.h"
#include "collVisitor.h"
#include "edge.h"
#include "halfedge.h"
#include "points.h"

#include <stdlib.h>

#include <osg/Notify>

using namespace std;

Recorder::Recorder()
{
	_outMoment.push_back(&_recS._time);
	_outMoment.push_back(&_recS._fps);
	_outMoment.push_back(&_recS._frame);
	_outMoment.push_back(&_recS._crash);
	_outMoment.push_back(&_recS._rb);
	_outMoment.push_back(&_recS._ru);
	_outMoment.push_back(&_recS._lu);
	_outMoment.push_back(&_recS._lb);
	_outMoment.push_back(&_recS._dither);
	_outMoment.push_back(&_recS._dAngle);
	_outMoment.push_back(&_recS._swAngle);
	_outMoment.push_back(&_recS._OX);
	_outMoment.push_back(&_recS._OY);
	_outMoment.push_back(&_recS._OZ);
	_outMoment.push_back(&_recS._HX);
	_outMoment.push_back(&_recS._HY);
	_outMoment.push_back(&_recS._HZ);
	_outMoment.push_back(&_recS._HA);
	_outMoment.push_back(&_recS._speed);

	_outMoment.push_back(&_recS._PERIOD);

	copy();
}



Recorder::~Recorder()
{
}

void Recorder::rectoTxt(const CarState *carState)
{
	char temp[10];
	const int size_temp = sizeof(temp);

	unsigned crash = carState->_collide;
	_itoa_s(crash, temp,size_temp);
	_recS._crash = temp;
	
	if (carState->_lastQuad.size() < carState->_carArray->size())
	{
		osg::notify(osg::FATAL) << "Cannot record" << std::endl;
		return;
	}
	quadList::const_iterator i = carState->_currentQuad.cbegin();
	int rb = (*i) ? (*i)->getIndex() : -1; i++;
	int ru = (*i) ? (*i)->getIndex() : -1; i++;
	int lu = (*i) ? (*i)->getIndex() : -1; i++;
	int lb = (*i) ? (*i)->getIndex() : -1;
	_itoa_s(rb, temp, size_temp);
	_recS._rb = temp;
	_itoa_s(ru, temp, size_temp);
	_recS._ru = temp;
	_itoa_s(lu, temp, size_temp);
	_recS._lu = temp;
	_itoa_s(lb, temp, size_temp);
	_recS._lb = temp;
	
	const unsigned nDigit(6);
	char tempd[20];
	const int size_tempd = sizeof(tempd);
	double swAngle = carState->_swangle;
	_gcvt_s(tempd, size_tempd, swAngle, nDigit);
	_recS._swAngle = tempd;

	double OX = carState->_O.x();
	double OY = carState->_O.y();
	double OZ = carState->_O.z();
	_gcvt_s(tempd, size_tempd, OX, nDigit);
	_recS._OX = tempd;
	_gcvt_s(tempd, size_tempd, OY, nDigit);
	_recS._OY = tempd;
	_gcvt_s(tempd, size_tempd, OZ, nDigit);
	_recS._OZ = tempd;

	double HX = carState->_direction.x();
	double HY = carState->_direction.y();
	double HZ = carState->_direction.z();
	_gcvt_s(tempd, size_tempd, HX, nDigit);
	_recS._HX = tempd;
	_gcvt_s(tempd, size_tempd, HY, nDigit);
	_recS._HY = tempd;
	_gcvt_s(tempd, size_tempd, HZ, nDigit);
	_recS._HZ = tempd;

	double speed = carState->_speed;
	_gcvt_s(tempd, size_tempd, speed, nDigit);
	_recS._speed = tempd;

	osg::Vec3 carD = carState->_direction;
	carD.normalize();
	if (!carState->_lastQuad.empty())
	{
		osg::ref_ptr<osg::Vec3Array> navigationEdge = carState->_lastQuad.back()->getLoop()->getNavigationEdge();
		osg::Vec3 naviEdge = navigationEdge->front() - navigationEdge->back();
		naviEdge.normalize();
		const double dA = acos(carD*naviEdge);
		_gcvt_s(tempd, size_tempd, dA, nDigit);
		_recS._dAngle = tempd;
	}

	osg::Vec3 carD_LastFrame = carState->_directionLastFrame;
	carD_LastFrame.normalize();
	const double HA = asin((carD ^ carD_LastFrame).z());
	_gcvt_s(tempd, size_tempd, HA, nDigit);
	_recS._HA = tempd;

	const osg::Vec3 O = carState->_O;
	const osg::Vec3 N = carState->_O_Project;
	const double dis = (N - O).length();
	_gcvt_s(tempd, size_tempd, dis, nDigit);
	_recS._dither = tempd;
}

void Recorder::copy()
{
	std::vector<const std::string*>::const_iterator i = _outMoment.cbegin();
	while (i != _outMoment.cend())
	{
		_txtRecorder += **i;
		i++;
	}
}

void Recorder::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
	const Car *refC = dynamic_cast<Car*>(node);
	const CarState *carState(NULL);
	if (refC)
	{
		carState = refC->getCarState();
	}

	if (carState)
	{
		rectoTxt(carState);

		copy();
	}

	traverse(node, nv);
}
