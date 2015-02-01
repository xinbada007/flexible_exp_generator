#include "stdafx.h"
#include "experimentCallback.h"
#include "collVisitor.h"
#include "debugNode.h"
#include "obstacle.h"

#include <osg/Switch>
#include <osg/MatrixTransform>
#include <osg/PositionAttitudeTransform>
#include <osg/Point>

#include <osgAudio/SoundManager.h>

#include <iterator>
#include <fstream>

ExperimentCallback::ExperimentCallback(const ReadConfig *rc) :_car(NULL), _expTime(0), _expSetting(rc->getExpSetting()), _cameraHUD(NULL)
, _road(NULL), _root(NULL), _dynamicUpdated(false), _mv(NULL), _roadLength(rc->getRoadSet()->_length),_cVisitor(NULL), _deviationWarn(false), _deviationLeft(false), _siren(NULL),
_coin(NULL), _obsListDrawn(false), _opticFlowDrawn(false), _anmCallback(NULL), _centerList(NULL), _timeBuffer(0.020f), _timeLastRecored(0.0f),
_opticFlowPoints(NULL)
{
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
			_siren->setLooping(false);
			_siren->setStopMethod(osgAudio::Stopped);
			_siren->setGain(2.00);
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
// 	if (_siren)
// 	{
// 		_siren.release();
// 	}
// 	if (_coin)
// 	{
// 		_coin.release();
// 	}
}

