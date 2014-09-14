"""sorteddict from sortedcontainers-0.8.5.tar.gz backported to Python 2.4.

Compatible with: Python 2.4, 2.5, 2.6 and 2.7 (not 3.x).

See sortedlist.py for a list of changes done during the porting.
"""

try:
  from collections import Mapping
except ImportError:  # collection.Mapping appeared in Python 2.6.
  class Mapping(object):
    pass

try:
  from collections import MutableMapping
except ImportError:  # collection.MutableMapping appeared in Python 2.6.
  class MutableMapping(object):
    pass

try:
  from collections import Set
except ImportError:  # collection.Set appeared in Python 2.6.
  class Set(object):
    pass

try:
  from collections import Sequence
except ImportError:  # collection.Sequence appeared in Python 2.6.
  class Sequence(object):
    pass

try:
  from collections import KeysView as AbstractKeysView
except ImportError:  # collection.KeysView appeared in Python 2.6.
  class AbstractKeysView(object):
    pass

try:
  from collections import ValuesView as AbstractValuesView
except ImportError:  # collection.ValuesView appeared in Python 2.6.
  class AbstractValuesView(object):
    pass

try:
  from collections import ItemsView as AbstractItemsView
except ImportError:  # collection.ItemsView appeared in Python 2.6.
  class AbstractItemsView(object):
    pass

from sortedset import SortedSet
from sortedlist import SortedList, recursive_repr, wraps, all, any

_NotGiven = object()

def not26(func):
  from sys import hexversion
  @wraps(func)
  def errfunc(*args, **kwargs):
    raise NotImplementedError
  if hexversion < 0x02070000:
    return errfunc
  else:
    return func


class _IlocWrapper:
  def __init__(self, _dict):
    self._dict = _dict
  def __len__(self):
    return len(self._dict)
  def __getitem__(self, index):
    return self._dict._list[index]
  def __delitem__(self, index):
    _temp = self._dict
    _list, _dict = _temp._list, _temp._dict
    if isinstance(index, slice):
      keys = _list[index]
      del _list[index]
      for key in keys:
        del _dict[key]
    else:
      key = _list[index]
      del _list[index]
      del _dict[key]


class SortedDict(MutableMapping):
  """
  A SortedDict provides the same methods as a dict.  Additionally, a
  SortedDict efficiently maintains its keys in sorted order. Consequently, the
  keys method will return the keys in sorted order, the popitem method will
  remove the item with the highest key, etc.
  """
  def __init__(self, *args, **kwargs):
    if len(args) > 0 and type(args[0]) == int:
      load = args[0]
      args = args[1:]
    else:
      load = 1000
    self._dict = dict()
    self._list = SortedList(load=load)
    self.iloc = _IlocWrapper(self)
    self.update(*args, **kwargs)
  def clear(self):
    self._dict.clear()
    self._list.clear()
  def __contains__(self, key):
    return key in self._dict
  def __delitem__(self, key):
    del self._dict[key]
    self._list.remove(key)
  def __getitem__(self, key):
    return self._dict[key]
  def __eq__(self, that):
    return (len(self._dict) == len(that)
        and all((key in that) and (self[key] == that[key])
            for key in self))
  def __ne__(self, that):
    return (len(self._dict) != len(that)
        or any((key not in that) or (self[key] != that[key])
             for key in self))
  def __iter__(self):
    return iter(self._list)
  def __reversed__(self):
    return reversed(self._list)
  def __len__(self):
    return len(self._dict)
  def __setitem__(self, key, value):
    _dict = self._dict
    if key not in _dict:
      self._list.add(key)
    _dict[key] = value
  def copy(self):
    return SortedDict(self._list._load, self._dict)
  def __copy__(self):
    return self.copy()
  @classmethod
  def fromkeys(cls, seq, value=None):
    that = SortedDict((key, value) for key in seq)
    return that
  def get(self, key, default=None):
    return self._dict.get(key, default)
  def has_key(self, key):
    return key in self._dict
  def items(self):
    return list(self.iteritems())
  def iteritems(self):
    _dict = self._dict
    return iter((key, _dict[key]) for key in self._list)
  def keys(self):
    return SortedSet(self._dict)
  def iterkeys(self):
    return iter(self._list)
  def values(self):
    return list(self.itervalues())
  def itervalues(self):
    _dict = self._dict
    return iter(_dict[key] for key in self._list)
  def pop(self, key, default=_NotGiven):
    if key in self._dict:
      self._list.remove(key)
      return self._dict.pop(key)
    else:
      if default == _NotGiven:
        raise KeyError
      else:
        return default
  def popitem(self):
    _dict, _list = self._dict, self._list
    if len(_dict) == 0:
      raise KeyError
    key = _list.pop()
    value = _dict[key]
    del _dict[key]
    return (key, value)
  def setdefault(self, key, default=None):
    _dict = self._dict
    if key in _dict:
      return _dict[key]
    else:
      _dict[key] = default
      self._list.add(key)
      return default
  def update(self, *args, **kwargs):
    _dict, _list = self._dict, self._list
    if len(_dict) == 0:
      _dict.update(*args, **kwargs)
      _list.update(_dict)
      return
    if (len(kwargs) == 0 and len(args) == 1 and isinstance(args[0], dict)):
      pairs = args[0]
    else:
      pairs = dict(*args, **kwargs)
    if (10 * len(pairs)) > len(self._dict):
      self._dict.update(pairs)
      _list = self._list
      _list.clear()
      _list.update(self._dict)
    else:
      for key in pairs:
        self[key] = pairs[key]
  def index(self, key, start=None, stop=None):
    return self._list.index(key, start, stop)
  def bisect_left(self, key):
    return self._list.bisect_left(key)
  def bisect(self, key):
    return self._list.bisect(key)
  def bisect_right(self, key):
    return self._list.bisect_right(key)
  @not26
  def viewkeys(self):
    return KeysView(self)
  @not26
  def viewvalues(self):
    return ValuesView(self)
  @not26
  def viewitems(self):
    return ItemsView(self)
  @recursive_repr
  def __repr__(self):
    _dict = self._dict
    return '%s({%s})' % (
      self.__class__.__name__,
      ', '.join('%r: %r' % (key, _dict[key]) for key in self._list))


