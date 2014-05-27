#include "stdafx.h"
#include "debugNode.h"
#include "collVisitor.h"
#include "car.h"
#include "math.h"
#include "Solid.h"
#include "switchVisitor.h"

#include <osgGA/EventVisitor>
#include <osg/Geometry>
#include <osg/Geode>
#include <osg/Notify>
#include <osg/PolygonMode>

#include <string>

DebugNode::DebugNode():
_root(NULL), _colorIndex(new osg::Vec4Array), _carDebugGde(new osg::Geode), _carDebugSwitch(new osg::Switch),
_z_deepth(getZDeepth())
{
	for (int i = 0; i <= 1;i++)
	{
		for (int j = 0; j <= 1;j++)
		{
			for (int k = 0; k <= 1;k++)
			{
				_colorIndex->push_back(osg::Vec4(double(i),double(j),double(k),1));
			}
		}
	}
	_i_color = _colorIndex->begin() + 1;
}


DebugNode::~DebugNode()
{
}

osg::ref_ptr<osg::Geometry> DebugNode::addCarDebuggerDrawables(osg::Vec3dArray *refArray)
{
	if (_i_color == _colorIndex->end() - 1)
	{
		_i_color = _colorIndex->begin() + 1;
	}

	osg::ref_ptr<osg::Geometry> carDebugGmt = new osg::Geometry;
	carDebugGmt->setVertexArray(refArray);
	osg::ref_ptr<osg::Vec4Array> color = new osg::Vec4Array;
	color->push_back(*_i_color++);
	carDebugGmt->setColorArray(color);
	carDebugGmt->setColorBinding(osg::Geometry::BIND_OVERALL);
	carDebugGmt->setDataVariance(osg::Object::DYNAMIC);

	if (refArray->size() == 1)
	{
		carDebugGmt->addPrimitiveSet(new osg::DrawArrays(GL_POINTS, 0, refArray->size()));
	}

	else if (refArray->size() == 2)
	{
		carDebugGmt->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, refArray->size()));
	}

	else if (refArray->size() == 3)
	{
		carDebugGmt->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, refArray->size()));
	}

	else if (refArray->size() == 4)
	{
		carDebugGmt->addPrimitiveSet(new osg::DrawArrays(GL_QUADS,0,refArray->size()));
	}

	else
	{
		carDebugGmt->addPrimitiveSet(new osg::DrawArrays(GL_LINES, 0, refArray->size()));
	}

	return carDebugGmt.release();
}

void DebugNode::addCarDebugger()
{
	if (!_root)
	{
		return;
	}

	if (!_carDebugSwitch->getNumParents())
	{
		_root->addChild(_carDebugSwitch);
		_carDebugSwitch->setDataVariance(osg::Object::DYNAMIC);

		return;
	}

	if (!_carDebugGde->getNumParents())
	{
		_carDebugSwitch->addChild(_carDebugGde);
		_carDebugSwitch->setAllChildrenOff();

		_carDebugGde->getOrCreateStateSet()->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
		_carDebugGde->getOrCreateStateSet()->setMode(GL_DEPTH_TEST, osg::StateAttribute::ON);

		std::vector<osg::ref_ptr<osg::Vec3dArray>>::const_iterator i = _carDebugArray.cbegin();
		while (i != _carDebugArray.cend())
		{
			_carDebugGde->addDrawable(addCarDebuggerDrawables(*i));
			i++;
		}

		return;
	}

	osg::Geode::DrawableList drawableList = _carDebugGde->getDrawableList();
	while (!drawableList.empty())
	{
		drawableList.back()->dirtyBound();
		drawableList.back()->dirtyDisplayList();
		drawableList.pop_back();
	}

	_carDebugGde->dirtyBound();
}

void DebugNode::setDeepth(std::vector<osg::ref_ptr<osg::Vec3dArray>> &refVector)
{
	std::vector<osg::ref_ptr<osg::Vec3dArray>>::iterator i = refVector.begin();
	while (i != refVector.end())
	{
		osg::Vec3dArray::iterator j = (*i)->begin();
		while (j != (*i)->end())
		{
			(*j).z() = _z_deepth;
			j++;
		}
		i++;
	}
}

