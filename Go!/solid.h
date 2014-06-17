#pragma once
#include "plane.h"
#include "loop.h"

#include <osg/Group>
#include <osg/Geometry>

class Points;
class Edge;

class Solid:public osg::Group
{
public:
	Solid();
	Solid(const Solid &copy, osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY);

	META_Node(BP, Solid);
	inline Plane * getPlane() const { return _startPlane; };
//	inline void setPlane(Plane *ref) { _startPlane = ref; };
	inline Plane *getLastPlane() const { return (_lastPlane->getAbstract())?_lastPlane->getPrev():_lastPlane; };
	inline Edge * getEdge() const { return _startE; };
//	inline void setEdge(Edge *refE) { _startE = refE; };
	inline Points * getPoint() const { return _startP; };
//	inline void setPoint(Points *refP) { _startP = refP; };

	inline Solid * getNext() const { return _next; };
	inline void setNext(Solid *ref) { _next = ref; };
	inline Solid * getPrev() const { return _prev; };
	inline void setPrev(Solid *ref) { _prev = ref; };
	inline bool getUpdated() const { return _updated; };
	inline void setUpdated(bool ref) { _updated = ref; };
	inline unsigned getNumPoints() const { return _numPoints; };
//	inline void setNumPoints(unsigned ref) { _numPoints = ref; };
	inline unsigned getNumGeometry() const { return this->getNumChildren(); };
	inline osg::ref_ptr<osg::Geometry> getGeometry(const unsigned index) const 
	{
		if (index >= _numPlanes)
		{
			return NULL;
		}
		const Plane *p(_startPlane);
		for (unsigned i(0); i != index; i++) p = p->getNext();
		return p->getLoop()->asGeometry();
	}
	inline unsigned getNumPlanes() const { return _numPlanes; };
//	inline void setNumPlanes(unsigned ref) { _numPlanes = ref; };

	Plane * getAbstract() const;

	void addPlanetoList(Plane *refPL);
	void addEdgetoList(Edge *refE);
	void addPointtoList(Points *refP);

	void traverse();
	bool isValid();
	Points * findPoint(osg::Vec3d ref);

	inline void setTexCoord(osg::ref_ptr<osg::Vec2Array> refT) { _texCoord = refT; };
	inline const osg::ref_ptr<osg::Vec2Array> getTexCoord() const { return _texCoord; };
	inline const osg::ref_ptr<osg::Image> getImageTexture() const { return _imgTexture; };

	inline void setIndex(const unsigned ref){ _index = ref; };
	inline unsigned getIndex() const { return _index; };

	struct iterator
	{
		iterator(){};
		iterator(Solid *ref) :_value(ref){};
		~iterator(){};

		iterator & operator++()
		{
			_value = (_value) ? _value->getNext() : NULL;
			return *this;
		}

		iterator & operator--()
		{
			_value = (_value) ? _value->getPrev() : NULL;
			return *this;
		}

		iterator operator++(int)
		{
			iterator ret(_value);
			++(*this);
			return ret;
		}

		iterator operator--(int)
		{
			iterator ret(_value);
			--(*this);
			return ret;
		}

		bool operator=(Solid *ref)
		{
			_value = ref;
		}

		Solid * operator*()
		{
			return _value;
		}
	private:
		Solid *_value;
	};

protected:
	~Solid();
	osg::ref_ptr<osg::Vec2Array> _texCoord;
	osg::ref_ptr<osg::Image> _imgTexture;

private:
	bool _updated;

	Plane *_startPlane;
	Edge* _startE;
	Points* _startP;
	Plane *_lastPlane;
	unsigned _numPoints;
	unsigned _numPlanes;

	osg::ref_ptr<Solid> _next;
	osg::ref_ptr<Solid> _prev;

	unsigned _index;
};