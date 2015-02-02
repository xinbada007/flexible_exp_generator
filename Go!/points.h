#pragma once
#include <osg/Vec3d>
#include <osg/Referenced>
class HalfEdge;

class Points: public osg::Referenced
{
public:
	Points(osg::Vec3d newP);
	~Points();

	inline Points * getNext() const { return _next; };
	inline void setNext(Points *refP) { _next = refP; };
	inline Points * getPrev() const { return _prev; };
	inline void setPrev(Points *refP) { _prev = refP; };
	inline osg::Vec3d getPoint() const { return _p; };
	inline void setPoint(osg::Vec3d ref) { _p = ref; };
	inline HalfEdge * getHE() const { return _homeHE; };
	inline void setHE(HalfEdge *ref) { _homeHE = ref; };
	inline unsigned getIndex() const { return _index; };
	inline void setIndex(unsigned ref) { _index = ref; };

	bool isEqual(osg::Vec3d) const;
	bool isValid() const { return (_homeHE) ? true : false; };

private:
	//variables
	osg::Vec3d _p;
	unsigned _index;
	HalfEdge *_homeHE;
	Points *_next;
	Points *_prev;
};

