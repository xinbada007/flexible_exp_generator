#include "stdafx.h"
#include "experimentCallback.h"
#include "collVisitor.h"
#include "debugNode.h"
#include "obstacle.h"

#include <osg/Switch>
#include <osgGA/EventVisitor>
#include <osgDB/ReadFile>
#include <osg/ShapeDrawable>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osg/Point>

#include <osgAudio/SoundManager.h>
#include <iterator>

ExperimentCallback::ExperimentCallback(const ReadConfig *rc) :_car(NULL), _expTime(0), _expSetting(rc->getExpSetting()), _cameraHUD(NULL)
, _road(NULL), _root(NULL), _dynamicUpdated(false), _mv(NULL), _roadLength(rc->getRoadSet()->_length),_cVisitor(NULL), _deviationWarn(false), _deviationLeft(false), _siren(NULL),
_coin(NULL), _obsListDrawn(false), _opticFlowDrawn(false), _anmCallback(NULL), _centerList(NULL), _timeBuffer(0.020f), _timeLastRecored(0.0f)
{
	_opticFlowPoints = new Obstacle;

	_dynamic = new osg::UIntArray(_expSetting->_dynamicChange->rbegin(),_expSetting->_dynamicChange->rend());
	_textHUD = new osgText::Text;
	_geodeHUD = new osg::Geode;
	_geodeHUD->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

	createObstacles();
	createOpticFlow();

//	osg::ref_ptr<osgAudio::FileStream> sample = NULL;
	osg::ref_ptr<osgAudio::Sample> sample = NULL;
	try
	{
		sample = new osgAudio::Sample(_expSetting->_deviationSiren);
	}
	catch (std::exception &e)
	{
		osg::notify(osg::WARN) << "Error:  " << e.what() << std::endl;
		sample = NULL;
	}

	_coinSample = NULL;
	try
	{
		_coinSample = new osgAudio::Sample("../Resources/sound/smw_coin.wav");
	}
	catch (std::exception &e)
	{
		osg::notify(osg::WARN) << "Error:  " << e.what() << std::endl;
		_coinSample = NULL;
	}

	if (sample)
	{
		_siren = osgAudio::SoundManager::instance()->findSoundState("GOsiren");
		if (!_siren)
		{
			_siren = new osgAudio::SoundState("GOsiren");
			_siren->allocateSource(20);
			_siren->setSample(sample.release());
			_siren->setAmbient(true);
			_siren->setPlay(false);
			_siren->setLooping(true);
			osgAudio::SoundManager::instance()->addSoundState(_siren);
		}
	}
	if (_coinSample)
	{
		_coin = osgAudio::SoundManager::instance()->findSoundState("Gocoin");
		if (!_coin)
		{
			_coin = new osgAudio::SoundState("Gocoin");
			_coin->allocateSource(20);
			_coin->setSample(_coinSample.get());
			_coin->setAmbient(true);
			_coin->setPlay(false);
			_coin->setLooping(false);
			_coin->setStopMethod(osgAudio::Stopped);
			_coin->setGain(2.00);
			osgAudio::SoundManager::instance()->addSoundState(_coin);
		}
	}
}

ExperimentCallback::~ExperimentCallback()
{
	if (_siren)
	{
		_siren.release();
	}
	if (_coin)
	{
		_coin.release();
	}
}

