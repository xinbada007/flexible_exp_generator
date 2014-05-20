#pragma once
#include <osg/Vec3d>
#include <osg/Referenced>
#include "edge.h"

class Points;
class Loop;

class HalfEdge:public osg::Referenced
{
public:
	HalfEdge(Points *newP);
	~HalfEdge();

	inline HalfEdge * operator++() const
	{
		return _next;
	}

	inline Points * getPoint() const { return _startP; };
	inline void setPoint(Points *ref) { _startP = ref; };
	inline HalfEdge * getNext() const { return _next; };
	inline void setNext(HalfEdge *ref) { _next = ref; };
	inline HalfEdge * getPrev() const { return _prev; };
	inline void setPrev(HalfEdge *ref) { _prev = ref; };
	inline Loop * getLoop() const { return _homeL; };
	inline void setLoop(Loop * ref) { _homeL = ref; };
	inline Edge * getEdge() const { return _homeE; };
	inline void setEdge(Edge *ref) { _homeE = ref; };
	inline HalfEdge *getMate() const { return (this == _homeE->getHE1()) ? _homeE->getHE2() : _homeE->getHE1(); };

	bool isValid() const;
private:
	Points *_startP;
	Loop *_homeL;
	Edge *_homeE;
	HalfEdge *_next;
	HalfEdge *_prev;
};