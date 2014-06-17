#include "stdafx.h"
#include "experimentCallback.h"
#include "collVisitor.h"
#include "debugNode.h"
#include "obstacle.h"
#include "renderVistor.h"

#include <osg/Switch>
#include <osgGA/EventVisitor>
#include <osgDB/ReadFile>
#include <osg/ShapeDrawable>

ExperimentCallback::ExperimentCallback(const ReadConfig *rc) :_carState(NULL), _expTime(0), _expSetting(rc->getExpSetting()), _cameraHUD(NULL)
, _road(NULL), _root(NULL), _dynamicUpdated(false)
{
	_dynamic = new osg::UIntArray(_expSetting->_dynamicChange->rbegin(),_expSetting->_dynamicChange->rend());
	_obstacle = new osg::UIntArray(_expSetting->_obstaclesTime->rbegin(), _expSetting->_obstaclesTime->rend());
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
			createObstacle();
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

	const int &size_moment = moment->size();
	const int &size_period = period->size();
	const int &size_content = content.size();
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

void ExperimentCallback::createObstacle()
{
	if (_obstacle->empty())
	{
		return;
	}

	const int &ob_size = _expSetting->_obstaclesTime->size();
	const int &range_size = _expSetting->_obstacleRange->size();
	if (ob_size != range_size)
	{
		osg::notify(osg::WARN) << "cannot put obstacles because size are inconsistent" << std::endl;
		return;
	}

	bool hit(false);
	osg::UIntArray::iterator i = _obstacle->end() - 1;
	while (i != _obstacle->begin() - 1)
	{
		if (_expTime > *i)
		{
			hit = true;
			_obstacle->pop_back();
			break;
		}
		i--;
	}

	if (hit)
	{
		int k = i - _obstacle->end();
		osg::DoubleArray::iterator j = _expSetting->_obstacleRange->begin() + (i - _obstacle->end());
		Plane::reverse_across_iterator curO = *_carState->_OQuad;
		if (*curO)
		{
			double distance(0.0f);
			osg::ref_ptr<osg::Vec3dArray> navi = (*curO)->getLoop()->getNavigationEdge();
			osg::Vec3d mid = navi->front() - navi->back();
			while (distance < *j)
			{
				curO++;
				navi = (*curO)->getLoop()->getNavigationEdge();
				mid = navi->front() - navi->back();
				distance += mid.length();
			}

			osg::Vec3d center = (navi->front() + navi->back()) * 0.5f;
// 			osg::ref_ptr<osg::Box> box = new osg::Box;
// 			box->setCenter(center);
// 			box->setHalfLengths(osg::Vec3d(1.0f, 1.0f, 1.0f));
// 
// 			osg::ref_ptr<osg::Geode> geode = new osg::Geode;
// 			geode->addDrawable(new osg::ShapeDrawable(box.get()));
// 
// 			_road->addChild(geode);

			osg::ref_ptr<Obstacle> obs = new Obstacle;
			obs->createBox(center, osg::Vec3d(1.0f, 1.0f, 1.0f));
			osg::ref_ptr<RenderVistor> rv = new RenderVistor;
			rv->setBeginMode(GL_QUADS);
			obs->accept(*rv);
			_road->addChild(obs);
			hit = false;
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