class KeysView(AbstractKeysView, Set, Sequence):
  def __init__(self, sorted_dict):
    self._list = sorted_dict._list
    self._view = sorted_dict._dict.viewkeys()
  def __len__(self):
    return len(self._view)
  def __contains__(self, key):
    return key in self._view
  def __iter__(self):
    return iter(self._list)
  def __getitem__(self, index):
    return self._list[index]
  def __reversed__(self):
    return reversed(self._list)
  def index(self, value, start=None, stop=None):
    return self._list.index(value, start, stop)
  def count(self, value):
    return int(value in self._view)
  def __eq__(self, that):
    return self._view == that
  def __ne__(self, that):
    return self._view != that
  def __lt__(self, that):
    return self._view < that
  def __gt__(self, that):
    return self._view > that
  def __le__(self, that):
    return self._view <= that
  def __ge__(self, that):
    return self._view >= that
  def __and__(self, that):
    return SortedSet(self._view & that)
  def __or__(self, that):
    return SortedSet(self._view | that)
  def __sub__(self, that):
    return SortedSet(self._view - that)
  def __xor__(self, that):
    return SortedSet(self._view ^ that)
  def isdisjoint(self, that):
    return not any(key in self._list for key in that)
  @recursive_repr
  def __repr__(self):
    return 'SortedDict_keys(%r)' % list(self)


class ValuesView(AbstractValuesView, Sequence):
  def __init__(self, sorted_dict):
    self._dict = sorted_dict
    self._list = sorted_dict._list
    self._view = sorted_dict._dict.viewvalues()
  def __len__(self):
    return len(self._dict)
  def __contains__(self, value):
    return value in self._view
  def __iter__(self):
    _dict = self._dict
    return iter(_dict[key] for key in self._list)
  def __getitem__(self, index):
    _dict, _list = self._dict, self._list
    if isinstance(index, slice):
      return [_dict[key] for key in _list[index]]
    else:
      return _dict[_list[index]]
  def __reversed__(self):
    _dict = self._dict
    return iter(_dict[key] for key in reversed(self._list))
  def index(self, value):
    for idx, val in enumerate(self):
      if value == val:
        return idx
    else:
      raise ValueError
  def count(self, value):
    _dict = self._dict
    return sum(1 for val in _dict.itervalues() if val == value)
  def __lt__(self, that):
    raise TypeError
  def __gt__(self, that):
    raise TypeError
  def __le__(self, that):
    raise TypeError
  def __ge__(self, that):
    raise TypeError
  def __and__(self, that):
    raise TypeError
  def __or__(self, that):
    raise TypeError
  def __sub__(self, that):
    raise TypeError
  def __xor__(self, that):
    raise TypeError
  @recursive_repr
  def __repr__(self):
    return 'SortedDict_values(%r)' % list(self)


class ItemsView(AbstractItemsView, Set, Sequence):
  def __init__(self, sorted_dict):
    self._dict = sorted_dict
    self._list = sorted_dict._list
    self._view = sorted_dict._dict.viewitems()
  def __len__(self):
    return len(self._view)
  def __contains__(self, key):
    return key in self._view
  def __iter__(self):
    _dict = self._dict
    return iter((key, _dict[key]) for key in self._list)
  def __getitem__(self, index):
    _dict, _list = self._dict, self._list
    if isinstance(index, slice):
      return [(key, _dict[key]) for key in _list[index]]
    else:
      key = _list[index]
      return (key, _dict[key])
  def __reversed__(self):
    _dict = self._dict
    return iter((key, _dict[key]) for key in reversed(self._list))
  def index(self, key, start=None, stop=None):
    pos = self._list.index(key[0], start, stop)
    if key[1] == self._dict[key[0]]:
      return pos
    else:
      raise ValueError
  def count(self, item):
    key, value = item
    return int(key in self._dict and self._dict[key] == value)
  def __eq__(self, that):
    return self._view == that
  def __ne__(self, that):
    return self._view != that
  def __lt__(self, that):
    return self._view < that
  def __gt__(self, that):
    return self._view > that
  def __le__(self, that):
    return self._view <= that
  def __ge__(self, that):
    return self._view >= that
  def __and__(self, that):
    return SortedSet(self._view & that)
  def __or__(self, that):
    return SortedSet(self._view | that)
  def __sub__(self, that):
    return SortedSet(self._view - that)
  def __xor__(self, that):
    return SortedSet(self._view ^ that)
  def isdisjoint(self, that):
    _dict = self._dict
    for key, value in that:
      if key in _dict and _dict[key] == value:
        return False
    return True
  @recursive_repr
  def __repr__(self):
    return 'SortedDict_items({0})' % list(self)
