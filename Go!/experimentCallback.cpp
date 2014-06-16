#include "stdafx.h"
#include "experimentCallback.h"
#include "collVisitor.h"

#include <osg/Switch>

ExperimentCallback::ExperimentCallback(const ReadConfig *rc) :_carState(NULL), _expTime(0), _expSetting(rc->getExpSetting()), _camera(NULL)
{
	_txtSwithcer = new osg::Switch;
	_text = new osgText::Text;
	_geode = new osg::Geode;
}

ExperimentCallback::~ExperimentCallback()
{
}

void ExperimentCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
	const CollVisitor *refCV = dynamic_cast<CollVisitor*>(this->getUserData());
	if (refCV && refCV->getCar())
	{
		_carState = refCV->getCar()->getCarState();
	}

	if (_carState)
	{
		_expTime = _carState->_timeReference;
		showText();
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

	_text->setText((whichshow)?(*whichshow):(""));
}

void ExperimentCallback::dynamicChange()
{
	const osg::UIntArray *dynamicC = _expSetting->_dynamicChange;
	osg::UIntArray::const_iterator i = dynamicC->begin();
	while (i != dynamicC->end())
	{
		if (_expTime == *i)
		{
			_carState->_dynamic = !_carState->_dynamic;
		}

		i++;
	}
}

void ExperimentCallback::setHUDCamera(osg::Camera *cam)
{
	_camera = cam;

	const double X = _camera->getViewport()->width();
	const double Y = _camera->getViewport()->height();

	osg::Vec3d position(0.4*X, 0.5*Y, 0.0f);
	std::string font("fonts/arial.ttf");
	_text->setFont(font);
	_text->setFontResolution(512, 512);
	_text->setPosition(position);
	_text->setDataVariance(osg::Object::DYNAMIC);

	_geode->addDrawable(_text);
	_camera->addChild(_geode);
}