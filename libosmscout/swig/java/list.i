%{
#include <list>
%}

%inline %{

  template <typename T> class Iterator_t {
   public:
      Iterator_t(const std::list<T>& m) : it(m.begin()), list(m) {}
      bool hasNext() const {
	return it != list.end();
      }

      const T& next() {
	// TODO throw java exception when at end
	return *it++;
      }

     void remove() {
      }

    private:
    typename std::list<T>::const_iterator it;
    const std::list<T>& list;
  };
%}

namespace std {
  template<class T> class list {
  public:
    typedef size_t size_type;
    typedef T value_type;
    typedef const value_type& const_reference;
    list();

    // -- seems not to work with classeswithout default constructor
    //list(size_type n);
    size_type size() const;
    %rename(isEmpty) empty;
    bool empty() const;
    void clear();
    //void reverse();
    %rename(prepend) push_front;
    void push_front(const value_type& x);
    %rename(add) push_back;
    void push_back(const value_type& x);
    %rename(first) front;
    const_reference front();
    %rename(last) back;
    const_reference back();
    %rename(popLast) pop_back;
    void pop_back();
    %rename(popFirst) pop_front;
    void pop_front();

    %extend {
      const Iterator_t<T>& iterator(){
	return *(new Iterator_t<T>(*($self)));
      }
    };

  };
}
