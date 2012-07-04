#! /usr/bin/python2.4
#
# pobject.py: @override, @final etc. decorator support in Python2.4
# by pts@fazekas.hu at Thu Mar 26 14:32:12 CET 2009
#
# pobjects works in python2.4 and python2.5.
#
# TODO(pts): Add decorators to properties / descriptors.

import re
import sys
import types

class AbstractMethodError(Exception):
  """Raised when an abstract method is called."""


class BadMethod(Exception):
  """Raised when a bad method definition is encountered."""

class BadDecoration(Exception):
  """Raised when a decorator cannot be applied."""


def _ParseClassNameList(spec, f_globals, f_builtins):
  if not isinstance(spec, str): raise TypeError
  classes = []
  for spec_item in spec.split(','):
    path = spec_item.strip().split('.')
    # TODO(pts): More detailed error reporting.
    if '' in path: return None
    path_start = path.pop(0)
    if path_start in f_globals:
      value = f_globals[path_start]
    elif path_start in f_builtins:
      value = f_builtins[path_start]
    else:
      # TODO(pts): More detailed error reporting.
      return None
    for path_item in path:
      # TODO(pts): More detailed error reporting.
      if not hasattr(value, path_item): return None
      value = getattr(value, path_item)
    classes.append(value)
  # TODO(pts): More detailed error reporting.
  if not classes: return None
  return classes


def _Self(value):
  """Helper identity function."""
  return value


def _UnwrapFunction(fwrap):
  """Unwarp function from function, classmethod or staticmethod.
  
  Args:
    fwrap: A function, classmethod or staticmethod.
  Returns:
    (function, wrapper), where `function' is the function unwrapped from
    fwrap, and wrapper is a callable which can be called on function to
    yield something equivalent to fwrap. The value (None, None) is returned
    if fwrap is is not of the right type.
  """
  if isinstance(fwrap, types.FunctionType):
    return fwrap, _Self
  elif isinstance(fwrap, classmethod):
    return fwrap.__get__(0).im_func, classmethod
  elif isinstance(fwrap, staticmethod):
    return fwrap.__get__(0), staticmethod
  else:
    return None, None


class _OldStyleClass:
  pass


class _MetaClassProxy(object):
  def __init__(self, orig_metaclass):
    self.orig_metaclass = orig_metaclass
    # List of dicts describing decorators applying to methods in this class,
    # see pobject_decorator below.
    self.decorators = []

  def __call__(self, class_name, bases, dict_obj):
    decorator_names = set([decorator['decorator_name']
                           for decorator in self.decorators])
    if not decorator_names or (
        isinstance(self.orig_metaclass, type) and
        issubclass(self.orig_metaclass, _PObjectMeta)):
      return self.orig_metaclass(class_name, bases, dict_obj)
    if 'final' not in decorator_names and 'finalim' not in decorator_names:
      CheckDecorators(class_name, bases, dict_obj)
      if not [1 for base in bases
              if not isinstance(base, type(_OldStyleClass))]:
        # Avoid `TypeError: Error when calling the metaclass bases;
        # a new-style class can't have only classic bases'.
        bases += (object,)
      return self.orig_metaclass(class_name, bases, dict_obj)
    # We have @final and/or @finalim among the decorators we want to apply.
    # Since these have affect on subclasses, we have to set up _PObjectMeta
    # as the metaclass in order to get CheckDecorators called when a
    # subclass is created.
    error_prefix = '; '.join([
        'decorator @%s in file %r, line %s cannot be applied to %s' %
        (decorator['decorator_name'], decorator['file_name'],
        decorator['line_number'], decorator['full_method_name'])
        for decorator in self.decorators])
    full_class_name = '%s.%s' % (dict_obj['__module__'], class_name)
    if (not isinstance(self.orig_metaclass, type) or
        not issubclass(_PObjectMeta, self.orig_metaclass)):
      raise BadDecoration(
          '%s; because the specified %s.__metaclass__ (%s) is not a superclass '
          'of _PObjectMeta' %
         (error_prefix, full_class_name, self.orig_metaclass))
    # This for loop is to give a better error message than:
    # TypeError: Error when calling the metaclass bases:: metaclass
    # conflict: the metaclass of a derived class must be a (non-strict)
    # subclass of the metaclasses of all its bases
    #bases = (type(re.compile('a').scanner('abc')),)
    old_base_count = 0
    for base in bases:
      if isinstance(base, type(_OldStyleClass)):
        old_base_count += 1
      elif not isinstance(base, type):
        # TODO(pts): exactly how to trigger this
        # Counterexamples: base == float; base == str; regexp match.
        # Examples: Exception.
        raise BadDecoration(
            '%s; because superclass %s is not a pure class' %
            (error_prefix, base))
      elif not issubclass(_PObjectMeta, type(base)) or 1:
        raise BadDecoration(
            '%s; because superclass %s has metaclass %s, '
            'which conflicts with _PObjectMeta' %
            (error_prefix, base, type(base)))
    if old_base_count == len(bases):
      # Avoid `TypeError: Error when calling the metaclass bases;
      # a new-style class can't have only classic bases'.
      # TODO(pts): Test this with exceptions.
      bases += (object,)
    return _PObjectMeta(class_name, bases, dict_obj)


