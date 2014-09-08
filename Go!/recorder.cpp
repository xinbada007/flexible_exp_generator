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
_lastFrameStamp(0), _lastTimeReference(0.0f), _saveState("TrialReplay\n"), _cameraHUD(NULL)
, _geodeHUD(new osg::Geode), _detailed(false)
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
	_outMoment.push_back(&_recS._customDither);
	_outMoment.push_back(&_recS._dAngle);
	_outMoment.push_back(&_recS._swAngle);
	_outMoment.push_back(&_recS._OX);
	_outMoment.push_back(&_recS._OY);
	_outMoment.push_back(&_recS._OZ);
	_outMoment.push_back(&_recS._HX);
	_outMoment.push_back(&_recS._HY);
	_outMoment.push_back(&_recS._HZ);
	_outMoment.push_back(&_recS._DX);
	_outMoment.push_back(&_recS._DY);
	_outMoment.push_back(&_recS._DZ);
	_outMoment.push_back(&_recS._HA);
	_outMoment.push_back(&_recS._RHA);
	_outMoment.push_back(&_recS._AHA);
	_outMoment.push_back(&_recS._speed);
	_outMoment.push_back(&_recS._Rspeed);
	_outMoment.push_back(&_recS._dynamic);
	_outMoment.push_back(&_recS._usrHit);
	_outMoment.push_back(&_recS._replay);

	_outMoment.push_back(&_recS._PERIOD);

	copyandSetHUDText();

	_geodeHUD->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	_geodeHUD->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
}

Recorder::~Recorder()
{
	std::cout << "Deconstruct Recorder" << std::endl;
}

