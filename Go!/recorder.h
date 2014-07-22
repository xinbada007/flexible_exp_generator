#pragma once
#include <osg/NodeCallback>
#include <osgText/Text>

#include <vector>
#include <string>

#include "car.h"

class ReadConfig;

typedef struct recState
{
	recState::recState() :
		_TAB("\t"), _PERIOD("\n"), _time("Time"), _fps("FPS"), _frame("Frame"), _crash("Crash"),
		_rb("RB"), _ru("RU"), _lu("LU"), _lb("LB"), _oc("OC"), _dither("Dither"), _dAngle("DAngle"),
		_swAngle("swAngle"), _OX("OX"), _OY("OY"), _OZ("OZ"), _HX("HX"), _HY("HY"),
		_HZ("HZ"), _DX("DX"), _DY("DY"), _DZ("DZ"), _HA("HA"), _RHA("RHA"), _AHA("AccumulativeHeading(Y)"),
		_speed("Speed"), _Rspeed("RSpeed"), _dynamic("Dynamic"), _usrHit(""), _replay(""), _accumulativeHeading(0.0f)
	{
		_time += _TAB; _fps += _TAB; _frame += _TAB; _crash += _TAB;
		_rb += _TAB; _ru += _TAB; _lu += _TAB; _lb += _TAB; _oc += _TAB; _dither += _TAB;
		_dAngle += _TAB; _swAngle += _TAB; _OX += _TAB; _OY += _TAB; _OZ += _TAB;
		_HX += _TAB; _HY += _TAB; _HZ += _TAB; _DX += _TAB; _DY += _TAB; _DZ += _TAB;
		_HA += _TAB; _RHA += _TAB;  _AHA += _TAB; _speed += _TAB; _Rspeed += _TAB;
		_dynamic += _TAB; _usrHit += _TAB;
	}
	std::string _time;
	std::string _fps;
	std::string _frame;
	std::string _crash;
	std::string _rb;
	std::string _ru;
	std::string _lu;
	std::string _lb;
	std::string _oc;
	std::string _dither;
	std::string _dAngle;
	std::string _swAngle;
	std::string _OX;
	std::string _OY;
	std::string _OZ;
	std::string _HX;
	std::string _HY;
	std::string _HZ;
	std::string _DX;
	std::string _DY;
	std::string _DZ;
	std::string _HA;
	std::string _RHA;
	std::string _AHA;
	std::string _speed;
	std::string _Rspeed;
	std::string _dynamic;
	std::string _usrHit;
	std::string _replay;

	std::string _PERIOD;
	std::string _TAB;

	double _accumulativeHeading;
}recState;

class Recorder :
	public osg::NodeCallback
{
public:
	Recorder();
	virtual ~Recorder();

	void operator()(osg::Node *node, osg::NodeVisitor *nv);
	bool output(ReadConfig *rc);
	osgText::Text * getStatus() const { return _statusText.get(); };

private:
	void rectoTxt(const CarState *carState);
	void copyandSetHUDText();
	void setHUDText();
	void setStatus(const std::string &txt);

	std::string _txtRecorder;
	std::string _saveState;
	std::vector<const std::string*> _outMoment;
	osg::ref_ptr<osgText::Text> _statusText;
	recState _recS;

	unsigned _lastFrameStamp;
	double _lastTimeReference;
};