def pobject_decorator(decorator):
  #print 'MMM', decorator.func_name
  def ApplyDecorator(fwrap):
    """Applies decorator to function in f (function or method)."""
    function, wrapper = _UnwrapFunction(fwrap)
    if not isinstance(function, types.FunctionType):
      raise BadDecoration(
          'decorator @%s cannot be applied to non-function %r' %
          (decorator.func_name, function))
    f = sys._getframe().f_back
    if '__module__' not in f.f_locals:
      raise BadDecoration(
          'decorator @%s cannot be applied to function %s in %s, '
          'because the latter is not a class definition' %
          (decorator.func_name, function.func_name, f.f_code.co_name))
    module_name = f.f_locals['__module__']
    full_method_name = '%s.%s.%s' % (
        module_name, f.f_code.co_name, function.func_name)
    # `type' below silently upgrades an old-style class to a new-style class.
    metaclass = f.f_locals.get('__metaclass__') or type
    if not isinstance(metaclass, _MetaClassProxy):
      # TODO(pts): Document that this doesn't work if __metaclass__ is
      # assigned after the first decorated method.
      f.f_locals['__metaclass__'] = metaclass = _MetaClassProxy(metaclass)
    metaclass.decorators.append({
        'decorator_name': decorator.func_name,
        'full_method_name': full_method_name,
        'file_name': f.f_code.co_filename,
        'line_number': f.f_lineno,
    })
    decorated_function = decorator(
        function=function, full_method_name=full_method_name)
    #print (decorator.func_name, decorated_function, function, wrapper)
    if decorated_function is function:
      return fwrap  # The wrapped function, classmethod or staticmethod.
    else:
      return wrapper(decorated_function)
  return ApplyDecorator


def _GenerateBothAbstractAndFinal(full_method_name):
  return 'decorators @abstract and @final cannot be both applied to %s' % (
      full_method_name)

def _GenerateBothAbstractAndFinalim(full_method_name):
  return 'decorators @abstract and @finalim cannot be both applied to %s' % (
      full_method_name)

@pobject_decorator
def abstract(function, full_method_name):
  assert type(function) == types.FunctionType

  if getattr(function, '_is_final', None):
    raise BadDecoration(_GenerateBothAbstractAndFinal(full_method_name))

  # We'd lose ._is_finalim on AbstractFunction anyway.
  if getattr(function, '_is_finalim', None):
    raise BadDecoration(_GenerateBothAbstractAndFinalim(full_method_name))

  def AbstractFunction(self, *args, **kwargs):
    if not isinstance(self, type):
      self = type(self)
    raise AbstractMethodError(
        'abstract method %s.%s.%s called' %
        (self.__module__, self.__name__, function.func_name))

  AbstractFunction._is_abstract = True

  # TODO(pts): Fail at runtime when instantiating an abstract class.

  return AbstractFunction