bool Recorder::output(ReadConfig *rc)
{	
	if (rc->isReplay())
	{
		return false;
	}

	const std::string filename = rc->getSubjects()->getRecPath() + "\\rec.txt";
	ofstream wout(filename);
	if (!wout)
	{
		osg::notify(osg::FATAL) << "cannot create recorder file" << std::endl;
	}
	else
	{
		wout << _txtRecorder << std::endl;
	}

	//save state
	{
		if (!_saveState.empty())
		{
			const std::string filename = rc->getSubjects()->getRecPath() + "\\savestate.txt";
			ofstream wout(filename);
			if (!wout)
			{
				osg::notify(osg::FATAL) << "cannot create savestate file" << std::endl;
			}
			else
			{
				wout << _saveState << std::endl;
			}
		}
	}

	//save all the points that comprise the road
	{
		nurbsList nurbs = rc->getRoadSet()->_nurbs;
		if (!nurbs.empty())
		{
			const std::string filename = rc->getSubjects()->getRecPath() + "\\roads.txt";
			const std::string filename_left = rc->getSubjects()->getRecPath() + "\\roadsLeft.txt";
			const std::string filename_right = rc->getSubjects()->getRecPath() + "\\roadsRight.txt";
			ofstream wout(filename);
			ofstream wout_left(filename_left);
			ofstream wout_right(filename_right);
			if (!wout || !wout_left || !wout_right)
			{
				osg::notify(osg::FATAL) << "cannot create roads file" << std::endl;
			}
			else
			{
				string roads,roads_left,roads_right;
				char tempd[20];
				const unsigned size_tempd(sizeof(tempd));
				const unsigned numDigit(6);
				nurbsList::const_iterator i = nurbs.cbegin();
				while (i != nurbs.cend())
				{
					osg::Vec3dArray::const_iterator j = (*i)->_path->begin();
					osg::Vec3dArray::const_iterator j_left = (*i)->_path_left->begin();
					osg::Vec3dArray::const_iterator j_right = (*i)->_path_right->begin();
					while (j != (*i)->_path->end())
					{
						_gcvt_s(tempd, size_tempd, (*j).x(), numDigit);
						roads += tempd;
						roads += "\t";
						_gcvt_s(tempd, size_tempd, (*j).y(), numDigit);
						roads += tempd;
						roads += "\t";
						_gcvt_s(tempd, size_tempd, (*j).z(), numDigit);
						roads += tempd;
						roads += "\n";
						j++;

						if (j_left != (*i)->_path_left->end())
						{
							_gcvt_s(tempd, size_tempd, (*j_left).x(), numDigit);
							roads_left += tempd;
							roads_left += "\t";
							_gcvt_s(tempd, size_tempd, (*j_left).y(), numDigit);
							roads_left += tempd;
							roads_left += "\t";
							_gcvt_s(tempd, size_tempd, (*j_left).z(), numDigit);
							roads_left += tempd;
							roads_left += "\n";
							j_left++;
						}

						if (j_right != (*i)->_path_right->end())
						{
							_gcvt_s(tempd, size_tempd, (*j_right).x(), numDigit);
							roads_right += tempd;
							roads_right += "\t";
							_gcvt_s(tempd, size_tempd, (*j_right).y(), numDigit);
							roads_right += tempd;
							roads_right += "\t";
							_gcvt_s(tempd, size_tempd, (*j_right).z(), numDigit);
							roads_right += tempd;
							roads_right += "\n";
							j_right++;
						}
					}
					i++;
				}
				wout << roads << std::endl;
				wout_left << roads_left << std::endl;
				wout_right << roads_right << std::endl;
			}
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
	const double timePeriod = timeReference - _lastTimeReference;
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
	int rb(-1), ru(-1), lu(-1), lb(-1), oc(-1);
	int srb(-1), sru(-1), slu(-1), slb(-1), soc(-1);
	if (*i)
	{
		rb = (*i)->getIndex();
		srb = (*i)->getHomeS()->getIndex();
	}
	i++;
	if (*i)
	{
		ru = (*i)->getIndex();
		sru = (*i)->getHomeS()->getIndex();
	}
	i++;
	if (*i)
	{
		lu = (*i)->getIndex();
		slu = (*i)->getHomeS()->getIndex();
	}
	i++;
	if (*i)
	{
		lb = (*i)->getIndex();
		slb = (*i)->getHomeS()->getIndex();
	}
	i++;
	if (*i)
	{
		oc = (*i)->getIndex();
		soc = (*i)->getHomeS()->getIndex();
	}
	_itoa_s(srb, temp, size_temp);
	_recS._rb = "("; _recS._rb += temp; _recS._rb += ")";
	_itoa_s(rb, temp, size_temp);
	_recS._rb += temp + _recS._TAB;
	
	_itoa_s(sru,temp,size_temp);
	_recS._ru = "("; _recS._ru += temp; _recS._ru += ")";
	_itoa_s(ru, temp, size_temp);
	_recS._ru += temp + _recS._TAB;

	_itoa_s(slu, temp, size_temp);
	_recS._lu = "("; _recS._lu += temp; _recS._lu += ")";
	_itoa_s(lu, temp, size_temp);
	_recS._lu += temp + _recS._TAB;
	
	_itoa_s(slb, temp, size_temp);
	_recS._lb = "("; _recS._lb += temp; _recS._lb += ")";
	_itoa_s(lb, temp, size_temp);
	_recS._lb += temp + _recS._TAB;
	
	_itoa_s(soc, temp, size_temp);
	_recS._oc = "("; _recS._oc += temp; _recS._oc += ")";
	_itoa_s(oc, temp, size_temp);
	_recS._oc += temp + _recS._TAB;

	const int dynamic = (carState->_dynamic) ? 1 : 0;
	_itoa_s(dynamic, temp, size_temp);
	_recS._dynamic = temp + _recS._TAB;

	const int usrHit = carState->_userHit;
	_itoa_s(usrHit, temp, size_temp);
	_recS._usrHit = temp + _recS._TAB;
	
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

	double HX = carState->_heading.x();
	double HY = carState->_heading.y();
	double HZ = carState->_heading.z();
	_gcvt_s(tempd, size_tempd, HX, nDigit);
	_recS._HX = tempd + _recS._TAB;
	_gcvt_s(tempd, size_tempd, HY, nDigit);
	_recS._HY = tempd + _recS._TAB;
	_gcvt_s(tempd, size_tempd, HZ, nDigit);
	_recS._HZ = tempd + _recS._TAB;

	double DX = carState->_direction.x();
	double DY = carState->_direction.y();
	double DZ = carState->_direction.z();
	_gcvt_s(tempd, size_tempd, DX, nDigit);
	_recS._DX = tempd + _recS._TAB;
	_gcvt_s(tempd, size_tempd, DY, nDigit);
	_recS._DY = tempd + _recS._TAB;
	_gcvt_s(tempd, size_tempd, DZ, nDigit);
	_recS._DZ = tempd + _recS._TAB;

	carState->cacluateSpeedandAngle();

	double speed = carState->getSpeed();
	_gcvt_s(tempd, size_tempd, speed, nDigit);
	_recS._speed = tempd + _recS._TAB;

	double Rspeed = carState->getRSpeed();
	_gcvt_s(tempd, size_tempd, Rspeed, nDigit);
	_recS._Rspeed = tempd + _recS._TAB;

	osg::Vec3d carD = carState->_direction;
	carD.normalize();
	if (!carState->_lastQuad.empty())
	{
		osg::ref_ptr<osg::Vec3dArray> navigationEdge = carState->_lastQuad.back()->getLoop()->getNavigationEdge();
		osg::Vec3d naviEdge = navigationEdge->front() - navigationEdge->back();
		naviEdge.normalize();
		const osg::Vec3d cross = naviEdge^carD;
		double dA = (asinR(cross.z()) / TO_RADDIAN);
		_gcvt_s(tempd, size_tempd, dA, nDigit);
		_recS._dAngle = tempd + _recS._TAB;
	}

	osg::Vec3d carD_LastFrame = carState->_directionLastFrame;
	carD_LastFrame.normalize();
	const double AHA = (asinR((carD ^ carD_LastFrame).z()) / TO_RADDIAN);
// 	const double HA = AHA * frameRate;
// 	_gcvt_s(tempd, size_tempd, HA, nDigit);
// 	_recS._HA = tempd + _recS._TAB;
	const double HA = carState->_angle / TO_RADDIAN;
	_gcvt_s(tempd, size_tempd, HA, nDigit);
	_recS._HA = tempd + _recS._TAB;

	const double RHA = (!timePeriod) ? AHA * frameRate : AHA / timePeriod;
	_gcvt_s(tempd, size_tempd, RHA, nDigit);
	_recS._RHA = tempd + _recS._TAB;

	_recS._accumulativeHeading += AHA;
	_gcvt_s(tempd, size_tempd, _recS._accumulativeHeading, nDigit);
	_recS._AHA = tempd + _recS._TAB;

// 	const osg::Vec3d O = carState->_O;
// 	const osg::Vec3d N = carState->_O_Project;
//	const double dis = (N - O).length();
	const double dis = carState->_dither;

	_gcvt_s(tempd, size_tempd, dis, nDigit);
	_recS._dither = tempd + _recS._TAB;

	const double customD = carState->_distancefromBase;
	_gcvt_s(tempd, size_tempd, customD, nDigit);
	_recS._customDither = tempd + _recS._TAB;

	_detailed = carState->_detailedDisplay;

	if (carState->_replay)
	{
		_recS._replay = "\n" + carState->getReplayText();
	}
}

void Recorder::copyandSetHUDText()
{
	std::vector<const std::string*>::const_iterator i = _outMoment.cbegin();
	std::string content;
	std::string lesscontent;
	while (i != _outMoment.cend())
	{
		const unsigned seq(i - _outMoment.cbegin());
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
			switch (seq)
			{
			case Recorder::TypeofText::TIME:
				lesscontent += temp;
				lesscontent.push_back('\t');
				break;
			case Recorder::TypeofText::CUSTOMD:
				lesscontent += temp;
				lesscontent.push_back('\t');
				break;
			case Recorder::TypeofText::SPEED:
				lesscontent += temp;
				lesscontent.push_back('\t');
				break;
// 			case Recorder::TypeofText::USRHIT:
// 				lesscontent += temp;
// 				lesscontent.push_back('\t');
// 				break;
			default:
				break;
			}
		}
		else
		{
			content += **i;
		}
		i++;
	}

	if (_detailed)
	{
		setStatus(content);
	}
	else
	{
		setStatusLess(lesscontent);
	}
}

void Recorder::setHUDText()
{
	std::vector<const std::string*>::const_iterator i = _outMoment.cbegin();
	std::string content;

	while (i != _outMoment.cend())
	{
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

void Recorder::setStatusLess(const std::string &txt)
{
	if (txt.empty())
	{
		return;
	}

	//1. Time
	std::string::const_iterator i = txt.cbegin();
	std::string text;
	text += "TIME:  ";
	while (*i != '\t' && i != txt.cend())
	{
		text.push_back(*i);
		i++;
	}
	(*i == '\t') ? ++i : i;

	//2. Deviation
	text += (i == txt.cend()) ? "" : "\n\nDEVIATION:  ";
	while (*i != '\t' && i != txt.cend())
	{
		text.push_back(*i);
		i++;
	}
	(*i == '\t') ? ++i : i;

	//3. Speed
	text += (i == txt.cend()) ? "" : "\n\nSPEED:  ";
	while (*i != '\t' && i != txt.cend())
	{
		text.push_back(*i);
		i++;
	}
	(*i == '\t') ? ++i : i;

	//4. UserHit
	text += (i == txt.cend()) ? "" : "\n\nREC.:  ";
	char rec('0');
	if (*i != '\t' && i != txt.cend())
	{
		if (*i == '0')
		{
			text += "Recording...Release to Stop...";
		}
		else
		{
			text += "Hold Left Turn Light to Record";
		}
	}

	_statusText->setText(text);
	_statusText->update();
}

void Recorder::setStatus(const std::string &content)
{
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
			case Recorder::TypeofText::CRASH:
				text.push_back(' ');
				text.push_back(' ');
				text += "Crash: ";
				break;
			case Recorder::TypeofText::RB:
				text.push_back('\n');
				text += "RB RU LU LB OC: ";
				break;
			case Recorder::TypeofText::DITHER:
				text.push_back('\n');
				text += "Dither: ";
				break;
			case Recorder::TypeofText::DANGLE:
				text.push_back(' ');
				text.push_back(' ');
				text += "DAngle: ";
				break;
			case Recorder::TypeofText::SWANGLE:
				text.push_back(' ');
				text.push_back(' ');
				text += "S\\W: ";
				break;
			case Recorder::TypeofText::OX:
				text.push_back('\n');
				text += "Original: ";
				break;
			case Recorder::TypeofText::HX:
				text.push_back('\n');
				text += "Heading: ";
				break;
			case Recorder::TypeofText::DX:
				text.push_back(' ');
				text.push_back(' ');
				text += "Direction: ";
				break;
			case Recorder::TypeofText::HA:
				text.push_back('\n');
				text += "Wheel Angle: ";
				break;
			case Recorder::TypeofText::AHA:
				text.push_back(' ');
				text.push_back(' ');
				text += "Accu. Heading: ";
				break;
			case Recorder::TypeofText::SPEED:
				text.push_back(' ');
				text.push_back(' ');
				text += "SPEED: ";
				break;
			case Recorder::TypeofText::DYNAMIC:
				text.push_back('\n');
				text += "Dynamic: ";
				break;
			case Recorder::TypeofText::USRHIT:
				text.push_back(' ');
				text.push_back(' ');
				text += "Hit: ";
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

		if (carState->_replay)
		{
			setHUDText();
		}
		else
		{
			copyandSetHUDText();
		}
	}

	//enable savestate
	//last to apply
	if (carState && !carState->_saveState)
	{
		if (carState->_frameStamp > 1)
		{
			const osg::Matrixd &m = carState->_moment;
			char temp[10];
			const unsigned temp_size = sizeof(temp);
			_itoa_s(carState->_frameStamp, temp, temp_size);
			_saveState += "\nFrame:\t";
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
			_saveState += "Dynamic:\t" + _recS._dynamic;
			_saveState.push_back('\n');
		}
	}

	traverse(node, nv);
}

void Recorder::setHUDCamera(osg::Camera *cam)
{
	if (!cam)
	{
		return;
	}

	_cameraHUD = cam;

	const double X = _cameraHUD->getViewport()->width();
	const double Y = _cameraHUD->getViewport()->height();

	osg::Vec3d position(0.05*X, 0.75*Y, 0.0f);
	std::string font("fonts/arial.ttf");
	_statusText->setFont(font);
	_statusText->setCharacterSize(28);
	_statusText->setFontResolution(28, 28);
	_statusText->setAlignment(osgText::TextBase::LEFT_CENTER);
	_statusText->setPosition(position);
	_statusText->setDataVariance(osg::Object::DYNAMIC);

	_geodeHUD->addDrawable(_statusText);
	_cameraHUD->addChild(_geodeHUD);
}