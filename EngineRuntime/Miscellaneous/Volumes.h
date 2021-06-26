#pragma once

#include "DynamicString.h"

namespace Engine
{
	namespace Volumes
	{
		template<class E> class List : public Object
		{
		public:
			class Element
			{
				friend class Engine::Volumes::List<E>;
				Element * _prev, * _next;
				E _value;
				Element(const E & src) : _prev(0), _next(0), _value(src) {}
			public:
				~Element(void) {}
				const Element * GetNext(void) const noexcept { return _next; }
				Element * GetNext(void) noexcept { return _next; }
				const Element * GetPrevious(void) const noexcept { return _prev; }
				Element * GetPrevious(void) noexcept { return _prev; }
				const E & GetValue(void) const noexcept { return _value; }
				E & GetValue(void) noexcept { return _value; }
			};
			template<class I> class ForwardIterator
			{
				I * _element;
			public:
				ForwardIterator(I * element) : _element(element) {}
				auto & operator * (void) const { return _element->GetValue(); }
				auto & operator * (void) { return _element->GetValue(); }
				auto & operator -> (void) const { return _element->GetValue(); }
				auto & operator -> (void) { return _element->GetValue(); }
				bool operator == (const ForwardIterator & b) const { return _element == b._element; }
				bool operator != (const ForwardIterator & b) const { return _element != b._element; }
				ForwardIterator & operator ++ (void) { _element = _element->_next; return *this; }
				ForwardIterator operator ++ (int) { ForwardIterator result(_element->_next); _element = _element->_next; return result; }
			};
			template<class I> class BackwardIterator
			{
				I * _element;
			public:
				BackwardIterator(I * element) : _element(element) {}
				auto & operator * (void) const { return _element->GetValue(); }
				auto & operator * (void) { return _element->GetValue(); }
				auto & operator -> (void) const { return _element->GetValue(); }
				auto & operator -> (void) { return _element->GetValue(); }
				bool operator == (const BackwardIterator & b) const { return _element == b._element; }
				bool operator != (const BackwardIterator & b) const { return _element != b._element; }
				BackwardIterator & operator ++ (void) { _element = _element->_prev; return *this; }
				BackwardIterator operator ++ (int) { BackwardIterator result(_element->_prev); _element = _element->_prev; return result; }
			};
			template<class L, class I> class ForwardEnumerator
			{
				L * _list;
			public:
				ForwardEnumerator(L * list) : _list(list) {}
				ForwardIterator<I> begin(void) const noexcept { return ForwardIterator<I>(_list->GetFirst()); }
				ForwardIterator<I> end(void) const noexcept { return ForwardIterator<I>(0); }
			};
			template<class L, class I> class BackwardEnumerator
			{
				L * _list;
			public:
				BackwardEnumerator(L * list) : _list(list) {}
				BackwardIterator<I> begin(void) const noexcept { return BackwardIterator<I>(_list->GetLast()); }
				BackwardIterator<I> end(void) const noexcept { return BackwardIterator<I>(0); }
			};
		private:
			Element * _first, * _last;
		public:
			List(void) : _first(0), _last(0) {}
			List(const List & src) : _first(0), _last(0) { auto e = src._first; while (e) { InsertLast(e->_value); e = e->_next; } }
			List(List && src) { _first = src._first; _last = src._last; src._first = src._last = 0; }
			virtual ~List(void) override { auto e = _first; while (e) { auto nx = e->_next; delete e; e = nx; } }
			List & operator = (const List & src)
			{
				if (&src == this) return *this;
				auto e = _first;
				while (e) { auto nx = e->_next; delete e; e = nx; }
				_first = _last = 0;
				e = src._first;
				while (e) { InsertLast(e->_value); e = e->_next; }
				return *this;
			}
			virtual ImmutableString GetListType(void) const { return L"List"; }
			virtual ImmutableString ToString(void) const override
			{
				DynamicString result;
				result << GetListType();
				result << L" : [";
				auto e = _first;
				while (e) {
					result << GetStringRepresentation(e->_value);
					if (e->_next) result << L", ";
					e = e->_next;
				}
				result << L"]";
				return result.ToString();
			}

			const Element * GetFirst(void) const noexcept { return _first; }
			Element * GetFirst(void) noexcept { return _first; }
			const Element * GetLast(void) const noexcept { return _last; }
			Element * GetLast(void) noexcept { return _last; }
			bool IsEmpty(void) const noexcept { return !_first; }

			Element * InsertFirst(const E & value)
			{
				auto element = new (std::nothrow) Element(value);
				if (!element) throw OutOfMemoryException();
				element->_prev = 0;
				element->_next = _first;
				if (_first) {
					_first->_prev = element;
					_first = element;
				} else _last = _first = element;
				return element;
			}
			Element * InsertLast(const E & value)
			{
				auto element = new (std::nothrow) Element(value);
				if (!element) throw OutOfMemoryException();
				element->_next = 0;
				element->_prev = _last;
				if (_last) {
					_last->_next = element;
					_last = element;
				} else _last = _first = element;
				return element;
			}
			Element * InsertAfter(Element * relative, const E & value)
			{
				if (!relative) throw InvalidArgumentException();
				if (!relative->_next) return InsertLast(value);
				auto element = new (std::nothrow) Element(value);
				if (!element) throw OutOfMemoryException();
				element->_prev = relative;
				element->_next = relative->_next;
				element->_prev->_next = element;
				element->_next->_prev = element;
				return element;
			}
			Element * InsertBefore(Element * relative, const E & value)
			{
				if (!relative) throw InvalidArgumentException();
				if (!relative->_prev) return InsertFirst(value);
				auto element = new (std::nothrow) Element(value);
				if (!element) throw OutOfMemoryException();
				element->_prev = relative->_prev;
				element->_next = relative;
				element->_prev->_next = element;
				element->_next->_prev = element;
				return element;
			}
			
			void Remove(Element * element) noexcept
			{
				if (!element) return;
				if (element->_next) element->_next->_prev = element->_prev; else _last = element->_prev;
				if (element->_prev) element->_prev->_next = element->_next; else _first = element->_next;
				delete element;
			}
			void RemoveFirst(void) noexcept { Remove(_first); }
			void RemoveLast(void) noexcept { Remove(_last); }
			void Clear(void) noexcept
			{
				auto e = _first;
				while (e) {
					auto nx = e->_next;
					delete e;
					e = nx;
				}
				_first = _last = 0;
			}

			int Count(void) const noexcept
			{
				auto e = _first;
				int result = 0;
				while (e) { e = e->_next; result++; }
				return result;
			}
			const Element * ElementAt(int index) const noexcept
			{
				auto e = _first;
				while (e && index > 0) { index--; e = e->_next; }
				if (!index) return e; else return 0;
			}
			Element * ElementAt(int index) noexcept
			{
				auto e = _first;
				while (e && index > 0) { index--; e = e->_next; }
				if (!index) return e; else return 0;
			}

			ForwardEnumerator< List<E>, Element > Elements(void) noexcept { return ForwardEnumerator< List<E>, Element >(this); }
			ForwardEnumerator< const List<E>, const Element > Elements(void) const noexcept { return ForwardEnumerator< const List<E>, const Element >(this); }
			BackwardEnumerator< List<E>, Element > InversedElements(void) noexcept { return BackwardEnumerator< List<E>, Element >(this); }
			BackwardEnumerator< const List<E>, const Element > InversedElements(void) const noexcept { return BackwardEnumerator< const List<E>, const Element >(this); }
			ForwardIterator< Element > begin(void) noexcept { return ForwardIterator< Element >(_first); }
			ForwardIterator< const Element > begin(void) const noexcept { return ForwardIterator< const Element >(_first); }
			ForwardIterator< Element > end(void) noexcept { return ForwardIterator< Element >(0); }
			ForwardIterator< const Element > end(void) const noexcept { return ForwardIterator< const Element >(0); }

			const E & operator [] (int index) const
			{
				auto element = ElementAt(index);
				if (!element) throw InvalidArgumentException();
				return element->GetValue();
			}
			E & operator [] (int index)
			{
				auto element = ElementAt(index);
				if (!element) throw InvalidArgumentException();
				return element->GetValue();
			}
			bool friend operator == (const List & a, const List & b)
			{
				auto ae = a._first;
				auto be = b._first;
				while (ae) {
					if (!be) return false;
					if (ae->GetValue() != be->GetValue()) return false;
					ae = ae->GetNext();
					be = be->GetNext();
				}
				if (be) return false;
				return true;
			}
			bool friend operator != (const List & a, const List & b) { return !(a == b); }
		};
		template<class E> class ISequence : public List<E>
		{
		public:
			virtual void Push(const E & value) = 0;
			virtual E Pop(void) = 0;
			ISequence & operator << (const E & value) { Push(value); return *this; }
			ISequence & operator >> (E & value) { value = Pop(); return *this; }
		};
		template<class E> class Queue : public ISequence<E>
		{
		public:
			virtual ImmutableString GetListType(void) const override { return L"Queue"; }
			virtual void Push(const E & value) override { this->InsertLast(value); }
			virtual E Pop(void) override
			{
				auto first = this->GetFirst();
				if (!first) throw InvalidStateException();
				auto value = first->GetValue();
				this->RemoveFirst();
				return value;
			}
		};
		template<class E> class Stack : public ISequence<E>
		{
		public:
			virtual ImmutableString GetListType(void) const override { return L"Stack"; }
			virtual void Push(const E & value) override { this->InsertLast(value); }
			virtual E Pop(void) override
			{
				auto last = this->GetLast();
				if (!last) throw InvalidStateException();
				auto value = last->GetValue();
				this->RemoveLast();
				return value;
			}
		};

