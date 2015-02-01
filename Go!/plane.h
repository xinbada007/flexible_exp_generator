#pragma once
class Solid;
class Loop;

#include <osg/geode>
#include <osg/Matrix>

class Plane: public osg::Geode
{
public:
	Plane();
	Plane(Loop *refL);
	Plane(const Plane &copy, osg::CopyOp copyop = osg::CopyOp::SHALLOW_COPY);
	~Plane();

	struct iterator :
		public std::iterator < std::input_iterator_tag, Plane >
	{
		iterator(){};
		iterator(Plane *ref) :_value(ref){ _value = isValid() ? _value : NULL; };
		iterator(const iterator &ref) :_value(ref._value){ _value = isValid() ? _value : NULL; };
		~iterator(){};

		iterator & operator++()
		{
			_value = isValid() ? _value->getNext() : NULL;
			_value = isValid() ? _value : NULL;
			return *this;
		}
		iterator operator++(int)
		{
			iterator ret(_value);
			++(*this);
			return ret;
		}

		iterator & operator--()
		{
			_value = isValid() ? _value->getPrev() : NULL;
			_value = isValid() ? _value : NULL;
			return *this;
		}
		iterator operator--(int)
		{
			iterator ret(_value);
			--(*this);
			return ret;
		}

		bool operator==(const iterator &ref)
		{
			return (ref._value == _value);
		}
		bool operator!=(iterator &ref)
		{
			return !(ref._value == _value);
		}

		iterator & operator=(Plane *ref)
		{
			_value = ref;
			return (*this);
		}
		iterator &operator=(const iterator &ref)
		{
			_value = ref._value;
			return (*this);
		}

		Plane * operator*()
		{
			return _value;
		}

	private:
		Plane *_value;
		bool isValid()
		{
			if (!_value || _value->getAbstract())
				return false;
			return true;
		}
	};

	struct reverse_iterator :
		public std::iterator < std::input_iterator_tag, Plane > 
	{
		reverse_iterator(){};
		reverse_iterator(Plane *ref) :_value(ref){ _value = isValid() ? _value : NULL; };
		reverse_iterator(const reverse_iterator &ref) :_value(ref._value){
			_value = isValid() ? _value : NULL;
		}
		~reverse_iterator(){};

		reverse_iterator & operator++();
		reverse_iterator operator++(int);
		reverse_iterator & operator--();
		reverse_iterator operator--(int);

		bool operator==(const reverse_iterator &ref);
		bool operator!=(const reverse_iterator &ref);
		
		reverse_iterator & operator=(Plane *ref);
		reverse_iterator & operator=(const reverse_iterator &ref);
		
		Plane * operator*();

	private:
		Plane *_value;
		bool isValid();
	};

	struct reverse_across_iterator
	{
		reverse_across_iterator() :_BACKWARD(0), _FORWARD(1), _value(NULL), _solid(NULL) {};
		reverse_across_iterator(const reverse_across_iterator &ref);
		reverse_across_iterator(Plane *ref);
		~reverse_across_iterator(){};

		reverse_across_iterator & operator++();
		reverse_across_iterator operator++(int);
		reverse_across_iterator & operator--();
		reverse_across_iterator operator--(int);

		bool operator==(const reverse_across_iterator &ref);
		bool operator!=(const reverse_across_iterator &ref);

		reverse_across_iterator & operator=(Plane *ref);
		reverse_across_iterator & operator=(const reverse_across_iterator &ref);

		reverse_across_iterator & operator+=(int);
		reverse_across_iterator operator+(int);

		reverse_across_iterator & operator-=(int);		
		reverse_across_iterator operator-(int);
		
		void add(int);
		
		Plane * operator*();

	private:
		Plane *_value;
		Solid *_solid;

		const unsigned _FORWARD;
		const unsigned _BACKWARD;
		bool isValid();
		void across(unsigned flag);
	};

	META_Node(BP, Plane);
	inline Loop * getLoop() const { return _startLoop; };
	inline void setLoop(Loop *ref) { _startLoop = ref; };
	inline Plane * getNext() const { return _next; };
	inline void setNext(Plane *ref){ _next = ref; };
	inline Plane * getPrev() const { return _prev; };
	inline void setPrev(Plane *ref){ _prev = ref; };
	inline Solid * getHomeS() const { return _homeS; };
	inline void setHomeS(Solid *ref) { _homeS = ref; };
	inline bool getUpdated() const { return _updated; };
	inline bool setUpdated(bool ref) { _updated = ref; };
	inline bool getAbstract() const { return _abstract; };
	inline void setAbstract(bool ref) { _abstract = ref; };
	inline void setIndex(const unsigned ref){ _index = ref; };
	inline unsigned getIndex() const { return _index; };

	bool isValid() const;

	void multiplyMatrix(const osg::Matrixd &m);

	void traverse();
private:
	void addLooptoList(Loop *refL);
	Loop *_startLoop;
	Plane *_next;
	Plane *_prev;
	Solid *_homeS;
	bool _updated;
	bool _abstract;

	unsigned _index;
};