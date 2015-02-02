#include "stdafx.h"
#include "carReplay.h"

#include <osg/Notify>
#include <osgGA/EventVisitor>

CarReplay::CarReplay():
_frame(0), _pause(true)
{
}

CarReplay::~CarReplay()
{
	_mTransform = NULL;
	_carState = NULL;
}

void CarReplay::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
	osg::MatrixTransform *refM = dynamic_cast<osg::MatrixTransform*> (node);
	Car *refC = (refM) ? dynamic_cast<Car*>(refM->getUserData()) : NULL;

	osgGA::EventVisitor *ev = dynamic_cast<osgGA::EventVisitor*>(nv);
	osgGA::EventQueue::Events events = (ev) ? ev->getEvents() : events;
	osgGA::GUIEventAdapter *ea = (!events.empty()) ? events.front() : NULL;

	if (refC && refC->getCarState() && refC->getCarState()->_saveState && ea)
	{
		_mTransform = refM;
		_carState = refC->getCarState();
		osg::MatrixdArray *marray = _carState->_saveState;
		if (_frame != marray->size() - 1)
		{
			_carState->_replay = true;
			switch (ea->getEventType())
			{
			case osgGA::GUIEventAdapter::KEYDOWN:
				switch (ea->getKey())
				{
				case osgGA::GUIEventAdapter::KEY_Space:
					_pause = !_pause;
					_carState->setReplayText("Replay Paused");
					break;
				default:
					break;
				}
			case osgGA::GUIEventAdapter::FRAME:
				_carState->_directionLastFrame = _carState->_direction;
				if (!_pause)
				{
					_carState->setReplayText("Replay Running");
					if (_carState->_dynamicState->size() == marray->size())
					{
						_carState->_dynamic = ((*_carState->_dynamicState)[_frame] == 1);
					}
					applyCarMovement(*(marray->begin() + _frame));
					_frame++;
				}
				break;
			}
		}
		else
		{
			_carState->setReplayText("Replay Ended");
		}
	}

	traverse(node, nv);
}

void CarReplay::applyCarMovement(osg::Matrixd &m)
{
	//cacluate direction
	_carState->_direction =	_carState->_direction = _carState->_direction * osg::Matrix::rotate(m.getRotate());
	_carState->_direction.normalize();
	//calculate speed
	osg::Vec3d curO = _carState->_O;

	//apply matrix
	osg::Matrixd &_moment = m;
	_carState->_state *= _moment;
	_carState->_moment = _moment;
	_carState->_O = _carState->_O * _moment;
	arrayByMatrix(_carState->_backWheel, _moment);
	arrayByMatrix(_carState->_frontWheel, _moment);
	arrayByMatrix(_carState->_carArray, _moment);

	_mTransform->setMatrix(_carState->_state);

	if (_carState->_midLine && !_carState->_midLine->empty())
	{
		//calculate speed
		_carState->_speed = (_carState->_O - curO).length();
		osg::Vec3d midD = _carState->_midLine->front() - _carState->_midLine->back();
		int sign = (midD * _carState->_direction > 0) ? 1 : -1;
		_carState->_speed *= sign;
		_carState->cacluateSpeedandAngle();
	}

	//set carstate
	if (*_carState->_OQuad)
	{
		osg::ref_ptr<osg::Vec3dArray> navigationEdge = (*_carState->_OQuad)->getLoop()->getNavigationEdge();
		const osg::Vec3d naviEdge = navigationEdge->back() - navigationEdge->front();
		const osg::Vec3d O = _carState->_O;
		const double lamida = ((O - navigationEdge->front()) * naviEdge) / naviEdge.length2();
		const osg::Vec3d N = navigationEdge->back()*lamida + navigationEdge->front()*(1 - lamida);
		const double dis = (N - O).length();
		_carState->_O_Project = N;
		_carState->_dither = dis;

		_carState->_midLine->clear();
		copy(navigationEdge->begin(), navigationEdge->end(), std::back_inserter(*_carState->_midLine));

		_carState->_updated = true;
	}
}