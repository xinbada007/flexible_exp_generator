#include "stdafx.h"
#include "experimentCallback.h"
#include "collVisitor.h"
#include "debugNode.h"

#include <osg/Switch>
#include <osgGA/EventVisitor>
#include <osgDB/ReadFile>

ExperimentCallback::ExperimentCallback(const ReadConfig *rc) :_carState(NULL), _expTime(0), _expSetting(rc->getExpSetting()), _cameraHUD(NULL)
, _road(NULL), _root(NULL), _dynamicUpdated(false)
{
	_dynamic = new osg::UIntArray(_expSetting->_dynamicChange->rbegin(),_expSetting->_dynamicChange->rend());
	_textHUD = new osgText::Text;
	_geodeHUD = new osg::Geode;
}

ExperimentCallback::~ExperimentCallback()
{
}

void ExperimentCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
	const CollVisitor *refCV = dynamic_cast<CollVisitor*>(this->getUserData());
	if (refCV && refCV->getCar() && !_carState)
	{
		_carState = refCV->getCar()->getCarState();
	}
	if (!_root)
	{
		_root = dynamic_cast<osg::Group*>(node);
	}
	if (!_road)
	{
		switchRoad swRD;
		_root->accept(swRD);
		_road = swRD.getSwRoad();
	}

	osgGA::EventVisitor *ev = dynamic_cast<osgGA::EventVisitor*>(nv);
	osgGA::EventQueue::Events events = (ev) ? ev->getEvents() : events;
	osgGA::GUIEventAdapter *ea = (!events.empty()) ? events.front() : NULL;

	if (_carState && ea)
	{
		switch (ea->getEventType())
		{
		case::osgGA::GUIEventAdapter::FRAME:
			_expTime = _carState->_timeReference;
			showText();
			dynamicChange();
			break;
		default:
			break;
		}
	}

	traverse(node, nv);
}

void ExperimentCallback::showText()
{
	const osg::UIntArray *moment = _expSetting->_textTime;
	const osg::DoubleArray *period = _expSetting->_textPeriod;
	const stringList &content = _expSetting->_textContent;

	const int size_moment = moment->size();
	const int size_period = period->size();
	const int size_content = content.size();
	if (size_moment != size_content || size_moment != size_period)
	{
		osg::notify(osg::WARN) << "cannot display text because time and period and text are inconsistent" << std::endl;
		return;
	}

	int i = 0;
	const int limit(moment->size());
	const std::string *whichshow(NULL);
	while (i != limit)
	{
		bool show = false;
		const unsigned time = moment->at(i);
		const double last = period->at(i);
		if (_expTime >= time && _expTime <= time + last)
		{
			whichshow = &(_expSetting->_textContent[i]);
		}
		i++;
	}

	_textHUD->setText((whichshow)?(*whichshow):(""));
}

void ExperimentCallback::dynamicChange()
{
	if (!_dynamic->empty())
	{
		osg::UIntArray::iterator i = _dynamic->end() - 1;
		while (i != _dynamic->begin() - 1)
		{
			if (_expTime > *i)
			{
				_carState->_dynamic = !_carState->_dynamic;
				_dynamicUpdated = true;
				_thisMomentDynamic = _expTime;
				_dynamic->pop_back();
				break;
			}
			i--;
		}
	}

	if (_dynamicUpdated)
	{
		if (_expTime <= _thisMomentDynamic + _expSetting->_dynamicChangeLasting)
		{
			if (!_expSetting->_dynamicChangeCondition)
			{
				_road->setAllChildrenOff();
				_dynamicUpdated = true;
			}
		}
		else
		{
			_road->setAllChildrenOn();
			_dynamicUpdated = false;
		}
	}
}

void ExperimentCallback::setHUDCamera(osg::Camera *cam)
{
	_cameraHUD = cam;

	const double X = _cameraHUD->getViewport()->width();
	const double Y = _cameraHUD->getViewport()->height();

	osg::Vec3d position(0.4*X, 0.5*Y, 0.0f);
	std::string font("fonts/arial.ttf");
	_textHUD->setFont(font);
	_textHUD->setFontResolution(512, 512);
	_textHUD->setPosition(position);
	_textHUD->setDataVariance(osg::Object::DYNAMIC);

	_geodeHUD->addDrawable(_textHUD);
	_cameraHUD->addChild(_geodeHUD);
}