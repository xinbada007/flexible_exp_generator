#pragma once
#include <osg/NodeCallback>

#include <vector>
#include <string>

#include "car.h"

class ReadConfig;

typedef struct recState
{
	recState::recState() :
		_TAB("\t"), _PERIOD("\n"), _time("Time"), _fps("FPS"), _frame("Frame"), _crash("Crash"),
		_rb("RB"), _ru("RU"), _lu("LU"), _lb("LB"), _dither("Dither"), _dAngle("DAngle"),
		_swAngle("swAngle"), _OX("OX"), _OY("OY"), _OZ("OZ"), _HX("HX"), _HY("HY"),
		_HZ("HZ"), _HA("HA"), _speed("Speed")
	{
		_time += _TAB; _fps += _TAB; _frame += _TAB; _crash += _TAB;
		_rb += _TAB; _ru += _TAB; _lu += _TAB; _lb += _TAB; _dither += _TAB;
		_dAngle += _TAB; _swAngle += _TAB; _OX += _TAB; _OY += _TAB; _OZ += _TAB;
		_HX += _TAB; _HY += _TAB; _HZ += _TAB; _HA += _TAB;

	}
	std::string _time;
	std::string _fps;
	std::string _frame;
	std::string _crash;
	std::string _rb;
	std::string _ru;
	std::string _lu;
	std::string _lb;
	std::string _dither;
	std::string _dAngle;
	std::string _swAngle;
	std::string _OX;
	std::string _OY;
	std::string _OZ;
	std::string _HX;
	std::string _HY;
	std::string _HZ;
	std::string _HA;
	std::string _speed;

	std::string _PERIOD;
	std::string _TAB;
}recState;

class Recorder :
	public osg::NodeCallback
{
public:
	Recorder();
	void operator()(osg::Node *node, osg::NodeVisitor *nv);
	void rectoTxt(const CarState *carState);
	virtual ~Recorder();

	bool output(ReadConfig *rc);

private:
	void copy();
	std::string _txtRecorder;
	std::vector<const std::string*> _outMoment;

	recState _recS;
};