void DebugNode::operator()(osg::Node *node, osg::NodeVisitor *nv)
{
	osgGA::EventVisitor *ev = dynamic_cast<osgGA::EventVisitor*>(nv);
	osgGA::EventQueue::Events events = (ev) ? ev->getEvents() : events;
	osgGA::GUIEventAdapter *ea = (!events.empty()) ? events.front() : NULL;

	const CollVisitor *refCV = dynamic_cast<CollVisitor*>(this->getUserData());
	if (refCV)
	{
		CarState *carState = refCV->getCar()->getCarState();
		if (carState->_updated)
		{
			std::vector<osg::ref_ptr<osg::Vec3dArray>>::iterator i = _carDebugArray.begin();

			(i == _carDebugArray.end()) ? (_carDebugArray.push_back(new osg::Vec3dArray), i = _carDebugArray.end() - 1) : (*i)->clear();
			(*i)->push_back(carState->_O); (*i)->push_back(carState->_O_Project);
			i++;

			(i == _carDebugArray.end()) ? (_carDebugArray.push_back(new osg::Vec3dArray), i = _carDebugArray.end() - 1) : (*i)->clear();
			osg::Vec3d midFront = carState->_frontWheel->front()*0.5f + carState->_frontWheel->back()*0.5f;
			midFront -= carState->_O;
			osg::Vec3d navi = carState->_midLine->front() - carState->_midLine->back();
			navi *= (midFront.length() / navi.length());
			navi += carState->_O;
			(*i)->push_back(carState->_O); (*i)->push_back(navi);
			i++;

			(i == _carDebugArray.end()) ? (_carDebugArray.push_back(new osg::Vec3dArray), i = _carDebugArray.end() - 1) : (*i)->clear();
			osg::Vec3d carD = carState->_direction;
			carD *= (midFront.length()) / (carD.length());
			carD += carState->_O;
			(*i)->push_back(carState->_O); (*i)->push_back(carD);
			i++;

			setDeepth(_carDebugArray);
			addCarDebugger();

			carState->_updated = false;
		}
	}

	_root = dynamic_cast<osg::Group*>(node);
	if (_root && !_road)
	{
		switchRoad swRD;
		_root->accept(swRD);
		_road = swRD.getSwRoad();
	}

	if (ea)
	{
		const int &key = ea->getKey();
		switch (ea->getEventType())
		{
		case osgGA::GUIEventAdapter::KEYDOWN:
			if (key == '1')
			{
				if (!_carDebugSwitch->getNewChildDefaultValue())
				{
					_carDebugSwitch->setAllChildrenOn();
				}
				else
				{
					_carDebugSwitch->setAllChildrenOff();
				}
			}
			else if (key == '2')
			{
				if (_root)
				{
					_root->accept(*(new deTexture));
				}
			}
			else if (key == '3')
			{
				//turn off Road
				if (_road)
				{
					_road->accept(*new SwitchVisitor(ROAD));
				}
			}
			else if (key == '4')
			{
				//turn off rwall
				if (_road)
				{
					_road->accept(*new SwitchVisitor(RWALL));
				}
			}
			else if (key == '5')
			{
				//turn off lwall;
				if (_road)
				{
					_road->accept(*new SwitchVisitor(LWALL));
				}
			}
			else if (key == '6')
			{
				//turn off ctrl
				if (_road)
				{
					_road->accept(*new SwitchVisitor(CTRL));
				}
			}
			else if (key == '7')
			{
				if (_road)
				{
					_road->accept(*new SwitchVisitor(MIDQUAD));
				}
			}
			else if (key == '0')
			{
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
			}
		case osgGA::GUIEventAdapter::FRAME:
			break;
		default:
			break;
		}
	}

	traverse(node, nv);
}

deTexture::deTexture() :
osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
}

deTexture::~deTexture(){};

void deTexture::reset(){};

void deTexture::apply(osg::Group &refNode)
{
	std::string libTemp = refNode.libraryName();
	std::string libClass = refNode.className();
	if (libTemp == "Car" && libClass == "Car")
	{
		traverse(refNode);
		return;
	}

	Solid *refS = dynamic_cast<Solid*>(&refNode);

	if (refS)
	{
		osg::notify(osg::NOTICE) << "Texture caught \t" << refNode.libraryName() << "\t" << refNode.className() << std::endl;
		setTexture(refS);
	}

	traverse(refNode);
}

void deTexture::setTexture(Solid *refS)
{
	if (!refS)
	{
		return;
	}

	osg::Group *refG = refS->asGroup();
	if (!refG)
	{
		return;
	}

	osg::StateSet *ss = refG->getOrCreateStateSet();
	osg::ref_ptr<osg::PolygonMode> pm = dynamic_cast<osg::PolygonMode*>(ss->getAttribute(osg::StateAttribute::POLYGONMODE));
	if (pm)
	{
		osg::PolygonMode::Mode polyMode = pm->getMode(osg::PolygonMode::FRONT_AND_BACK);
		polyMode = (polyMode == osg::PolygonMode::FILL) ? osg::PolygonMode::LINE : osg::PolygonMode::FILL;
		pm->setMode(osg::PolygonMode::FRONT_AND_BACK, polyMode);
	}

	osg::StateAttribute::GLModeValue textureMode = ss->getTextureMode(0, GL_TEXTURE_2D);
	textureMode = (textureMode == osg::StateAttribute::ON) ? osg::StateAttribute::OFF : osg::StateAttribute::ON;
	ss->setTextureMode(0, GL_TEXTURE_2D, textureMode);
}

switchRoad::switchRoad():
osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN)
{
}

switchRoad::~switchRoad()
{
}

void switchRoad::reset()
{
}

void switchRoad::apply(osg::Group &refNode)
{
	std::string libName = refNode.libraryName();
	std::string libClass = refNode.className();
	if (libName == "Road" && libClass == "Road")
	{
		Road *refRd = dynamic_cast<Road*>(&refNode);
		if (refRd)
		{
			_road = refRd->asSwitch();
		}
	}

	traverse(refNode);
}