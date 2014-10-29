#pragma once
#include "math.h"

#include <osg/Geometry>

class HalfEdge;
class Plane;
class Edge;

typedef std::vector<Edge*> edgelist;

class Loop:public osg::Geometry
{
public:
	Loop();
	Loop(const Loop &copy, osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY);
	~Loop();

	META_Object(BP, Loop);
	inline HalfEdge * getHE() const { return _startHE; };
	inline void setHE(HalfEdge *ref) { _startHE = ref; };
	inline Loop * getPrev() const { return _prev; };
	inline void setPrev(Loop *ref) { _prev = ref; };
	inline Loop * getNext() const { return _next; };
	inline void setNext(Loop *ref) { _next = ref; };
	inline Plane * getHomeP() const { return _homeP; };
	inline void setHomeP(Plane *ref) { _homeP = ref; };
	inline bool getSwitch() const { return _switch; };
	inline void setSwitch(bool ref) { _switch = ref; };
	inline unsigned getnumHalfEdges() const { return _numHE; };

	const edgelist getFlagEdge() const;
	const osg::ref_ptr<osg::Vec3dArray> getNavigationEdge() const;

	bool isValid() const;

	void addHEtoLoop(Edge *refE);
	void splitLoop(Edge *refE, Loop *newL);
	void draw();

	void setPlaneEQU(planeEQU ref) { _planeEQU = ref; };
	planeEQU getPlaneEQU() const { return _planeEQU; };
	inline double substituePointinEQU(const osg::Vec3d p) { return _planeEQU[0] * p.x() + _planeEQU[1] * p.y() + _planeEQU[2] * p.z() + _planeEQU[3]; };

	const osg::ref_ptr<osg::Vec3dArray> getPoints();
	
	bool ifPointinLoop(const osg::Vec3d &p);

	void setTexCoord(unsigned int unit, osg::Array* array, osg::Array::Binding binding = osg::Array::BIND_UNDEFINED);

private:
	void forwardInsertHE(HalfEdge *he1, HalfEdge *he2);
	HalfEdge *_startHE;
	Loop *_prev;
	Loop *_next;
	Plane *_homeP;
	
	planeEQU _planeEQU;
	bool _switch;

	unsigned _numHE;
};