void ExperimentCallback::createObstacles()
{
	if (_expSetting->_nurbs.empty() && _expSetting->_obstaclePos->empty())
	{
		return;
	}

	_obsListDrawn = false;

	osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
	osg::ref_ptr<osg::Point> psize = new osg::Point(10.0f);
	ss->setAttribute(psize);
	ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);

	if (!_expSetting->_obstaclePos->empty())
	{
		osg::IntArray::const_iterator pos = _expSetting->_obstaclePos->begin();
		osg::IntArray::const_iterator posEND = _expSetting->_obstaclePos->end();
		while (pos != posEND)
		{
			const osg::Vec3d &center = H_POINT;
			osg::ref_ptr<Obstacle> obs = new Obstacle;
			obs->setSolidType(Solid::solidType::OBSBODY);
			obs->setTag(ROADTAG::RT_UNSPECIFIED);
			obs->setImage(_expSetting->_imgOBS);
			switch (_expSetting->_obsShape)
			{
			case 0:
				obs->createBox(center, _expSetting->_obsSize);
				break;
			case 1:
				obs->createCylinder(center, _expSetting->_obsSize.x(), _expSetting->_obsSize.y());
				break;
			case 3:
				obs->createSphere(center, _expSetting->_obsSize.x());
				break;
			default:
				obs->createCylinder(center, _expSetting->_obsSize.x(), _expSetting->_obsSize.y());
				break;
			}
			_obstacleList.push_back(obs);
			_collisionOBSList.push_back(obs);
			++pos;
		}
	}

	if (!_expSetting->_nurbs.empty())
	{
		_centerList = new osg::Vec3dArray;
		nurbsList::const_iterator i = _expSetting->_nurbs.cbegin();
		while (i != _expSetting->_nurbs.cend())
		{

			osg::Vec3dArray::const_iterator start = (*i)->_path->begin();
			osg::Vec3dArray::const_iterator end = (*i)->_path->end();
			while (start != end)
			{
				_centerList->push_back(*start);
				start++;
			}
			i++;
		}

		if (_expSetting->_animationMode)
		{
			osg::ref_ptr<osg::AnimationPath> anmPath = new osg::AnimationPath;
			anmPath->setLoopMode(osg::AnimationPath::NO_LOOPING);
			osg::Vec3dArray::iterator cB = _centerList->begin();
			osg::Vec3dArray::const_iterator cE = _centerList->end();
			double time(0.0f);
			while (cB != cE)
			{
				double delta(0.0f);
				if (cB != _centerList->begin())
				{
					delta = ((*cB) - *(cB - 1)).length();
					delta /= _expSetting->_speed;
				}
				time += delta;
				anmPath->insert(time, osg::AnimationPath::ControlPoint(*cB));
				++cB;
			}

			if (!_anmCallback)
			{
				_anmCallback = new osg::AnimationPathCallback;
			}

			_anmCallback->setAnimationPath(anmPath.release());

			osg::ref_ptr<Obstacle> obs = new Obstacle;
			obs->setSolidType(Solid::solidType::SD_ANIMATION);
			obs->setTag(ROADTAG::RT_UNSPECIFIED);
			obs->setImage(_expSetting->_imgObsArray);
			obs->createBox(osg::Vec3d(0.0f, 0.0f, 0.0f), _expSetting->_obsSize*_expSetting->_obsArraySize);
			_obstacleList.push_back(obs);
		}

		else
		{
			osg::Vec3dArray::const_iterator centerBegin = _centerList->begin();
			osg::Vec3dArray::const_iterator centerEnd = _centerList->end();
			while (centerBegin != centerEnd)
			{
				osg::ref_ptr<Obstacle> obs = new Obstacle;

				if (_expSetting->_GLPOINTSMODE)
				{
					obs->createPOINTS(*centerBegin);
					obs->setStateSet(ss);
					obs->setSolidType(Solid::solidType::GL_POINTS_BODY);
					obs->setTag(ROADTAG::RT_UNSPECIFIED);
				}
				else
				{
					obs->setSolidType(Solid::solidType::COINBODY);
					obs->setTag(ROADTAG::OBS);
					obs->setImage(_expSetting->_imgObsArray);
					obs->createBox(*centerBegin, _expSetting->_obsSize*_expSetting->_obsArraySize);
				}

				_obstacleList.push_back(obs);
				centerBegin++;
			}
		}
	}
}

void ExperimentCallback::createOpticFlow()
{
	if (!_expSetting->_opticFlow)
	{
		return;
	}

	_opticFlowDrawn = false;

	osg::ref_ptr<osg::Vec3dArray> points = new osg::Vec3dArray;

	std::vector<double> xv, yv, zv;
	int i(0);
	do 
	{
		yv.push_back(i*_expSetting->_depthDensity);
		++i;
	} while (yv.back() < _roadLength);

	double x, y, z;
	for (i = 0; i < yv.size(); i++)
	{
		y = yv.at(i);

		for (int j = 0; j < _expSetting->_opticFlowHDensity; j++)
		{
			double temp = drand();
			temp = -0.5f * _expSetting->_opticFlowHeight + temp * _expSetting->_opticFlowHeight;
			zv.push_back(temp);
		}
		for (int j = 0; j < _expSetting->_opticFlowWDensity; j++)
		{
			x = drand();
			x = -0.5f * _expSetting->_opticFlowWidth + x * _expSetting->_opticFlowWidth;
			
			const int index = j % zv.size();
			z = zv.at(index);

			points->push_back(osg::Vec3(x, y, z));
		}

		if (!points->empty())
		{
			osg::ref_ptr<Obstacle> temp = new Obstacle;
			temp->createPOINTS(points);
			temp->setSolidType(Solid::solidType::GL_POINTS_BODY);
			temp->setTag(ROADTAG::RT_UNSPECIFIED);

			_opticFlowPoints->addChild(temp);
			points->clear();
		}
	}

	osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
	osg::ref_ptr<osg::Point> psize = new osg::Point(5.0f);
	ss->setAttribute(psize);
	ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
	ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);

	_opticFlowPoints->setStateSet(ss);
}

