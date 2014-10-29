#pragma once
#include "plane.h"
#include "loop.h"

#include <osg/Switch>
#include <osg/Geometry>

class Points;
class Edge;

class Solid:public osg::Switch
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

	inline void setTexMode(bool mode){ _texMode = mode; };
	inline bool getTexMode() const { return _texMode; };

	Plane * getAbstract() const;

	void addPlanetoList(Plane *refPL);
	void addEdgetoList(Edge *refE);
	void addPointtoList(Points *refP);

	void traverse();
	bool isValid();
	Points * findPoint(osg::Vec3d ref);
	Edge * findEdge(const osg::Vec3d &p1, const osg::Vec3d &p2);
	Loop * findLoop(const Edge *e1, const Edge *e2);
	Loop * findLoop(const osg::Vec3d &p1, const osg::Vec3d &p2);

	inline void setTexCoord(osg::ref_ptr<osg::Vec2Array> refT) { _texCoord = refT; if (_texCoord) _texMode = true; };
	inline const osg::ref_ptr<osg::Vec2Array> getTexCoord() const { return _texCoord; };
	inline const osg::ref_ptr<osg::Image> getImageTexture() const { return _imgTexture; };

	inline void setIndex(const unsigned ref){ _index = ref; };
	inline unsigned getIndex() const { return _index; };

	inline bool getCCW() const { if(!_ccwupdated) caclCCW(); return _ccw; };

	bool ifPointinSolid(const osg::Vec3d p) const;

	void caclCCW() const;

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

	struct absoluteTerritory
	{
		absoluteTerritory()
		{
			_detectR = 0.0f;
			_refuseR = 0.0f;
		}
		osg::Vec3d center;
		double _detectR;
		double _refuseR;
	}absoluteTerritory;

protected:
	~Solid();
	osg::ref_ptr<osg::Vec2Array> _texCoord;
	osg::ref_ptr<osg::Image> _imgTexture;

private:
	bool _updated;
	mutable bool _ccw;
	mutable bool _ccwupdated;

	Plane *_startPlane;
	Edge* _startE;
	Points* _startP;
	Plane *_lastPlane;
	unsigned _numPoints;
	unsigned _numPlanes;

	osg::ref_ptr<Solid> _next;
	osg::ref_ptr<Solid> _prev;

	unsigned _index;

	bool _texMode;
};