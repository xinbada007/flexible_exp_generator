#include "stdafx.h"
#include "plane.h"
#include "loop.h"
#include "solid.h"

Plane::Plane():
_startLoop(NULL), _prev(NULL), _next(NULL), _homeS(NULL), _updated(false), _abstract(false), _index(0)
{

}

Plane::Plane(Loop *refL) :
_startLoop(NULL), _prev(NULL), _next(NULL), _homeS(NULL), _updated(false), _abstract(false), _index(0)
{
	addLooptoList(refL);
}

Plane::Plane(const Plane &copy, osg::CopyOp copyop /* = osg::CopyOp::SHALLOW_COPY */):
osg::Geode(copy, copyop), _startLoop(copy.getLoop()),
_prev(copy._prev), _next(copy._next), _homeS(copy._homeS), _updated(copy._updated), _abstract(copy._abstract),
_index(copy._index)
{

}

Plane::~Plane()
{
	osg::notify(osg::NOTICE) << "deleting Plane..." << std::endl;
}

void Plane::addLooptoList(Loop *refL)
{
	refL->setNext(_startLoop);
	refL->setPrev(NULL);

	if (_startLoop)
	{
		_startLoop->setPrev(refL);
	}
	_startLoop = refL;
	refL->setHomeP(this);
}

void Plane::traverse()
{
	if (_updated)
	{
		return;
	}

	Loop *refL = _startLoop;

	do 
	{
		this->addDrawable(refL->asGeometry());
		refL->draw();
		refL = refL->getNext();
	} while (refL);

	_updated = true;
}

bool Plane::isValid() const
{
	if ((!_startLoop) || (!_homeS))
	{
		osg::notify(osg::FATAL) << "Plane is not valid" << std::endl;
		return false;
	}
	if (_startLoop->getHomeP() != this)
	{
		osg::notify(osg::FATAL) << "Plane and Loop is inconsistent!" << std::endl;
		return false;
	}

	Loop *refL = _startLoop;
	bool valid(false);
	do 
	{
		valid = _startLoop->isValid();
		refL = refL->getNext();
	} while (valid && refL);

	return valid;
}

Plane::reverse_across_iterator::reverse_across_iterator(Plane *ref):
_value(ref), _BACKWARD(0), _FORWARD(1)
{
	_value = isValid() ? _value : NULL;
	_solid = (_value) ? _value->getHomeS() : NULL;
}

Plane::reverse_across_iterator & Plane::reverse_across_iterator::operator--()
{
	_value = isValid() ? _value->getNext() : NULL;
	_value = isValid() ? _value : NULL;

	across(_BACKWARD);

	return *this;
}

Plane::reverse_across_iterator & Plane::reverse_across_iterator::operator++()
{
	_value = isValid() ? _value->getPrev() : NULL;
	_value = isValid() ? _value : NULL;

	across(_FORWARD);

	return *this;
}

Plane::reverse_across_iterator Plane::reverse_across_iterator::operator--(int)
{
	reverse_across_iterator ret(_value);
	--(*this);
	return ret;
}

Plane::reverse_across_iterator Plane::reverse_across_iterator::operator++(int)
{
	reverse_across_iterator ret(_value);
	++(*this);
	return ret;
}

Plane::reverse_across_iterator & Plane::reverse_across_iterator::operator=(Plane *ref)
{
	_value = ref;
	return (*this);
}

Plane * Plane::reverse_across_iterator::operator*()
{
	return _value;
}

bool Plane::reverse_across_iterator::isValid()
{
	if (!_value || _value->getAbstract())
		return false;
	return true;
}

void Plane::reverse_across_iterator::across(unsigned flag)
{
	if (!isValid())
	{
		if (flag == _BACKWARD)
		{
			_solid = _solid->getPrev();
			if (_solid)
			{
				_value = _solid->getPlane();
			}
		}
		else
		{
			_solid = _solid->getNext();
			if (_solid)
			{
				_value = _solid->getLastPlane();
			}
		}
	}
}

Plane::reverse_across_iterator & Plane::reverse_across_iterator::operator=(Plane::reverse_across_iterator &ref)
{
	_value = ref._value;
	_solid = ref._solid;

	return (*this);
}

Plane::reverse_across_iterator & Plane::reverse_across_iterator::operator+=(int ref)
{
	Plane::reverse_across_iterator ret_prv = (*this);
	for (int i = 0; i < ref;i++)
	{
		++(*this);
		if (!this->isValid())
		{
			(*this) = ret_prv;
			return (*this);
		}
		ret_prv = (*this);
	}

	return (*this);
}

Plane::reverse_across_iterator & Plane::reverse_across_iterator::operator-=(int ref)
{
	Plane::reverse_across_iterator ret_prv = (*this);
	for (int i = 0; i < ref;i++)
	{
		--(*this);
		if (!this->isValid())
		{
			(*this) = ret_prv;
			return (*this);
		}
		ret_prv = (*this);
	}

	return (*this);
}

Plane::reverse_across_iterator Plane::reverse_across_iterator::operator+(int ref)
{
	Plane::reverse_across_iterator ret = (*this);
	Plane::reverse_across_iterator ret_prv = ret;
	for (int i = 0; i < ref;i++)
	{
		++(ret);
		if (!ret.isValid())
		{
			return ret_prv;
		}
		ret_prv = ret;
	}

	return ret;
}

Plane::reverse_across_iterator Plane::reverse_across_iterator::operator-(int ref)
{
	Plane::reverse_across_iterator ret = (*this);
	Plane::reverse_across_iterator ret_prv = ret;
	for (int i = 0; i < ref; i++)
	{
		--(ret);
		if (!ret.isValid())
		{
			return ret_prv;
		}
		ret_prv = ret;
	}

	return ret;
}

Plane::reverse_iterator & Plane::reverse_iterator::operator--()
{
	_value = isValid() ? _value->getNext() : NULL;
	_value = isValid() ? _value : NULL;
	return *this;
}

Plane::reverse_iterator & Plane::reverse_iterator::operator++()
{
	_value = isValid() ? _value->getPrev() : NULL;
	_value = isValid() ? _value : NULL;
	return *this;
}

Plane::reverse_iterator & Plane::reverse_iterator::operator=(Plane *ref)
{
	_value = ref;
	return (*this);
}

Plane::reverse_iterator & Plane::reverse_iterator::operator=(reverse_iterator &ref)
{
	_value = ref._value;

	return (*this);
}

bool Plane::reverse_iterator::operator!=(reverse_iterator &ref)
{
	return !(_value == ref._value);
}

Plane::reverse_iterator Plane::reverse_iterator::operator--(int)
{
	Plane::reverse_iterator ret(_value);
	--(*this);
	return ret;
}

Plane::reverse_iterator Plane::reverse_iterator::operator++(int)
{
	Plane::reverse_iterator ret(_value);
	++(*this);
	return ret;
}

Plane * Plane::reverse_iterator::operator*()
{
	return _value;
}

bool Plane::reverse_iterator::isValid()
{
	if (!_value || _value->getAbstract())
		return false;
	return true;
}