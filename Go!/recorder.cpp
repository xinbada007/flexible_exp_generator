#include "stdafx.h"
#include "recorder.h"
#include "collVisitor.h"
#include "edge.h"
#include "halfedge.h"
#include "points.h"
#include "readConfig.h"

#include <stdlib.h>
#include <fstream>
#include <atlstr.h>

#include <osg/Notify>

using namespace std;

Recorder::Recorder() :_statusText(new osgText::Text),
_lastFrameStamp(0), _lastTimeReference(0.0f)
{
	_outMoment.push_back(&_recS._time);
	_outMoment.push_back(&_recS._fps);
	_outMoment.push_back(&_recS._frame);
	_outMoment.push_back(&_recS._crash);
	_outMoment.push_back(&_recS._rb);
	_outMoment.push_back(&_recS._ru);
	_outMoment.push_back(&_recS._lu);
	_outMoment.push_back(&_recS._lb);
	_outMoment.push_back(&_recS._oc);
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
	_outMoment.push_back(&_recS._AHA);
	_outMoment.push_back(&_recS._speed);

	_outMoment.push_back(&_recS._PERIOD);

	copy();
}

Recorder::~Recorder()
{
	std::cout << "Deconstruct Recorder" << std::endl;
}

bool Recorder::output(ReadConfig *rc)
{
	const std::string filename = rc->getSubjects()->getRecPath() + "\\rec.txt";
	ofstream wout(filename);
	if (!wout)
	{
		osg::notify(osg::FATAL) << "cannot create recorder file" << std::endl;
		return false;
	}

	wout << _txtRecorder << std::endl;

	//save state
	{
		if (!_saveState.empty())
		{
			const std::string filename = rc->getSubjects()->getRecPath() + "\\savestate.txt";
			ofstream wout(filename);
			if (!wout)
			{
				osg::notify(osg::FATAL) << "cannot create savestate file" << std::endl;
				return false;
			}

			wout << _saveState << std::endl;
		}
	}

	return true;
}

void Recorder::rectoTxt(const CarState *carState)
{
	char temp[10];
	const int size_temp = sizeof(temp);

	const unsigned frameStamp = carState->_frameStamp;
	_itoa_s(frameStamp, temp, size_temp);
	_recS._frame = temp + _recS._TAB;

	const unsigned nDigit(6);
	char tempd[20];
	const int size_tempd = sizeof(tempd);

	const double timeReference = carState->_timeReference;
	_gcvt_s(tempd, size_tempd, timeReference, nDigit);
	_recS._time = tempd + _recS._TAB;

	const double fps = (frameStamp - _lastFrameStamp) / (timeReference - _lastTimeReference);
	_gcvt_s(tempd, size_tempd, fps, nDigit);
	_recS._fps = tempd + _recS._TAB;
	_lastFrameStamp = frameStamp;
	_lastTimeReference = timeReference;

	unsigned crash = carState->_collide;
	_itoa_s(crash, temp,size_temp);
	_recS._crash = temp + _recS._TAB;
	
	if (carState->_currentQuad.size() < carState->_carArray->size())
	{
		osg::notify(osg::FATAL) << "Cannot record" << std::endl;
		return;
	}
	quadList::const_iterator i = carState->_currentQuad.cbegin();
	int rb = (*i) ? (*i)->getIndex() : -1; i++;
	int ru = (*i) ? (*i)->getIndex() : -1; i++;
	int lu = (*i) ? (*i)->getIndex() : -1; i++;
	int lb = (*i) ? (*i)->getIndex() : -1; i++;
	int oc = (*i) ? (*i)->getIndex() : -1;
	_itoa_s(rb, temp, size_temp);
	_recS._rb = temp + _recS._TAB;
	_itoa_s(ru, temp, size_temp);
	_recS._ru = temp + _recS._TAB;
	_itoa_s(lu, temp, size_temp);
	_recS._lu = temp + _recS._TAB;
	_itoa_s(lb, temp, size_temp);
	_recS._lb = temp + _recS._TAB;
	_itoa_s(oc, temp, size_temp);
	_recS._oc = temp + _recS._TAB;
	
	double swAngle = carState->_swangle;
	_gcvt_s(tempd, size_tempd, swAngle, nDigit);
	_recS._swAngle = tempd + _recS._TAB;

	double OX = carState->_O.x();
	double OY = carState->_O.y();
	double OZ = carState->_O.z();
	_gcvt_s(tempd, size_tempd, OX, nDigit);
	_recS._OX = tempd + _recS._TAB;
	_gcvt_s(tempd, size_tempd, OY, nDigit);
	_recS._OY = tempd + _recS._TAB;
	_gcvt_s(tempd, size_tempd, OZ, nDigit);
	_recS._OZ = tempd + _recS._TAB;

	double HX = carState->_direction.x();
	double HY = carState->_direction.y();
	double HZ = carState->_direction.z();
	_gcvt_s(tempd, size_tempd, HX, nDigit);
	_recS._HX = tempd + _recS._TAB;
	_gcvt_s(tempd, size_tempd, HY, nDigit);
	_recS._HY = tempd + _recS._TAB;
	_gcvt_s(tempd, size_tempd, HZ, nDigit);
	_recS._HZ = tempd + _recS._TAB;

	double speed = carState->getSpeed();
	_gcvt_s(tempd, size_tempd, speed, nDigit);
	_recS._speed = tempd + _recS._TAB;

	osg::Vec3d carD = carState->_direction;
	carD.normalize();
	if (!carState->_lastQuad.empty())
	{
		osg::ref_ptr<osg::Vec3dArray> navigationEdge = carState->_lastQuad.back()->getLoop()->getNavigationEdge();
		osg::Vec3d naviEdge = navigationEdge->front() - navigationEdge->back();
		naviEdge.normalize();
		double dA = (acosR(carD*naviEdge) / TO_RADDIAN);
		_gcvt_s(tempd, size_tempd, dA, nDigit);
		_recS._dAngle = tempd + _recS._TAB;
	}

	osg::Vec3d carD_LastFrame = carState->_directionLastFrame;
	carD_LastFrame.normalize();
	const double AHA = (asinR((carD ^ carD_LastFrame).z()) / TO_RADDIAN);
	const double HA = AHA * frameRate;
	_gcvt_s(tempd, size_tempd, HA, nDigit);
	_recS._HA = tempd + _recS._TAB;

	_recS._accumulativeHeading += AHA;
	_gcvt_s(tempd, size_tempd, _recS._accumulativeHeading, nDigit);
	_recS._AHA = tempd + _recS._TAB;

	const osg::Vec3d O = carState->_O;
	const osg::Vec3d N = carState->_O_Project;
	const double dis = (N - O).length();
	_gcvt_s(tempd, size_tempd, dis, nDigit);
	_recS._dither = tempd + _recS._TAB;
}

