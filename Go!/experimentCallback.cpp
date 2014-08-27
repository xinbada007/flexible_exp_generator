#include "stdafx.h"
#include "experimentCallback.h"
#include "collVisitor.h"
#include "debugNode.h"
#include "obstacle.h"
#include "renderVistor.h"
#include "textureVisitor.h"

#include <osg/Switch>
#include <osgGA/EventVisitor>
#include <osgDB/ReadFile>
#include <osg/ShapeDrawable>

ExperimentCallback::ExperimentCallback(const ReadConfig *rc) :_carState(NULL), _expTime(0), _expSetting(rc->getExpSetting()), _cameraHUD(NULL)
, _road(NULL), _root(NULL), _dynamicUpdated(false), _mv(NULL), _deviationWarn(false)
{
	_dynamic = new osg::UIntArray(_expSetting->_dynamicChange->rbegin(),_expSetting->_dynamicChange->rend());
	_obstacle = new osg::IntArray(_expSetting->_obstaclesTime->begin(), _expSetting->_obstaclesTime->end());
	_textHUD = new osgText::Text;
	_geodeHUD = new osg::Geode;
	_geodeHUD->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
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
		Plane::reverse_across_iterator start = *_carState->_OQuad;
		if (*start)
		{
			const int future = (int)((*start)->getHomeS()->getNumGeometry()*0.01f) + 1;
			start.add(future);
			if (!(*start))
			{
				if (_mv)
				{
					_mv->setDone(true);
				}
			}
		}
		
		quadList::const_iterator i = _carState->_currentQuad.cbegin();
		unsigned notFound(0);
		while(i != _carState->_currentQuad.cend())
		{
			if (!(*i)) notFound++;
			i++;
		}
		if (notFound == _carState->_currentQuad.size())
		{
			_carState->_reset = true;
		}
		else
		{
			_carState->_reset = false;
		}

		switch (ea->getEventType())
		{
		case::osgGA::GUIEventAdapter::FRAME:
			_expTime = _carState->_timeReference;
			deviationCheck();
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

void ExperimentCallback::deviationCheck()
{
	if (!_expSetting->_deviation)
	{
		return;
	}

	_deviationWarn = (_carState->_dither > _expSetting->_deviation) ? true : false;
}

void ExperimentCallback::showText()
{
	//First display Warn Information
	const std::string *warnshow(NULL);
	if (_deviationWarn && ((_carState->_frameStamp / 20) % 2))
	{
		warnshow = &_expSetting->_deviationWarn;
	}

	//Display designed Information
	const osg::UIntArray *moment = _expSetting->_textTime;
	const osg::DoubleArray *period = _expSetting->_textPeriod;
	const stringList &content = _expSetting->_textContent;

	const int &size_moment = moment->size();
	const int &size_period = period->size();
	const int &size_content = content.size();
	
// 	if (!size_moment || !size_period || !size_content)
// 	{
// 		return;
// 	}

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

	std::string totalTxt;
	totalTxt.push_back('\n');
	if (warnshow)
	{
		totalTxt += *warnshow;
	}
	if (whichshow)
	{
		totalTxt += *whichshow;
	}

	_textHUD->setText(totalTxt);
	_textHUD->setColor(osg::Vec4(1.0f,0.0f,0.0f,1.0f));

	//_textHUD->setText((whichshow)?(*whichshow):(""));
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
				_dynamic->erase(i);
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
	const int &pos_size = _expSetting->_obstaclePos->size();

	if (!ob_size || !range_size || !pos_size)
	{
		return;
	}

	if (ob_size != range_size || ob_size != pos_size)
	{
		osg::notify(osg::WARN) << "cannot put obstacles because size are inconsistent" << std::endl;
		return;
	}

	bool hit(false);
	osg::IntArray::iterator obsi = _obstacle->begin();
	osg::UIntArray::iterator obsTi = _expSetting->_obstaclesTime->begin();
	while (obsTi != _expSetting->_obstaclesTime->end())
	{
		if (_expTime > *obsTi && *obsi != -1)
		{
			hit = true;
			*obsi = -1;
			break;
		}
		obsTi++;
		obsi++;
	}

	if (hit)
	{
		Plane::reverse_across_iterator curO = *_carState->_OQuad;
		if (*curO)
		{
			const int offset = obsTi - _expSetting->_obstaclesTime->begin();
			osg::DoubleArray::iterator requiedDistance = _expSetting->_obstacleRange->begin() + offset;
			double distance(0.0f);
			osg::ref_ptr<osg::Vec3dArray> navi = (*curO)->getLoop()->getNavigationEdge();
			osg::Vec3d mid = navi->front() - navi->back();
			while (distance < *requiedDistance)
			{
				curO++;
				navi = (*curO)->getLoop()->getNavigationEdge();
				mid = navi->front() - navi->back();
				distance += mid.length();
			}
			osg::IntArray::iterator pos = _expSetting->_obstaclePos->begin() + offset;
			osg::Vec3d center = (navi->front() + navi->back()) * 0.5f;
			center = center * osg::Matrix::translate(X_AXIS * *pos * _expSetting->_offset * 0.25f);
			const double hflength = _expSetting->_offset * 0.20f;
			osg::ref_ptr<Obstacle> obs = new Obstacle;
			obs->createBox(center, osg::Vec3d(hflength,1.0f,1.0f));

			//render
			osg::ref_ptr<RenderVistor> rv = new RenderVistor;
			rv->setBeginMode(GL_QUADS);
			obs->accept(*rv);
			_road->addChild(obs);
			//texture      
			osg::ref_ptr<osg::Image> img = osgDB::readImageFile("..\\Resources\\texture\\wall.jpg");
			obs->setImage(img);
			osg::ref_ptr<TextureVisitor> tv = new TextureVisitor;
			obs->accept(*tv);
			//visitor
			CollVisitor *refCV = dynamic_cast<CollVisitor*>(this->getUserData());
			if (refCV)
			{
				refCV->setMode(OBS);
				_road->accept(*refCV);
			}
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