@pobject_decorator
def nosuper(function, full_method_name):
  assert type(function) == types.FunctionType
  function._is_nosuper = True
  return function


@pobject_decorator
def final(function, full_method_name):
  assert type(function) == types.FunctionType

  if getattr(function, '_is_abstract', None):
    raise BadDecoration(_GenerateBothAbstractAndFinal(full_method_name))

  function._is_final = True
  return function


@pobject_decorator
def finalim(function, full_method_name):
  """Declare that the method cannot be overriden with instance methods."""
  assert type(function) == types.FunctionType

  function._is_finalim = True
  return function


@pobject_decorator
def override(function, full_method_name):
  assert type(function) == types.FunctionType
  function._is_override = True
  return function


def _DumpSuperClassList(bases):
  if len(bases) == 1:
    return 'superclass %s.%s' % (bases[0].__module__, bases[0].__name__)
  else:
    return 'superclasses ' + ', '.join(
        ['%s.%s' % (base.__module__, base.__name__) for base in bases])


def CheckDecorators(class_name, bases, dict_obj):
  """Raise BadMethod if the new class is inconsistent with some decorators."""
  module = dict_obj['__module__']
  for name in sorted(dict_obj):
    function, _ = _UnwrapFunction(dict_obj[name])
    if isinstance(function, types.FunctionType):
      #print (class_name, name)
      if getattr(function, '_is_nosuper', None):
        bases_with_name = [base for base in bases if hasattr(base, name)]
        if bases_with_name:
          # Unfortunately, we don't get the method definition line in the
          # traceback. TODO(pts): Somehow forge it.
          raise BadMethod(
              '@nosuper method %s.%s.%s defined in %s' %
              (module, class_name, name,
               _DumpSuperClassList(bases_with_name)))
      if getattr(function, '_is_override', None):
        bases_with_name = [base for base in bases if hasattr(base, name)]
        if not bases_with_name:
          # TODO(pts): report line numbers (elsewhere etc.)
          raise BadMethod(
              '@override method %s.%s.%s not defined in %s' %
              (module, class_name, name, _DumpSuperClassList(bases)))
      # We don't need any special casing for getattr(..., '_is_final', None) below
      # if getattr(base, name) is an ``instancemethod'' created from a
      # classmethod or a function. This is because an instancemathod
      # automirorrs all attributes of its im_func.
      bases_with_final = [
          base for base in bases if hasattr(base, name) and
          getattr(getattr(base, name), '_is_final', None)]
      if bases_with_final:
        raise BadMethod(
            'method %s.%s.%s overrides @final method in %s' %
              (module, class_name, name,
               _DumpSuperClassList(bases_with_final)))
      if function is dict_obj[name]:  # function is instance method
        bases_with_finalim = [
            base for base in bases if hasattr(base, name) and
            getattr(getattr(base, name), '_is_finalim', None)]
        if bases_with_finalim:
          raise BadMethod(
              'instance method %s.%s.%s overrides @finalim method in %s' %
                (module, class_name, name,
                 _DumpSuperClassList(bases_with_finalim)))


class _PObjectMeta(type):
  def __new__(cls, class_name, bases, dict_obj):
    # cls is _PObjectMeta here.
    #print ('NewClass', cls, class_name, bases, dict_obj)
    CheckDecorators(class_name, bases, dict_obj)
    return type.__new__(cls, class_name, bases, dict_obj)

class pobject(object):
  __metaclass__ = _PObjectMeta  # be type(pobjects) == _PObjectMeta, not type  

# Updates for all modules.
__builtins__['pobject'] = pobject
__builtins__['final'] = final
__builtins__['finalim'] = finalim
__builtins__['nosuper'] = nosuper
__builtins__['abstract'] = abstract
__builtins__['override'] = override