void Recorder::copy()
{
	std::vector<const std::string*>::const_iterator i = _outMoment.cbegin();
	std::string content;
	while (i != _outMoment.cend())
	{
		_txtRecorder += **i;
		if (isNumber(**i))
		{
			float number = (stof(**i));
			int i_number = number;
			char temp[20];
			unsigned temp_size = sizeof(temp);
			if (!(number - i_number))
			{
				sprintf_s(temp, temp_size, "%d", i_number);
			}
			else
			{
				sprintf_s(temp, temp_size, "%.2f", number);
			}
			content += temp;
			content.push_back('\t');
		}
		else
		{
			content += **i;
		}
		i++;
	}

	setStatus(content);
}

void Recorder::setStatus(const std::string &content)
{
	const unsigned TIME(0), FPS(1), FRAME(2), CRASH(3);
	const unsigned RB(4), RU(5), LU(6), LB(7), OC(8);
	const unsigned DITHER(9), DANGLE(10), SWANGLE(11);
	const unsigned OX(12), OY(13), OZ(14);
	const unsigned HX(15), HY(16), HZ(17);
	const unsigned HA(18), SPEED(20);

	std::string text;
	std::string::const_iterator iter = content.cbegin();
	unsigned numTab(0);
	while (iter != content.cend())
	{
		if (*iter != '\t')
		{
			text.push_back(*iter);
		}
		else
		{
			numTab++;
			switch (numTab)
			{
			case 3:
				text.push_back(' ');
				text.push_back(' ');
				text += "Crash: ";
				break;
			case 4:
				text.push_back('\n');
				text += "RB RU LU LB OC: ";
				break;
			case 9:
				text.push_back('\n');
				text += "Dither: ";
				break;
			case 10:
				text.push_back(' ');
				text.push_back(' ');
				text += "DAngle: ";
				break;
			case 11:
				text.push_back(' ');
				text.push_back(' ');
				text += "S\\W: ";
				break;
			case 12:
				text.push_back('\n');
				text += "Original: ";
				break;
			case 15:
				text.push_back('\n');
				text += "Heading: ";
				break;
			case 18:
				text.push_back('\n');
				text += "Wheel Angle: ";
				break;
			case 19:
				text.push_back(' ');
				text.push_back(' ');
				text += "Accumulative Heading: ";
				break;
			case 20:
				text.push_back(' ');
				text.push_back(' ');
				text += "SPEED: ";
				break;
			default:
				text.push_back(' ');
				text.push_back(' ');
				break;
			}
		}
		iter++;
	}
	_statusText->setText(text);
	_statusText->update();
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

	//enable savestate
	if (carState)
	{
		if (carState->_frameStamp > 1)
		{
			const osg::Matrixd &m = carState->_state;
			char temp[10];
			const unsigned temp_size = sizeof(temp);
			_itoa_s(carState->_frameStamp, temp, temp_size);
			_saveState += temp;
			_saveState.push_back('\n');
			//m is 4*4
			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 4; j++)
				{
					double number = m(i, j);
					char temp[20];
					const unsigned temp_size = sizeof(temp);
					const unsigned nDigit(6);
					_gcvt_s(temp, temp_size, number, nDigit);
					_saveState += temp;
					_saveState.push_back('\t');
				}
				_saveState.push_back('\n');
			}
			_saveState.push_back('\n');
		}
	}

	traverse(node, nv);
}
