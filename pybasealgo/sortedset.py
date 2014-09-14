"""sortedset from sortedcontainers-0.8.5.tar.gz backported to Python 2.4.

Compatible with: Python 2.4, 2.5, 2.6 and 2.7 (not 3.x).

See sortedlist.py for a list of changes done during the porting.
"""

try:
  from collections import MutableSet
except ImportError:  # collection.MutableSet appeared in Python 2.5.
  class MutableSet(object):
    pass

try:
  from collections import Sequence
except ImportError:  # collection.Sequence appeared in Python 2.5.
  class Sequence(object):
    pass


from itertools import chain

from sortedlist import SortedList, recursive_repr, all, any


class SortedSet(MutableSet, Sequence):
  """
  A `SortedSet` provides the same methods as a `set`.  Additionally, a
  `SortedSet` maintains its items in sorted order, allowing the `SortedSet` to
  be indexed.

  Unlike a `set`, a `SortedSet` requires items be hashable and comparable.
  """
  def __init__(self, iterable=None, load=1000, _set=None):
    if _set is None:
      self._set = set()
    else:
      self._set = set
    self._list = SortedList(self._set, load=load)
    if iterable is not None:
      self.update(iterable)
  def __contains__(self, value):
    return (value in self._set)
  def __getitem__(self, index):
    if isinstance(index, slice):
      return SortedSet(self._list[index])
    else:
      return self._list[index]
  def __delitem__(self, index):
    _list = self._list
    if isinstance(index, slice):
      values = _list[index]
      self._set.difference_update(values)
    else:
      value = _list[index]
      self._set.remove(value)
    del _list[index]
  def __setitem__(self, index, value):
    _list, _set = self._list, self._set
    prev = _list[index]
    _list[index] = value
    if isinstance(index, slice):
      _set.difference_update(prev)
      _set.update(prev)
    else:
      _set.remove(prev)
      _set.add(prev)
  def __eq__(self, that):
    if len(self) != len(that):
      return False
    if isinstance(that, SortedSet):
      return (self._list == that._list)
    elif isinstance(that, set):
      return (self._set == that)
    else:
      _set = self._set
      return all(val in _set for val in that)
  def __ne__(self, that):
    if len(self) != len(that):
      return True
    if isinstance(that, SortedSet):
      return (self._list != that._list)
    elif isinstance(that, set):
      return (self._set != that)
    else:
      _set = self._set
      return any(val not in _set for val in that)
  def __lt__(self, that):
    if isinstance(that, set):
      return (self._set < that)
    else:
      return (len(self) < len(that)) and all(val in that for val in self._list)
  def __gt__(self, that):
    if isinstance(that, set):
      return (self._set > that)
    else:
      _set = self._set
      return (len(self) > len(that)) and all(val in _set for val in that)
  def __le__(self, that):
    if isinstance(that, set):
      return (self._set <= that)
    else:
      return all(val in that for val in self._list)
  def __ge__(self, that):
    if isinstance(that, set):
      return (self._set >= that)
    else:
      _set = self._set
      return all(val in _set for val in that)
  def __and__(self, that):
    return self.intersection(that)
  def __or__(self, that):
    return self.union(that)
  def __sub__(self, that):
    return self.difference(that)
  def __xor__(self, that):
    return self.symmetric_difference(that)
  def __iter__(self):
    return iter(self._list)
  def __len__(self):
    return len(self._set)
  def __reversed__(self):
    return reversed(self._list)
  def add(self, value):
    if value not in self._set:
      self._set.add(value)
      self._list.add(value)
  def bisect_left(self, value):
    return self._list.bisect_left(value)
  def bisect(self, value):
    return self._list.bisect(value)
  def bisect_right(self, value):
    return self._list.bisect_right(value)
  def clear(self):
    self._set.clear()
    self._list.clear()
  def copy(self):
    return SortedSet(load=self._list._load, _set=set(self._set))
  def __copy__(self):
    return self.copy()
  def count(self, value):
    return int(value in self._set)
  def discard(self, value):
    if value in self._set:
      self._set.remove(value)
      self._list.discard(value)
  def index(self, value, start=None, stop=None):
    return self._list.index(value, start, stop)
  def isdisjoint(self, that):
    return self._set.isdisjoint(that)
  def issubset(self, that):
    return self._set.issubset(that)
  def issuperset(self, that):
    return self._set.issuperset(that)
  def pop(self, index=-1):
    value = self._list.pop(index)
    self._set.remove(value)
    return value
  def remove(self, value):
    self._set.remove(value)
    self._list.remove(value)
  def difference(self, *iterables):
    diff = self._set.difference(*iterables)
    new_set = SortedSet(load=self._list._load, _set=diff)
    return new_set
  def difference_update(self, *iterables):
    values = set(chain(*iterables))
    if (4 * len(values)) > len(self):
      self._set.difference_update(values)
      self._list.clear()
      self._list.update(self._set)
    else:
      _discard = self.discard
      for value in values:
        _discard(value)
  def intersection(self, *iterables):
    comb = self._set.intersection(*iterables)
    new_set = SortedSet(load=self._list._load, _set=comb)
    return new_set
  def intersection_update(self, *iterables):
    self._set.intersection_update(*iterables)
    self._list.clear()
    self._list.update(self._set)
  def symmetric_difference(self, that):
    diff = self._set.symmetric_difference(that)
    new_set = SortedSet(load=self._list._load, _set=diff)
    return new_set
  def symmetric_difference_update(self, that):
    self._set.symmetric_difference_update(that)
    self._list.clear()
    self._list.update(self._set)
  def union(self, *iterables):
    return SortedSet(chain(iter(self), *iterables), load=self._list._load)
  def update(self, *iterables):
    values = set(chain(*iterables))
    if (4 * len(values)) > len(self):
      self._set.update(values)
      self._list.clear()
      self._list.update(self._set)
    else:
      _add = self.add
      for value in values:
        _add(value)
  @recursive_repr
  def __repr__(self):
    return '%s(%r)' % (self.__class__.__name__, list(self))