		template<class E> class BinaryTree : public Object
		{
		public:
			class Element
			{
				friend class Engine::Volumes::BinaryTree<E>;
				Element * _left, * _right, * _parent;
				E _value;
				bool _black;
				int Count(void) const noexcept { int result = 1; if (_left) result += _left->Count(); if (_right) result += _right->Count(); return result; }
				void Destruct(void) noexcept { if (_left) _left->Destruct(); if (_right) _right->Destruct(); delete this; }
				Element(const E & src) : _left(0), _right(0), _parent(0), _value(src), _black(false) {}
				Element(const Element & src, Element * new_root) : _value(src._value), _parent(new_root), _black(src._black)
				{
					Element * new_left = 0, * new_right = 0;
					try {
						if (src._left) {
							new_left = new (std::nothrow) Element(*src._left, this);
							if (!new_left) throw OutOfMemoryException();
						}
						if (src._right) {
							new_right = new (std::nothrow) Element(*src._right, this);
							if (!new_right) throw OutOfMemoryException();
						}
					} catch (...) {
						if (new_left) new_left->Destruct();
						if (new_right) new_right->Destruct();
						throw;
					}
					_left = new_left;
					_right = new_right;
				}
			public:
				~Element(void) {}
				bool IsBlack(void) const noexcept { return _black; }
				const Element * GetLeft(void) const noexcept { return _left; }
				Element * GetLeft(void) noexcept { return _left; }
				const Element * GetRight(void) const noexcept { return _right; }
				Element * GetRight(void) noexcept { return _right; }
				const Element * GetParent(void) const noexcept { return _parent; }
				Element * GetParent(void) noexcept { return _parent; }
				const E & GetValue(void) const noexcept { return _value; }
				E & GetValue(void) noexcept { return _value; }
				const Element * GetPrevious(void) const noexcept
				{
					if (_left) {
						auto result = _left;
						while (result->_right) result = result->_right;
						return result;
					} else {
						const Element * prev = this;
						const Element * current = _parent;
						while (current) {
							if (current->_left == prev) {
								prev = current;
								current = current->_parent;
							} else return current;
						}
						return 0;
					}
				}
				Element * GetPrevious(void) noexcept
				{
					if (_left) {
						auto result = _left;
						while (result->_right) result = result->_right;
						return result;
					} else {
						Element * prev = this;
						Element * current = _parent;
						while (current) {
							if (current->_left == prev) {
								prev = current;
								current = current->_parent;
							} else return current;
						}
						return 0;
					}
				}
				const Element * GetNext(void) const noexcept
				{
					if (_right) {
						auto result = _right;
						while (result->_left) result = result->_left;
						return result;
					} else {
						const Element * prev = this;
						const Element * current = _parent;
						while (current) {
							if (current->_right == prev) {
								prev = current;
								current = current->_parent;
							} else return current;
						}
						return 0;
					}
				}
				Element * GetNext(void) noexcept
				{
					if (_right) {
						auto result = _right;
						while (result->_left) result = result->_left;
						return result;
					} else {
						Element * prev = this;
						Element * current = _parent;
						while (current) {
							if (current->_right == prev) {
								prev = current;
								current = current->_parent;
							} else return current;
						}
						return 0;
					}
				}
			};
			template<class I> class ForwardIterator
			{
				I * _element;
			public:
				ForwardIterator(I * element) : _element(element) {}
				auto & operator * (void) const { return _element->GetValue(); }
				auto & operator * (void) { return _element->GetValue(); }
				auto & operator -> (void) const { return _element->GetValue(); }
				auto & operator -> (void) { return _element->GetValue(); }
				bool operator == (const ForwardIterator & b) const { return _element == b._element; }
				bool operator != (const ForwardIterator & b) const { return _element != b._element; }
				ForwardIterator & operator ++ (void) { _element = _element->GetNext(); return *this; }
				ForwardIterator operator ++ (int) { ForwardIterator result(_element->GetNext()); _element = _element->GetNext(); return result; }
			};
			template<class I> class BackwardIterator
			{
				I * _element;
			public:
				BackwardIterator(I * element) : _element(element) {}
				auto & operator * (void) const { return _element->GetValue(); }
				auto & operator * (void) { return _element->GetValue(); }
				auto & operator -> (void) const { return _element->GetValue(); }
				auto & operator -> (void) { return _element->GetValue(); }
				bool operator == (const BackwardIterator & b) const { return _element == b._element; }
				bool operator != (const BackwardIterator & b) const { return _element != b._element; }
				BackwardIterator & operator ++ (void) { _element = _element->GetPrevious(); return *this; }
				BackwardIterator operator ++ (int) { BackwardIterator result(_element->GetPrevious()); _element = _element->GetPrevious(); return result; }
			};
			template<class T, class I> class ForwardEnumerator
			{
				T * _tree;
			public:
				ForwardEnumerator(T * tree) : _tree(tree) {}
				ForwardIterator<I> begin(void) const noexcept { return ForwardIterator<I>(_tree->GetFirst()); }
				ForwardIterator<I> end(void) const noexcept { return ForwardIterator<I>(0); }
			};
			template<class T, class I> class BackwardEnumerator
			{
				T * _tree;
			public:
				BackwardEnumerator(T * tree) : _tree(tree) {}
				BackwardIterator<I> begin(void) const noexcept { return BackwardIterator<I>(_tree->GetLast()); }
				BackwardIterator<I> end(void) const noexcept { return BackwardIterator<I>(0); }
			};
		private:
			Element * _root;
			void _reballance_on_create(Element * start_from)
			{
				auto current = start_from;
				while (current) {
					if (current->_parent && !current->_parent->_black) {
						auto parent = current->_parent;
						auto grandparent = parent->_parent;
						if (grandparent) {
							if (grandparent->_left && grandparent->_right && !grandparent->_left->_black && !grandparent->_right->_black) {
								if (grandparent->_left) grandparent->_left->_black = true;
								if (grandparent->_right) grandparent->_right->_black = true;
								grandparent->_black = false;
								current = grandparent;
							} else {
								if (parent == grandparent->_left && current == parent->_right) {
									auto c1 = parent->_left;
									auto c2 = current->_left;
									auto c3 = current->_right;
									grandparent->_left = current;
									current->_parent = grandparent;
									current->_left = parent;
									parent->_parent = current;
									parent->_left = c1;
									parent->_right = c2;
									current->_right = c3;
									if (c1) c1->_parent = parent;
									if (c2) c2->_parent = parent;
									if (c3) c3->_parent = current;
									current = parent;
								} else if (parent == grandparent->_right && current == parent->_left) {
									auto c1 = current->_left;
									auto c2 = current->_right;
									auto c3 = parent->_right;
									grandparent->_right = current;
									current->_parent = grandparent;
									current->_right = parent;
									parent->_parent = current;
									current->_left = c1;
									parent->_left = c2;
									parent->_right = c3;
									if (c1) c1->_parent = current;
									if (c2) c2->_parent = parent;
									if (c3) c3->_parent = parent;
									current = parent;
								} else {
									auto super = grandparent->_parent;
									Element ** super_ref;
									if (super) {
										if (super->_left == grandparent) super_ref = &super->_left;
										else super_ref = &super->_right;
									} else super_ref = &_root;
									if (parent == grandparent->_left) {
										auto c1 = current->_left;
										auto c2 = current->_right;
										auto c3 = parent->_right;
										auto c4 = grandparent->_right;
										*super_ref = parent;
										parent->_parent = super;
										parent->_left = current;
										parent->_right = grandparent;
										current->_parent = parent;
										grandparent->_parent = parent;
										parent->_left->_left = c1;
										parent->_left->_right = c2;
										parent->_right->_left = c3;
										parent->_right->_right = c4;
										if (c1) c1->_parent = parent->_left;
										if (c2) c2->_parent = parent->_left;
										if (c3) c3->_parent = parent->_right;
										if (c4) c4->_parent = parent->_right;
									} else {
										auto c1 = grandparent->_left;
										auto c2 = parent->_left;
										auto c3 = current->_left;
										auto c4 = current->_right;
										*super_ref = parent;
										parent->_parent = super;
										parent->_left = grandparent;
										parent->_right = current;
										current->_parent = parent;
										grandparent->_parent = parent;
										parent->_left->_left = c1;
										parent->_left->_right = c2;
										parent->_right->_left = c3;
										parent->_right->_right = c4;
										if (c1) c1->_parent = parent->_left;
										if (c2) c2->_parent = parent->_left;
										if (c3) c3->_parent = parent->_right;
										if (c4) c4->_parent = parent->_right;
									}
									grandparent->_black = false;
									parent->_black = true;
									break;
								}
							}
						} else {
							parent->_black = true;
							break;
						}
					} else break;
				}
			}
			void _reballance_on_delete(Element * start_from, Element ** direction)
			{
				while (start_from) {
					Element ** alt_direction;
					if (direction == &start_from->_left) alt_direction = &start_from->_right;
					else alt_direction = &start_from->_left;
					auto sibling = *alt_direction;
					if (!sibling) throw InvalidStateException();
					if ((!sibling->_left || sibling->_left->_black) && (!sibling->_right || sibling->_right->_black)) {
						if (!start_from->_black) {
							start_from->_black = true;
							sibling->_black = false;
							return;
						} else if (!sibling->_black) {
							Element ** attach;
							_get_element_attachment(start_from, &attach);
							if (direction == &start_from->_left) {
								auto c2 = sibling->_left;
								auto c3 = sibling->_right;
								*attach = sibling;
								sibling->_parent = start_from->_parent;
								sibling->_left = start_from;
								sibling->_right = c3;
								start_from->_parent = sibling;
								start_from->_right = c2;
								if (c2) c2->_parent = start_from;
								if (c3) c3->_parent = sibling;
							} else {
								auto c1 = sibling->_left;
								auto c2 = sibling->_right;
								*attach = sibling;
								sibling->_parent = start_from->_parent;
								sibling->_left = c1;
								sibling->_right = start_from;
								if (c1) c1->_parent = sibling;
								start_from->_parent = sibling;
								start_from->_left = c2;
								if (c2) c2->_parent = start_from;
							}
							sibling->_black = true;
							start_from->_black = false;
						} else {
							sibling->_black = false;
							_get_element_attachment(start_from, &direction);
							start_from = start_from->_parent;
						}
					} else {
						if (direction == &start_from->_left && sibling->_left && !sibling->_left->_black) {
							auto sleft = sibling->_left;
							auto c = sleft->_right;
							start_from->_right = sleft;
							sleft->_parent = start_from;
							sleft->_right = sibling;
							sibling->_parent = sleft;
							sibling->_left = c;
							if (c) c->_parent = sibling;
							sleft->_black = true;
							sibling->_black = false;
						} else if (direction == &start_from->_right && sibling->_right && !sibling->_right->_black) {
							auto sright = sibling->_right;
							auto c = sright->_left;
							start_from->_left = sright;
							sright->_parent = start_from;
							sright->_left = sibling;
							sibling->_parent = sright;
							sibling->_right = c;
							if (c) c->_parent = sibling;
							sright->_black = true;
							sibling->_black = false;
						} else if (direction == &start_from->_left && sibling->_right && !sibling->_right->_black) {
							Element ** attach;
							_get_element_attachment(start_from, &attach);
							auto sleft = sibling->_left;
							auto sright = sibling->_right;
							*attach = sibling;
							sibling->_parent = start_from->_parent;
							sibling->_left = start_from;
							sibling->_right = sright;
							start_from->_parent = sibling;
							start_from->_right = sleft;
							sright->_parent = sibling;
							if (sleft) sleft->_parent = start_from;
							sibling->_black = start_from->_black;
							start_from->_black = true;
							sright->_black = true;
							return;
						} else if (direction == &start_from->_right && sibling->_left && !sibling->_left->_black) {
							Element ** attach;
							_get_element_attachment(start_from, &attach);
							auto sleft = sibling->_left;
							auto sright = sibling->_right;
							*attach = sibling;
							sibling->_parent = start_from->_parent;
							sibling->_left = sleft;
							sibling->_right = start_from;
							start_from->_parent = sibling;
							start_from->_left = sright;
							sleft->_parent = sibling;
							if (sright) sright->_parent = start_from;
							sibling->_black = start_from->_black;
							start_from->_black = true;
							sleft->_black = true;
							return;
						} else throw InvalidStateException();
					}
				}
			}
			void _get_element_attachment(Element * element_for, Element *** ptr)
			{
				if (element_for->_parent) {
					if (element_for->_parent->_left == element_for) *ptr = &element_for->_parent->_left;
					else *ptr = &element_for->_parent->_right;
				} else *ptr = &_root;
			}
			void _swap_element_bodies(Element * upper, Element * lower)
			{
				Element ** upper_attach;
				Element ** lower_attach;
				_get_element_attachment(upper, &upper_attach);
				_get_element_attachment(lower, &lower_attach);
				swap(upper->_left, lower->_left);
				swap(upper->_right, lower->_right);
				swap(upper->_parent, lower->_parent);
				swap(upper->_black, lower->_black);
				if (lower->_left == lower) lower->_left = upper;
				else if (lower->_right == lower) lower->_right = upper;
				else *lower_attach = upper;
				if (upper->_parent == upper) upper->_parent = lower;
				*upper_attach = lower;
				if (lower->_left) lower->_left->_parent = lower;
				if (lower->_right) lower->_right->_parent = lower;
				if (upper->_left) upper->_left->_parent = upper;
				if (upper->_right) upper->_right->_parent = upper;
			}
		public:
			BinaryTree(void) : _root(0) {}
			BinaryTree(const BinaryTree & src)
			{
				if (src._root) {
					_root = new (std::nothrow) Element(*src._root, 0);
					if (!_root) throw OutOfMemoryException();
				} else _root = 0;
			}
			BinaryTree(BinaryTree && src) { _root = src._root; src._root = 0; }
			virtual ~BinaryTree(void) override { if (_root) _root->Destruct(); }
			BinaryTree & operator = (const BinaryTree & src)
			{
				if (&src == this) return *this;
				if (_root) _root->Destruct();
				if (src._root) {
					_root = new (std::nothrow) Element(*src._root, 0);
					if (!_root) throw OutOfMemoryException();
				} else _root = 0;
				return *this;
			}
			virtual ImmutableString GetTreeType(void) const { return L"Binary Tree"; }
			virtual ImmutableString ToString(void) const override
			{
				DynamicString result;
				result << GetTreeType();
				result << L" : [";
				auto e = GetFirst();
				while (e) {
					result << GetStringRepresentation(e->GetValue());
					e = e->GetNext();
					if (e) result << L", ";
				}
				result << L"]";
				return result.ToString();
			}

			const Element * FindElement(const E & value) const
			{
				auto _current = _root;
				while (_current) {
					if (value < _current->_value) _current = _current->_left;
					else if (value > _current->_value) _current = _current->_right;
					else return _current;
				}
				return 0;
			}
			Element * FindElement(const E & value, bool create_if_not_exists = false, bool * was_created = 0)
			{
				if (was_created) *was_created = false;
				if (!_root && create_if_not_exists) {
					auto element = new (std::nothrow) Element(value);
					if (!element) throw OutOfMemoryException();
					_root = element;
					if (was_created) *was_created = true;
					return element;
				}
				auto _current = _root;
				while (_current) {
					if (value < _current->_value) {
						if (!_current->_left && create_if_not_exists) {
							auto element = new (std::nothrow) Element(value);
							if (!element) throw OutOfMemoryException();
							element->_parent = _current;
							_current->_left = element;
							_reballance_on_create(element);
							if (was_created) *was_created = true;
							return element;
						}
						_current = _current->_left;
					} else if (value > _current->_value) {
						if (!_current->_right && create_if_not_exists) {
							auto element = new (std::nothrow) Element(value);
							if (!element) throw OutOfMemoryException();
							element->_parent = _current;
							_current->_right = element;
							_reballance_on_create(element);
							if (was_created) *was_created = true;
							return element;
						}
						_current = _current->_right;
					} else return _current;
				}
				return 0;
			}
			template<class T> const Element * FindElementEquivalent(const T & value) const noexcept
			{
				auto _current = _root;
				while (_current) {
					if (value < _current->_value) _current = _current->_left;
					else if (value > _current->_value) _current = _current->_right;
					else return _current;
				}
				return 0;
			}
			template<class T> Element * FindElementEquivalent(const T & value) noexcept
			{
				auto _current = _root;
				while (_current) {
					if (value < _current->_value) _current = _current->_left;
					else if (value > _current->_value) _current = _current->_right;
					else return _current;
				}
				return 0;
			}
			void Remove(Element * element) noexcept
			{
				if (!element) return;
				if (element->_parent || element->_left || element->_right) {
					auto current = element;
					while (true) {
						if (current->_left && current->_right) {
							auto substitute = current->GetPrevious();
							_swap_element_bodies(current, substitute);
						} else if (current->_left) {
							Element ** attach;
							_get_element_attachment(current, &attach);
							current->_left->_black = true;
							current->_left->_parent = current->_parent;
							*attach = current->_left;
							delete current;
							return;
						} else if (current->_right) {
							Element ** attach;
							_get_element_attachment(current, &attach);
							current->_right->_black = true;
							current->_right->_parent = current->_parent;
							*attach = current->_right;
							delete current;
							return;
						} else {
							Element ** attach;
							_get_element_attachment(current, &attach);
							auto black = current->_black;
							auto parent = current->_parent;
							delete current;
							*attach = 0;
							if (black) _reballance_on_delete(parent, attach);
							return;
						}
					}
				} else { delete element; _root = 0; }
			}
			void Clear(void) noexcept { if (_root) _root->Destruct(); _root = 0; }

			const Element * GetRoot(void) const noexcept { return _root; }
			Element * GetRoot(void) noexcept { return _root; }
			const Element * GetFirst(void) const noexcept { auto e = GetRoot(); while (e && e->GetLeft()) e = e->GetLeft(); return e; }
			Element * GetFirst(void) noexcept { auto e = GetRoot(); while (e && e->GetLeft()) e = e->GetLeft(); return e; }
			const Element * GetLast(void) const noexcept { auto e = GetRoot(); while (e && e->GetRight()) e = e->GetRight(); return e; }
			Element * GetLast(void) noexcept { auto e = GetRoot(); while (e && e->GetRight()) e = e->GetRight(); return e; }
			bool IsEmpty(void) const noexcept { return !_root; }

			int Count(void) const noexcept { return _root ? _root->Count() : 0; }
			const Element * ElementAt(int index) const noexcept
			{
				auto e = GetFirst();
				while (e && index > 0) { index--; e = e->GetNext(); }
				if (!index) return e; else return 0;
			}
			Element * ElementAt(int index) noexcept
			{
				auto e = GetFirst();
				while (e && index > 0) { index--; e = e->GetNext(); }
				if (!index) return e; else return 0;
			}

			ForwardEnumerator< BinaryTree<E>, Element > Elements(void) noexcept { return ForwardEnumerator< BinaryTree<E>, Element >(this); }
			ForwardEnumerator< const BinaryTree<E>, const Element > Elements(void) const noexcept { return ForwardEnumerator< const BinaryTree<E>, const Element >(this); }
			BackwardEnumerator< BinaryTree<E>, Element > InversedElements(void) noexcept { return BackwardEnumerator< BinaryTree<E>, Element >(this); }
			BackwardEnumerator< const BinaryTree<E>, const Element > InversedElements(void) const noexcept { return BackwardEnumerator< const BinaryTree<E>, const Element >(this); }
			ForwardIterator< Element > begin(void) noexcept { return ForwardIterator< Element >(GetFirst()); }
			ForwardIterator< const Element > begin(void) const noexcept { return ForwardIterator< const Element >(GetFirst()); }
			ForwardIterator< Element > end(void) noexcept { return ForwardIterator< Element >(0); }
			ForwardIterator< const Element > end(void) const noexcept { return ForwardIterator< const Element >(0); }
		};
		template<class E> class Set : public BinaryTree<E>
		{
		public:
			virtual ImmutableString GetTreeType(void) const override { return L"Set"; }

			void AddElement(const E & value) { this->FindElement(value, true); }
			void RemoveElement(const E & value) { auto e = this->FindElement(value); if (e) this->Remove(e); }
			bool Contains(const E & value) const noexcept { return this->FindElement(value); }

			Set Union(const Set & b) const
			{
				Set result;
				auto i1 = this->GetFirst();
				auto i2 = b.GetFirst();
				while (i1 || i2) {
					if (i1 && i2) {
						if (i1->GetValue() < i2->GetValue()) {
							result.AddElement(i1->GetValue());
							i1 = i1->GetNext();
						} else if (i1->GetValue() > i2->GetValue()) {
							result.AddElement(i2->GetValue());
							i2 = i2->GetNext();
						} else {
							result.AddElement(i1->GetValue());
							i1 = i1->GetNext();
							i2 = i2->GetNext();
						}
					} else if (i1) {
						result.AddElement(i1->GetValue());
						i1 = i1->GetNext();
					} else if (i2) {
						result.AddElement(i2->GetValue());
						i2 = i2->GetNext();
					}
				}
				return result;
			}
			Set Intersect(const Set & b) const
			{
				Set result;
				auto i1 = this->GetFirst();
				auto i2 = b.GetFirst();
				while (i1 && i2) {
					if (i1->GetValue() < i2->GetValue()) {
						i1 = i1->GetNext();
					} else if (i1->GetValue() > i2->GetValue()) {
						i2 = i2->GetNext();
					} else {
						result.AddElement(i1->GetValue());
						i1 = i1->GetNext();
						i2 = i2->GetNext();
					}
				}
				return result;
			}
			Set Subtract(const Set & b) const
			{
				Set result;
				auto i1 = this->GetFirst();
				auto i2 = b.GetFirst();
				while (i1) {
					if (i2) {
						if (i1->GetValue() < i2->GetValue()) {
							result.AddElement(i1->GetValue());
							i1 = i1->GetNext();
						} else if (i1->GetValue() > i2->GetValue()) {
							i2 = i2->GetNext();
						} else {
							i1 = i1->GetNext();
							i2 = i2->GetNext();
						}
					} else {
						result.AddElement(i1->GetValue());
						i1 = i1->GetNext();
					}
				}
				return result;
			}
			Set SymmetricDifference(const Set & b) const
			{
				Set result;
				auto i1 = this->GetFirst();
				auto i2 = b.GetFirst();
				while (i1 || i2) {
					if (i1 && i2) {
						if (i1->GetValue() < i2->GetValue()) {
							result.AddElement(i1->GetValue());
							i1 = i1->GetNext();
						} else if (i1->GetValue() > i2->GetValue()) {
							result.AddElement(i2->GetValue());
							i2 = i2->GetNext();
						} else {
							i1 = i1->GetNext();
							i2 = i2->GetNext();
						}
					} else if (i1) {
						result.AddElement(i1->GetValue());
						i1 = i1->GetNext();
					} else if (i2) {
						result.AddElement(i2->GetValue());
						i2 = i2->GetNext();
					}
				}
				return result;
			}
			bool IsEqual(const Set & b) const noexcept
			{
				auto i1 = this->GetFirst();
				auto i2 = b.GetFirst();
				while (i1 || i2) {
					if (i1 && i2) {
						if (i1->GetValue() != i2->GetValue()) return false;
						i1 = i1->GetNext();
						i2 = i2->GetNext();
					} else if (i1) return false;
					else if (i2) return false;
				}
				return true;
			}

			bool operator [] (const E & value) const noexcept { return Contains(value); }
			Set friend operator + (const Set & a, const Set & b) { return a.Union(b); }
			Set friend operator | (const Set & a, const Set & b) { return a.Union(b); }
			Set friend operator * (const Set & a, const Set & b) { return a.Intersect(b); }
			Set friend operator & (const Set & a, const Set & b) { return a.Intersect(b); }
			Set friend operator - (const Set & a, const Set & b) { return a.Subtract(b); }
			Set friend operator ^ (const Set & a, const Set & b) { return a.SymmetricDifference(b); }
			bool friend operator == (const Set & a, const Set & b) noexcept { return a.IsEqual(b); }
			bool friend operator != (const Set & a, const Set & b) noexcept { return !a.IsEqual(b); }
		};
		template<class K, class V> class KeyValuePair
		{
		public:
			K key;
			V value;
		public:
			KeyValuePair(void) {}
			KeyValuePair(const K & src_key, const V & src_value) : key(src_key), value(src_value) {}
			KeyValuePair(const KeyValuePair & src) : key(src.key), value(src.value) {}
			KeyValuePair & operator = (const KeyValuePair & src) { key = src.key; value = src.value; return *this; }

			bool friend operator == (const KeyValuePair & a, const KeyValuePair & b) { return a.key == b.key; }
			bool friend operator != (const KeyValuePair & a, const KeyValuePair & b) { return a.key != b.key; }
			bool friend operator < (const KeyValuePair & a, const KeyValuePair & b) { return a.key < b.key; }
			bool friend operator <= (const KeyValuePair & a, const KeyValuePair & b) { return a.key <= b.key; }
			bool friend operator > (const KeyValuePair & a, const KeyValuePair & b) { return a.key > b.key; }
			bool friend operator >= (const KeyValuePair & a, const KeyValuePair & b) { return a.key >= b.key; }

			bool friend operator == (const K & a, const KeyValuePair & b) { return a == b.key; }
			bool friend operator != (const K & a, const KeyValuePair & b) { return a != b.key; }
			bool friend operator < (const K & a, const KeyValuePair & b) { return a < b.key; }
			bool friend operator <= (const K & a, const KeyValuePair & b) { return a <= b.key; }
			bool friend operator > (const K & a, const KeyValuePair & b) { return a > b.key; }
			bool friend operator >= (const K & a, const KeyValuePair & b) { return a >= b.key; }

			bool friend operator == (const KeyValuePair & a, const K & b) { return a.key == b; }
			bool friend operator != (const KeyValuePair & a, const K & b) { return a.key != b; }
			bool friend operator < (const KeyValuePair & a, const K & b) { return a.key < b; }
			bool friend operator <= (const KeyValuePair & a, const K & b) { return a.key <= b; }
			bool friend operator > (const KeyValuePair & a, const K & b) { return a.key > b; }
			bool friend operator >= (const KeyValuePair & a, const K & b) { return a.key >= b; }

			operator string (void) const { return GetStringRepresentation(key) + L": " + GetStringRepresentation(value); }
		};
		template<class K, class V> class Dictionary : public BinaryTree< KeyValuePair<K, V> >
		{
		public:
			virtual ImmutableString GetTreeType(void) const override { return L"Dictionary"; }

			bool Append(const K & key, const V & value)
			{
				bool result;
				this->FindElement(KeyValuePair(key, value), true, &result);
				return result;
			}
			bool Update(const K & key, const V & value)
			{
				bool result;
				auto element = this->FindElement(KeyValuePair(key, value), true, &result);
				if (!result) element->GetValue().value = value;
				return result;
			}
			void Remove(const K & key) noexcept
			{
				auto element = this->FindElementEquivalent(key);
				if (element) BinaryTree< KeyValuePair<K, V> >::Remove(element);
			}
			const V * GetElementByKey(const K & key) const noexcept { auto element = this->FindElementEquivalent(key); if (element) return &element->GetValue().value; else return 0; }
			V * GetElementByKey(const K & key) noexcept { auto element = this->FindElementEquivalent(key); if (element) return &element->GetValue().value; else return 0; }
			bool ElementExists(const K & key) const noexcept { return this->FindElementEquivalent(key) != 0; }

			const V * operator [] (const K & key) const noexcept { return GetElementByKey(key); }
			V * operator [] (const K & key) noexcept { return GetElementByKey(key); }
		};
		template<class K, class O> class ObjectDictionary : public Dictionary< K, SafePointer<O> >
		{
		public:
			virtual ImmutableString GetTreeType(void) const override { return L"Object Dictionary"; }

			bool Append(const K & key, O * object)
			{
				SafePointer<O> ptr;
				ptr.SetRetain(object);
				return Dictionary< K, SafePointer<O> >::Append(key, ptr);
			}
			bool Update(const K & key, O * object)
			{
				SafePointer<O> ptr;
				ptr.SetRetain(object);
				return Dictionary< K, SafePointer<O> >::Update(key, ptr);
			}
			O * GetObjectByKey(const K & key) const noexcept { auto element = this->FindElementEquivalent(key); if (element) return element->GetValue().value.Inner(); else return 0; }

			O * operator [] (const K & key) noexcept { return GetObjectByKey(key); }
		};
		template<class K, class O> class ObjectCache : public ObjectDictionary<K, O>
		{
			int _capacity, _iteration;
		public:
			explicit ObjectCache(int capacity) : _capacity(capacity), _iteration(0) {}
			void Push(const K & key, O * object)
			{
				if (!this->GetRoot()) _iteration = 0;
				if (_iteration >= _capacity) {
					if (this->ElementExists(key)) return;
					if (this->GetRoot()) {
						auto current = this->GetFirst();
						auto least = current;
						uint least_rc = least->GetValue().value->GetReferenceCount();
						current = current->GetNext();
						while (current) {
							auto rc = current->GetValue().value->GetReferenceCount();
							if (rc < least_rc) { least_rc = rc; least = current; }
							current = current->GetNext();
						}
						BinaryTree< KeyValuePair<K, SafePointer<O> > >::Remove(least);
						_iteration--;
					}
					if (this->Append(key, object)) _iteration++;
				} else {
					if (this->Append(key, object)) _iteration++;
				}
			}
		};
	}
}