void ExperimentCallback::showOpticFlow()
{
	if (!_expSetting->_opticFlow)
	{
		return;
	}

	if (!_opticFlowDrawn && _opticFlowPoints && _opticFlowPoints->getNumChildren())
	{
		_opticFlowDrawn = true;

		_road->addChild(_opticFlowPoints);
	}

	if (_opticFlowDrawn)
	{
		double y = _car->getCarState()->_O.y();
		const unsigned CURR = y / _expSetting->_depthDensity;

		y += _expSetting->_opticFlowRange;
		const unsigned NEXT = y / _expSetting->_depthDensity + 0.5f;

		y -= 2 * _expSetting->_opticFlowRange;
		y = std::fmax(y, 0.0f);
		const unsigned PREV = y / _expSetting->_depthDensity;

		const unsigned TOTL = _opticFlowPoints->getNumChildren();
		for (unsigned i = 0; i < std::min(PREV, TOTL);i++)
		{
			osg::Node *node = _opticFlowPoints->getChild(i);
			if (node)
			{
				if (node->asSwitch())
				{
					node->asSwitch()->setAllChildrenOff();
				}
			}
		}

		for (unsigned i = PREV; i < std::min(NEXT, TOTL); i++)
		{
			osg::Node *node = _opticFlowPoints->getChild(i);
			if (node)
			{
				if (node->asSwitch())
				{
					node->asSwitch()->setAllChildrenOn();
				}
			}
		}

		for (unsigned i = NEXT; i < TOTL; i++)
		{
			osg::Node *node = _opticFlowPoints->getChild(i);
			if (node)
			{
				if (node->asSwitch())
				{
					node->asSwitch()->setAllChildrenOff();
				}
			}
		}
	}
}