void ExperimentCallback::createObstacles()
{
	if (_expSetting->_nurbs.empty() && _expSetting->_obstaclesTime->empty())
	{
		return;
	}

	_obsListDrawn = false;

	if (!_expSetting->_obstaclesTime->empty())
	{
		osg::UIntArray::const_iterator pos = _expSetting->_obstaclesTime->begin();
		osg::UIntArray::const_iterator posEND = _expSetting->_obstaclesTime->end();
		while (pos != posEND)
		{
			const osg::Vec3d &center = H_POINT;
			osg::ref_ptr<Obstacle> obs = new Obstacle;
			obs->setSolidType(Solid::solidType::SD_UNDEFINED);
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
			anmPath->setLoopMode((osg::AnimationPath::LoopMode)_expSetting->_animationLoop);
			osg::Vec3dArray::iterator cB = _centerList->begin();
			osg::Vec3dArray::const_iterator cE = _centerList->end();
			double time(0.0f);
			double theta(0.0f);
			while (cB != cE)
			{
				double delta(0.0f);
				if (cB != _centerList->begin())
				{
					delta = ((*cB) - *(cB - 1)).length();
					delta /= _expSetting->_speed;
				}
				time += delta;
				if (cB + 1 != cE)
				{
					const osg::Vec3d &dir = *(cB + 1) - *cB;
					const double costheta = (dir * UP_DIR) / (dir.length()*UP_DIR.length());
					const int sign = ((UP_DIR^dir).z() > 0) ? 1 : -1;
					theta = acos(costheta) * sign;
				}
				osg::Quat rotationQ;
				rotationQ.makeRotate(theta, Z_AXIS);
				anmPath->insert(time, osg::AnimationPath::ControlPoint(*cB, rotationQ));
				++cB;
			}

			if (!_anmCallback)
			{
				_anmCallback = new osg::AnimationPathCallback;
			}

			_anmCallback->setAnimationPath(anmPath.release());
			_anmCallback->setPause(true);

			osg::ref_ptr<Obstacle> obs = new Obstacle;
			obs->setSolidType(Solid::solidType::SD_ANIMATION);
			obs->setTag(ROADTAG::RT_UNSPECIFIED);
			obs->setImage(_expSetting->_imgObsArray);
			obs->createBox(_expSetting->_obsArrayAlign, _expSetting->_obsArraySize);
			_obstacleList.push_back(obs);
		}

		else
		{
			osg::Vec3dArray::const_iterator centerBegin = _centerList->begin();
			osg::Vec3dArray::const_iterator centerEnd = _centerList->end();

			if (_expSetting->_GLPOINTSMODE)
			{
				osg::ref_ptr<Obstacle> obs = new Obstacle;
				osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
				osg::ref_ptr<osg::Point> psize = new osg::Point(5.0f);
				ss->setAttribute(psize);
				ss->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
				ss->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);

				osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
				color->push_back(osg::Vec4(0.0f, 1.0f, 0.0f, 1.0f));
				obs->setPointsColorArray(color);

				obs->createPOINTS(_centerList);
				obs->setStateSet(ss);
				obs->setSolidType(Solid::solidType::GL_POINTS_BODY);
				obs->setTag(ROADTAG::RT_UNSPECIFIED);

				_obstacleList.push_back(obs);
			}
			else
			{
				while (centerBegin != centerEnd)
				{
					osg::ref_ptr<Obstacle> obs = new Obstacle;
					obs->setSolidType(Solid::solidType::COINBODY);
					obs->setTag(ROADTAG::OBS);
					obs->setImage(_expSetting->_imgObsArray);
					obs->createBox(*centerBegin, _expSetting->_obsArraySize);

					_obstacleList.push_back(obs);
					centerBegin++;
				}
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

	_opticFlowPoints = new Obstacle;
	_opticFlowDrawn = false;

	osg::ref_ptr<osg::Vec3Array> points = new osg::Vec3Array;

	std::vector<float> ya, yv, zv;
	int i(0);
	do 
	{
		yv.push_back(i*_expSetting->_depthDensity);
		++i;
	} while (yv.back() < _roadLength && _expSetting->_depthDensity);

	float x, y, z;
	std::pair<std::vector<osg::ref_ptr<osg::Vec3Array>>, unsigned> ver;
	std::vector<osg::ref_ptr<osg::Vec3Array>> verOrder;

	for (i = 0; i < yv.size(); i++)
	{
		unsigned versions(0);
		do
		{
			for (int j = 0; j < _expSetting->_opticFlowDensity; j++)
			{
				if (!_expSetting->_depthDensity)
				{
					y = 0.0f;
				}
				else
				{
					unsigned int temp = randreseed();
					y = randrange(_expSetting->_depthDensity);
					y += drand();
					y += yv.at(i);
				}
				ya.push_back(y);
			}
			for (int j = 0; j < _expSetting->_opticFlowDensity; j++)
			{
				if (!_expSetting->_opticFlowHeight)
				{
					z = 0.0f;
				}
				else
				{
					randreseed();
					const int which = randbiased(0.5f);
					z = randrange(_expSetting->_opticFlowHeight);
					if (which)
					{
						z += drand();
					}
					else
					{
						z = -z;
						z -= drand();
					}
				}
				zv.push_back(z);
			}
			for (int j = 0; j < _expSetting->_opticFlowDensity; j++)
			{
				if (!_expSetting->_opticFlowWidth)
				{
					x = 0.0f;
				}
				else
				{
					randreseed();
					const int which = randbiased(0.5f);
					x = randrange(_expSetting->_opticFlowWidth);
					if (which)
					{
						x += drand();
					}
					else
					{
						x = -x;
						x -= drand();
					}
				}

				z = zv.at(j);
				y = ya.at(j);
				points->push_back(osg::Vec3f(x, y, z));
			}
			zv.clear();
			ya.clear();

			if (!points->empty())
			{
				if (!versions)
				{
					osg::ref_ptr<Obstacle> temp = new Obstacle;
					temp->createPOINTS(points);
					temp->setSolidType(Solid::solidType::GL_POINTS_BODY);
					temp->setTag(ROADTAG::RT_UNSPECIFIED);

					_opticFlowPoints->addChild(temp);
				}
				
				verOrder.push_back(new osg::Vec3Array(points->begin(), points->end()));
				points->clear();
			}
		} while (++versions < _expSetting->_opticFlowVersions);
		ver = std::make_pair(verOrder, 0);
		verOrder.clear();
		_opticFlowVersions.push_back(ver);
	}
	osg::ref_ptr<osg::StateSet> ss = new osg::StateSet;
	osg::ref_ptr<osg::Point> psize = new osg::Point(3.0f);
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

		_root->addChild(_opticFlowPoints);
	}

	if (_opticFlowDrawn)
	{
		const double &y = _car->getCarState()->_O.y();

		const unsigned &TOTL = _opticFlowPoints->getNumChildren();

		double nextY = y + _expSetting->_opticFlowRange;
		const unsigned NEXT = (_expSetting->_depthDensity && _expSetting->_opticFlowRange) ? nextY / _expSetting->_depthDensity : TOTL;

		double prevY = y - _expSetting->_opticFlowRange;
		prevY = std::fmax(prevY, 0.0f);
		const unsigned PREV = (_expSetting->_depthDensity) ? prevY / _expSetting->_depthDensity : 0;

		for (unsigned i = 0; i < std::min(PREV, TOTL);i++)
		{
			Obstacle *obs = dynamic_cast<Obstacle*>(_opticFlowPoints->getChild(i));
			if (obs)
			{
				obs->asSwitch()->setAllChildrenOff();
				obs->setFrameCounts(0);
			}
		}

		for (unsigned i = PREV; i < std::min(NEXT, TOTL); i++)
		{
			Obstacle *obs = dynamic_cast<Obstacle*>(_opticFlowPoints->getChild(i));
			if (obs)
			{
				obs->asSwitch()->setAllChildrenOn();
				obs->setFrameCounts(obs->getFrameCounts()+1);
				dynamicFlow(obs, i);
			}
		}

		for (unsigned i = NEXT; i < TOTL; i++)
		{
			Obstacle *obs = dynamic_cast<Obstacle*>(_opticFlowPoints->getChild(i));
			if (obs)
			{
				obs->asSwitch()->setAllChildrenOff();
				obs->setFrameCounts(0);
			}
		}
	}
}

void ExperimentCallback::dynamicFlow(osg::ref_ptr<Obstacle> obs, const unsigned depth)
{
	if (!_expSetting->_opticFlowFrameCounts)
	{
		return;
	}

	if (obs->getFrameCounts() > _expSetting->_opticFlowFrameCounts)
	{
		obs->setFrameCounts(0);
		osg::Geode *geode = obs->getChild(0)->asGeode();
		if (geode)
		{
			osg::Geometry *gmtry = geode->getDrawable(0)->asGeometry();
			if (gmtry)
			{
				if (gmtry->getPrimitiveSet(0))
				{
					unsigned &order = _opticFlowVersions.at(depth).second;
					osg::ref_ptr<osg::Vec3Array> points = _opticFlowVersions.at(depth).first.at(order);

					++order;
					order %= _opticFlowVersions.at(depth).first.size();

					gmtry->setVertexArray(points);
					gmtry->setPrimitiveSet(0, new osg::DrawArrays(GL_POINTS, 0, points->getNumElements()));
					gmtry->dirtyBound();
					gmtry->dirtyDisplayList();
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
		if (notFound == carState->_currentQuad.size() && _car->getVehicle()->_carReset == Vehicle::VEHICLE_RESET_TYPE::AUTO)
		{
			carState->_reset = true;
		}

		switch (ea->getEventType())
		{
		case::osgGA::GUIEventAdapter::KEYDOWN:
			switch (ea->getKey())
			{
			case::osgGA::GUIEventAdapter::KEY_I:
				if (_anmCallback)
				{
					_anmCallback->setPause(!_anmCallback->getPause());
				}
				break;
			default:
				break;
			}
			break;
		case::osgGA::GUIEventAdapter::FRAME:
			_expTime = std::fmax(carState->_timeReference - (_expSetting->_timer*carState->_startTime), 0.0f);
			deviationCheck();
			showText();
			dynamicChange();
			showObstacle();
			showOpticFlow();
			dealCollision();

			//highest priority controller
			//overwrite all
			trigger();
			break;
		default:
			break;
		}
	}

	traverse(node, nv);
}

void ExperimentCallback::trigger()
{
	if (!_expSetting->_triggerEnable.size())
	{
		return;
	}

	if (_expSetting->_triggerEnable.size() != _expSetting->_triggerTimer->size())
	{
		return;
	}

	std::vector<double>::iterator i = _expSetting->_triggerTimer->begin();
	while (i != _expSetting->_triggerTimer->end() && !_expSetting->_triggerTimer->empty())
	{
		if (_expTime >= *i)
		{
			const unsigned pos = (i - _expSetting->_triggerTimer->begin());
			const triggerEnablePair &pair = _expSetting->_triggerEnable.at(pos);
			std::vector<triggerEnablePair>::const_iterator del = _expSetting->_triggerEnable.begin() + pos;
			if (pair.first && (*i) >= 0.0f)
			{
				switch (pair.second)
				{
				case::Experiment::TRIGGER_COM::ROAD:
					if (_road)
					{
						if (_road->getNewChildDefaultValue())
						{
							_road->setAllChildrenOff();
						}
						else
						{
							_road->setAllChildrenOn();
						}
					}
					break;
				case::Experiment::TRIGGER_COM::FLOW:
					if (_opticFlowPoints)
					{
						if (_opticFlowPoints->getNewChildDefaultValue())
						{
							_opticFlowPoints->setAllChildrenOff();
						}
						else
						{
							_opticFlowPoints->setAllChildrenOn();
						}
					}
					break;
				case::Experiment::TRIGGER_COM::CRASHPERMIT:
					if (_car)
					{
						_car->getCarState()->_crashPermit = !(_car->getCarState()->_crashPermit);
					}
					break;
				case::Experiment::TRIGGER_COM::SOUNDALERT:
					if (_siren)
					{
						_siren->setGain(2.00f);
						_siren->setPlay(true);
					}
					break;
				case::Experiment::TRIGGER_COM::NEAR_FAR_COMPUTE:
					if (_mv)
					{
						osg::Camera *cam = _mv->getMainView()->getCamera();
						if (cam->getComputeNearFarMode() == osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR)
						{
							cam->setComputeNearFarMode(osg::CullSettings::COMPUTE_NEAR_FAR_USING_BOUNDING_VOLUMES);
						}
						else
						{
							cam->setComputeNearFarMode(osg::CullSettings::DO_NOT_COMPUTE_NEAR_FAR);
						}
					}
					break;
				case::Experiment::TRIGGER_COM::QUIT:
					if (_mv)
					{
						_mv->setDone(true);
					}
					break;
				default:
					break;
				}
			}
			_expSetting->_triggerEnable.erase(del);
			_expSetting->_triggerTimer->erase(i);
			if (_expSetting->_triggerTimer->empty())
			{
				break;
			}
			i = _expSetting->_triggerTimer->begin();
			i += pos;
			continue;
		}
		++i;
	}


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
	const obstacleList &obsList = carstate->getObsList();
	const quadList &wallList = carstate->_collisionQuad;
	if (!obsList.size() && !wallList.size())
	{
		carstate->_collide = false;
		return;
	}
	if (wallList.size())
	{
		carstate->_collide = true;
	}

	obstacleList::const_iterator i = obsList.cbegin();
	obstacleList::const_iterator END = obsList.cend();

	while (i != END)
	{
		const unsigned &obstype = (*i)->getSolidType();
		switch (obstype)
		{
		case Solid::solidType::COINBODY:
			if (_coin)
			{
				_coin->setSample(_coinSample);
				_coin->setGain(2.00f);
				_coin->setPlay(true);
			}
			++carstate->_pointsEarned;
			removeNodefromRoad(*i);
			_cVisitor->setMode(OBS);
			_road->accept(*_cVisitor);
			break;
		case Solid::solidType::OBSBODY:
			carstate->_collide = true;
			break;
		default:
			break;
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

	//Visible
	if (_expSetting->_obsVisible)
	{
		const obstacleList &obsList = _cVisitor->getObstacle();
		obstacleList::const_iterator obs_begin = obsList.cbegin();
		obstacleList::const_iterator obs_end = obsList.cend();
		while (obs_begin != obs_end)
		{
			const osg::Vec3d &v = (*obs_begin)->getDistancetoCar();
			const double YD = abs(v * UP_DIR);
			if (YD <= _expSetting->_obsVisible)
			{
				(*obs_begin)->setAllChildrenOn();
			}
			else
			{
				(*obs_begin)->setAllChildrenOff();
			}
			++obs_begin;
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
				double reDistance(*requiedDistance);
				osg::ref_ptr<osg::Vec3dArray> navi = (*curO)->getLoop()->getNavigationEdge();
				osg::Vec3d mid = navi->front() - navi->back();
				double alreadyDistance((carState->_O_Project - navi->back()).length());
				double distance(mid.length() - alreadyDistance);
				reDistance -= distance;
				while (reDistance > 0)
				{
					curO++;
					if (!(*curO))
					{
						break;
					}
					navi = (*curO)->getLoop()->getNavigationEdge();
					mid = navi->front() - navi->back();
					reDistance -= mid.length();
				}

				osg::IntArray::iterator pos = _expSetting->_obstaclePos->begin() + offset;
				osg::DoubleArray::iterator posOffset = _expSetting->_obsPosOffset->begin() + offset;
				const double distanceRatio = std::fmin((mid.length() + reDistance) / mid.length(), 1.0f);
				osg::Vec3d center = navi->front()*distanceRatio + navi->back()*(1 - distanceRatio);
				
				center = center * osg::Matrix::translate(X_AXIS * *pos * _expSetting->_offset * 0.25);
				center = center * osg::Matrix::translate(X_AXIS * *posOffset);
				std::vector<osg::ref_ptr<Obstacle>>::iterator posOBS = _collisionOBSList.begin() + offset;
				osg::ref_ptr<Obstacle> obs = *posOBS;
				const osg::Vec3d &OC = H_POINT;
				osg::Matrixd m = osg::Matrix::translate(center - OC);
				obs->multiplyMatrix(m);

				osg::UIntArray::iterator posCollision = _expSetting->_obsCollision->begin() + offset;
				const unsigned flagCollision = *posCollision;
				switch (flagCollision)
				{
				case 0:
					obs->setSolidType(Solid::solidType::COINBODY);
					obs->setTag(ROADTAG::OBS);
					break;
				case 1:
					obs->setSolidType(Solid::solidType::OBSBODY);
					obs->setTag(ROADTAG::OBS);
					break;
				default:
					obs->setSolidType(Solid::solidType::OBSBODY);
					obs->setTag(ROADTAG::OBS);
					break;
				}

				_expSetting->_obstaclesTime->erase(obsTi);
				_expSetting->_obstacleRange->erase(requiedDistance);
				_expSetting->_obstaclePos->erase(pos);
				_expSetting->_obsPosOffset->erase(posOffset);
				_expSetting->_obsCollision->erase(posCollision);
				_collisionOBSList.erase(posOBS);
				hit = true;

				if (_expSetting->_obstaclesTime->empty())
				{
					break;
				}
				obsTi = _expSetting->_obstaclesTime->begin();
				obsTi += offset;
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

	osg::Vec3d position(0.5*X, 0.6*Y, 0.0f);
	std::string font("fonts/Calibri.ttf");
	_textHUD->setFont(font);
	_textHUD->setFontResolution(64, 64);
	_textHUD->setCharacterSize(64);
	_textHUD->setPosition(position);
	_textHUD->setAlignment(osgText::TextBase::CENTER_CENTER);
	_textHUD->setDataVariance(osg::Object::DYNAMIC);

	_geodeHUD->addDrawable(_textHUD);
	_cameraHUD->addChild(_geodeHUD);
}