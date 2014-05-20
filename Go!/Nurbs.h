/* -*-c++-*- osgModeling - Copyright (C) 2008 Wang Rui <wangray84@gmail.com>
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public
* License as published by the Free Software Foundation; either
* version 2.1 of the License, or (at your option) any later version.

* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
* Lesser General Public License for more details.

* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef OSGMODELING_NURBS
#define OSGMODELING_NURBS 1

#include <osg/Vec3d>
#include <osg/Array>
#include <osg/Referenced>

/** NURBS curve class
 * Create k-degree Non-uniform rational B-splines by inputing a control points array and a knot vector.
 * There are 2 algorithms to generate a curve at present:
 * - The Cox-de Boor recursive polynomials.
 * - The de Boor recursive method, as a generalization of de Casteljau's, used by default.
 */
class NurbsCurve:public osg::Referenced
{
public:
	NurbsCurve();

	/** Specifies parameters in constructor & no need to call update(). Provided for convenience. */
	NurbsCurve( osg::Vec3dArray* pts, osg::DoubleArray* weights, osg::DoubleArray* knots,
		unsigned int degree=3, unsigned int numPath=20 );

	/** Specifies parameters like gluNurbsCurve & no need to call update(). Provided for convenience.
	 * The size of control points is determined by 'kcount-order'.
	 * \param kcount Size of the knot vector.
	 * \param stride Can be 2, 3 & 4, Vec3d data will be created for control points, and the 4th is considered as weight.
	 * \param order Order equals degree + 1.
	 */
	NurbsCurve( unsigned int kcount, double* knotPtr, unsigned int stride, double* ctrlPtr,
		unsigned int order, unsigned int numPath=20 );

	/** Set a method to generate NURBS curve.
	 * There are 2 algorithms to generate a curve at present:
	 * - 0: The Cox-de Boor recursive polynomials.
	 * - 1: The de Boor recursive method, as a generalization of de Casteljau's, used by default.
	 */
	inline void setMethod( int m ) { _method=m; }
	inline int getMethod() { return _method; }

	/** Specifies a vertex list as the defining polygon vertices. */
	inline void setCtrlPoints( osg::Vec3dArray* pts )
	{
		_ctrlPts = pts;
		if (_updated) _updated=false;
	}
	inline osg::Vec3dArray* getCtrlPoints() { return _ctrlPts.get(); }
	inline const osg::Vec3dArray* getCtrlPoints() const { return _ctrlPts.get(); }

	/** Specifies a value list as the knots vector.
	 * The knots vector is used to control the distribution of B-spline.
	 * The vector size is n+k+1 (n is size of control-points), and ascending with multiplicity.
	 * Values of knot elements alter positions of each basis functions.
	 * If not set, a default vector will be created.
	 */
	inline void setKnotVector( osg::DoubleArray* pts )
	{
		if ( _knots!=pts ) _knots = pts;
		if (_updated) _updated=false;
	}
	inline osg::DoubleArray* getKnotVector() { return _knots.get(); }
	inline const osg::DoubleArray* getKnotVector() const { return _knots.get(); }

	/** Specifies weights of each control points. All to 1.0 if not set. */
	inline void setWeights( osg::DoubleArray* pts )
	{
		if ( _weights!=pts ) _weights = pts;
		if (_updated) _updated=false;
	}
	inline osg::DoubleArray* getWeights() { return _weights.get(); }
	inline const osg::DoubleArray* getWeights() const { return _weights.get(); }

	/** Specifies the degree of the curve. Default is 2. */
	inline void setDegree( unsigned int k ) { _degree=k; if (_updated) _updated=false; }
	inline unsigned int getDegree() { return _degree; }

	/** Specifies number of vertices on the curve path. */
	inline void setNumPath( unsigned int num ) { _numPath=num; if (_updated) _updated=false; }
	inline unsigned int getNumPath() { return _numPath; }

	virtual void updateImplementation();

	/* This helps generate a knots vector for a k-degree curve with specified control points. */
	static osg::DoubleArray* generateKnots( unsigned int k, unsigned int numCtrl );
	
	inline void update() {updateImplementation();_updated=true;}

	inline osg::Vec3dArray *getPath() const { return _PathArray.get(); };
protected:
	virtual ~NurbsCurve();

	void useCoxDeBoor( osg::Vec3dArray* result );
	void useDeBoor( osg::Vec3dArray* result );

	osg::Vec4 lerpRecursion( unsigned int k, unsigned int r, unsigned int i, double u );
	void coxDeBoor( osg::DoubleArray* basis, int m, int num, double u );
	
	inline void setPath(osg::Vec3dArray *pathArray) {_PathArray = pathArray;}
	template<typename T>
	inline T lerp( const T& a, const T& b, double u ) { return a*(1.0f-u)+b*u; }

	int _method;

	osg::ref_ptr<osg::Vec3dArray> _ctrlPts;
	osg::ref_ptr<osg::DoubleArray> _knots;
	osg::ref_ptr<osg::DoubleArray> _weights;
	unsigned int _degree;
	unsigned int _numPath;
	
	osg::ref_ptr<osg::Vec3dArray> _PathArray;
	bool _updated;
};

#endif