void ExperimentCallback::operator()(osg::Node* node, osg::NodeVisitor* nv)
{
	if (!_cVisitor)
	{
		_cVisitor = CollVisitor::instance();
	}
	if (!_car)
	{
		osg::notify(osg::WARN) << "Cannot find Car failed to control experiment" << std::endl;
		return;
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
	CarState *carState = _car->getCarState();
	if (carState && ea)
	{
		Plane::reverse_across_iterator start = *carState->_OQuad;
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
		
		quadList::const_iterator i = carState->_currentQuad.cbegin();
		unsigned notFound(0);
		while (i != carState->_currentQuad.cend())
		{
			if (!(*i)) notFound++;
			i++;
		}
		if (notFound == carState->_currentQuad.size() && _car->getVehicle()->_carReset == 1)
		{
			carState->_reset = true;
		}
		else
		{
			carState->_reset = false;
		}

		osg::ref_ptr<osg::AnimationPath> anmPath = NULL;
		switch (ea->getEventType())
		{
		case::osgGA::GUIEventAdapter::FRAME:
			_expTime = carState->_timeReference;
			deviationCheck();
			showText();
			dynamicChange();
			showObstacle();
			showOpticFlow();
			dealCollision();
			if (_anmCallback)
			{
				_anmCallback->setPause(!carState->getRSpeed() ? true : false);
			}
			break;
		default:
			break;
		}
	}

	traverse(node, nv);
}

void ExperimentCallback::removeNodefromRoad(osg::Node *n)
{
	if (n->getNumParents() > 1)
	{
		osg::notify(osg::WARN) << "cannot remove this node because it has multiple parents" << std::endl;
		return;
	}

	if (!_road->removeChild(n))
	{
		removeNodefromRoad(n->getParent(0));
	}

	return;
}

void ExperimentCallback::dealCollision()
{
	CarState *carstate = _car->getCarState();
	const solidList &obsList = carstate->getObsList();
	const quadList &wallList = carstate->_collisionQuad;
	if (!obsList.size() && !wallList.size())
	{
		return;
	}

	carstate->_collide = true;
	solidList::const_iterator i = obsList.cbegin();
	solidList::const_iterator END = obsList.cend();

	while (i != END)
	{
		if ((*i)->getSolidType() == Solid::solidType::COINBODY)
		{
			if (_coin)
			{
				_coin->setSample(_coinSample.get());
				_coin->setGain(2.00);
				_coin->setPlay(true);
			}
			carstate->_collide = false;
			++carstate->_pointsEarned;
			removeNodefromRoad(*i);
			_cVisitor->setMode(OBS);
			_road->accept(*_cVisitor);
		}
		++i;
	}
}

void ExperimentCallback::deviationCheck()
{
	if (!_expSetting->_deviation)
	{
		return;
	}

	CarState *carState = _car->getCarState();
	_deviationWarn = (abs(carState->_distancefromBase) > _expSetting->_deviation) ? true : false;
	_deviationLeft = carState->_distancefromBase < 0;
	if (_siren)
	{
		if (_siren->isPlaying() != _deviationWarn)
		{
			_siren->setPlay(_deviationWarn);
		}
	}
}

void ExperimentCallback::showText()
{
	//First display Warn Information
	CarState *carState = _car->getCarState();
	const std::string *warnshow(NULL);
	std::string deviationDisplay;
	if (_deviationWarn && ((carState->_frameStamp / 20) % 2))
	{
		deviationDisplay = _expSetting->_deviationWarn;
// 		if (_deviationLeft)
// 		{
// 			deviationDisplay += "\nTO RIGHT";
// 		}
// 		else
// 		{
// 			deviationDisplay += "\nTO LEFT";
// 		}

		warnshow = &deviationDisplay;		
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
	CarState *carState = _car->getCarState();
	if (!_dynamic->empty())
	{
		osg::UIntArray::iterator i = _dynamic->end() - 1;
		while (i != _dynamic->begin() - 1)
		{
			if (_expTime > *i)
			{
				carState->_dynamic = !carState->_dynamic;
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

void ExperimentCallback::showObstacle()
{
	if (_expSetting->_obstaclesTime->empty() && _obstacleList.empty())
	{
		return;
	}

	if (!_obsListDrawn && !_obstacleList.empty())
	{
		_obsListDrawn = true;

		std::vector<osg::ref_ptr<Obstacle>>::const_iterator i = _obstacleList.cbegin();
		while (i != _obstacleList.cend())
		{
			osg::ref_ptr<Obstacle> obs = *i;
			osg::ref_ptr<osg::MatrixTransform> mT = new osg::MatrixTransform;
			mT->addChild(obs);
			_road->addChild(mT);

			if (_anmCallback && ((*i)->getSolidType() == Solid::solidType::SD_ANIMATION))
			{
				mT->setUpdateCallback(_anmCallback);
				_road->addChild(mT);
			}

			i++;
		}
		if (_cVisitor)
		{
			_cVisitor->setMode(OBS);
			_road->accept(*_cVisitor);
		}
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

	osg::UIntArray::iterator obsTi = _expSetting->_obstaclesTime->begin();
	bool hit(false);
	while (obsTi != _expSetting->_obstaclesTime->end())
	{
		if (_expTime > *obsTi)
		{
			CarState *carState = _car->getCarState();
			Plane::reverse_across_iterator curO = *carState->_OQuad;
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
					if (!(*curO))
					{
						break;
					}
					navi = (*curO)->getLoop()->getNavigationEdge();
					mid = navi->front() - navi->back();
					distance += mid.length();
				}

				osg::IntArray::iterator pos = _expSetting->_obstaclePos->begin() + offset;
				osg::DoubleArray::iterator posOffset = _expSetting->_obsPosOffset->begin() + offset;
				osg::Vec3d center = (navi->front() + navi->back()) * 0.5f;
				center = center * osg::Matrix::translate(X_AXIS * *pos * _expSetting->_offset * 0.25);
				center = center * osg::Matrix::translate(X_AXIS * *posOffset);
				std::vector<osg::ref_ptr<Obstacle>>::iterator posOBS = _collisionOBSList.begin() + offset;
				osg::ref_ptr<Obstacle> obs = *posOBS;
				const osg::Vec3d &OC = H_POINT;
				osg::Matrixd m = osg::Matrix::translate(center - OC);
				obs->multiplyMatrix(m);
				obs->setSolidType(Solid::solidType::OBSBODY);
				obs->setTag(ROADTAG::OBS);

				_expSetting->_obstaclesTime->erase(obsTi);
				_expSetting->_obstacleRange->erase(requiedDistance);
				_expSetting->_obstaclePos->erase(pos);
				_expSetting->_obsPosOffset->erase(posOffset);
				_collisionOBSList.erase(posOBS);
				hit = true;

				continue;
			}
		}

		++obsTi;
	}

	if (hit)
	{
		//visitor
		if (_cVisitor)
		{
			_cVisitor->setMode(OBS);
			_road->accept(*_cVisitor);
		}
	}
}

void ExperimentCallback::setHUDCamera(osg::Camera *cam)
{
	if (!cam)
	{
		return;
	}

	_cameraHUD = cam;

	const double X = _cameraHUD->getViewport()->width();
	const double Y = _cameraHUD->getViewport()->height();

	osg::Vec3d position(0.5*X, 0.5*Y, 0.0f);
	std::string font("fonts/arial.ttf");
	_textHUD->setFont(font);
	_textHUD->setFontResolution(28, 28);
	_textHUD->setCharacterSize(28);
	_textHUD->setPosition(position);
	_textHUD->setAlignment(osgText::TextBase::CENTER_CENTER);
	_textHUD->setDataVariance(osg::Object::DYNAMIC);

	_geodeHUD->addDrawable(_textHUD);
	_cameraHUD->addChild(_geodeHUD);
}