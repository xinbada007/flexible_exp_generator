#pragma once
#include <osg/Array>
#include <osg/Referenced>

#include <iostream>

class HalfEdge;

typedef struct edgeFlag:
public osg::Referenced
{
	edgeFlag()
	{
		_collsionEdge = false;
		_navigateEdge = false;
		_navigationArray = NULL;
	};

	osg::ref_ptr<osg::Vec3dArray> _navigationArray;
	bool _collsionEdge;
	bool _navigateEdge;

protected:
	~edgeFlag(){ /*std::cout << "Deconstruct EdgeFlag" << std::endl;*/ };
}edgeFlag;

class Edge
{
public:
	Edge();
	~Edge();

	inline HalfEdge *getHE1() const { return _he1; };
	inline HalfEdge *getHE2() const { return _he2; };
	inline void setHE1(HalfEdge *ref){ _he1 = ref; };
	inline void setHE2(HalfEdge *ref){ _he2 = ref; };
	inline Edge *getNext() const { return _next; };
	inline void setNext(Edge *ref) { _next = ref; };
	inline Edge *getPrev() const { return _prev; };
	inline void setPrev(Edge *ref) { _prev = ref; };

	void addHEtoEdge(HalfEdge *refHE1,HalfEdge *refHE2);

	bool isValid() const;

	inline edgeFlag *  getEdgeFlag()
	{
		return _eFlag.get();
	};
	inline void setEdgeFlag(osg::ref_ptr<edgeFlag> ref) { _eFlag = ref; };
private:
	HalfEdge *_he1;
	HalfEdge *_he2;

	Edge *_next;
	Edge *_prev;

	osg::ref_ptr<edgeFlag> _eFlag;
};