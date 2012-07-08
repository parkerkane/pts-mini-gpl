#! /usr/bin/python2.4
# by pts@fazekas.hu at Wed Jul  4 14:44:36 CEST 2012
#
# TODO(pts): Find that the class works as expected on success.

import unittest

#import pobjects_builtins
import pobjects
from pobjects import *


class PobjectsTest(unittest.TestCase):
  def assertRaisesWithLiteralMatch(self, expected_exception,
                                   expected_exception_message, callable_obj,
                                   *args, **kwargs):
    """Asserts that the message in a raised exception equals the given string.

    Args:
      expected_exception: Exception class expected to be raised.
      expected_exception_message: String message expected in the raised
        exception.  For a raise exception e, expected_exception_message must
        equal str(e).
      callable_obj: Function to be called.
      args: Extra args.
      kwargs: Extra kwargs.
    """
    try:
      callable_obj(*args, **kwargs)
    except expected_exception, err:
      actual_exception_message = str(err)
      self.assert_(expected_exception_message == actual_exception_message,
                   'Exception message does not match.\nExpected: %r\n'
                   'Actual: %r' % (expected_exception_message,
                                   actual_exception_message))
    else:
      self.fail(expected_exception.__name__ + ' not raised')

  def assertCompileError(self, expected_exception, expected_exception_message,
                         code):
    if isinstance(code, str):
      code = code.rstrip()
    def Do():
      globals_dict = {'__name__': 'testmod', 'final': final}
      exec code in globals_dict
    self.assertRaisesWithLiteralMatch(
        expected_exception, expected_exception_message, Do)

  def testFinal(self):
    class Foo:
      def eat(self): return 'FOO.EAT'
      @final
      def drink(self): return 'FOO.DRINK'
    class Bar(Foo): pass
    class Baz(Bar):
      def eat(self): return 'BAZ.EAT'
    self.assertEquals('FOO.EAT', Foo().eat())
    self.assertEquals('FOO.EAT', Bar().eat())
    self.assertEquals('BAZ.EAT', Baz().eat())
    self.assertEquals('FOO.DRINK', Foo().drink())
    self.assertEquals('FOO.DRINK', Bar().drink())
    self.assertEquals('FOO.DRINK', Baz().drink())

    class Foo:
      def eat(self): return 'FOO.EAT'
      @final
      def drink(self): return 'FOO.DRINK'
    class Baz(Foo):
      def eat(self): return 'BAZ.EAT'
    self.assertEquals('FOO.EAT', Foo().eat())
    self.assertEquals('BAZ.EAT', Baz().eat())
    self.assertEquals('FOO.DRINK', Foo().drink())
    self.assertEquals('FOO.DRINK', Baz().drink())

    self.assertCompileError(
        pobjects.BadClass,
        'Cannot create class testmod.Baz because method eat overrides @final '
        'method testmod.Foo.eat.', r"""if 1:
        class Foo:
          @final
          def eat(self): print 'FOO.EAT'
        class Bar(Foo): pass
        class Baz(Bar):
          def eat(self): print 'BAZ.EAT'
        """)

    self.assertCompileError(
        pobjects.BadClass,
        'Cannot create class testmod.Baz because method eat overrides @final '
        'method testmod.Foo.eat.', r"""if 1:
        class Foo:
          @final
          def eat(self): print 'FOO.EAT'
        class Baz(Foo):
          def eat(self): print 'BAZ.EAT'
        """)

    self.assertCompileError(
        pobjects.BadClass,
        'Cannot create class testmod.Bar because method drink overrides @final '
        'method testmod.Foo.drink; method eat overrides @final method '
        'testmod.Foo.eat.', r"""if 1:
        class Foo:
          @final
          def eat(self): pass
          @final
          def drink(self): pass
        class Bar(Foo):
          def eat(self): pass
          def drink(self): pass
        """)

  def testAbstract(self):
    # TODO(pts): Test with @abstract __init__ methods.
    class Foo(object):
      answer = 42
      @abstract
      def eat(self): pass
      def sleep(self): return 'FOO.SLEEP'
    class Bar(Foo):
      def drink(self): return 'BAR.DRINK'
    class Baz(Bar):
      def eat(self): return 'BAZ.EAT'
      def sleep(self): return 'BAZ.SLEEP'
    self.assertEquals(42, Bar.answer)
    baz = Baz()
    self.assertEquals(Baz, type(baz))
    self.assertEquals('BAZ.EAT', baz.eat())
    self.assertEquals('BAR.DRINK', baz.drink())
    self.assertEquals('BAZ.SLEEP', baz.sleep())
    self.assertEquals(pobjects._PObjectMeta, type(Baz))
    self.assertRaisesWithLiteralMatch(
        pobjects.BadInstantiation,
        'Cannot instantiate abstract class __main__.Bar because it has '
        '@abstract method __main__.Foo.eat', Bar)
    self.assertRaisesWithLiteralMatch(
        pobjects.BadInstantiation,
        'Cannot instantiate abstract class __main__.Foo because it has '
        '@abstract method __main__.Foo.eat', Foo)

    class Foo(object):
      def __init__(self, x, y): self.answer = x * y
      answer = 41
      @abstract
      def eat(self): pass
    class Baz(Foo):
      def eat(self): print 'EATING'
    self.assertEquals(41, Foo.answer)
    baz = Baz(6, 7)
    self.assertEquals(Baz, type(baz))
    self.assertEquals(41, Baz.answer)
    self.assertEquals(42, baz.answer)
    self.assertEquals(pobjects._PObjectMeta, type(Baz))
    self.assertRaisesWithLiteralMatch(
        pobjects.BadInstantiation,
        'Cannot instantiate abstract class __main__.Foo because it has '
        '@abstract method __main__.Foo.eat', Foo)

    class Foo(object):
      answer = 42
      @abstract
      def eat(self): pass
      def sleep(self): return 'FOO.SLEEP'
    class Bar(Foo):
      def __init__(self, x, y): assert 0, 'never Bar'
      def drink(self): return 'BAR.DRINK'
    class Baz(Bar):
      def __init__(self, x, y): self.answer = x * y * 10
      def eat(self): return 'BAZ.EAT'
      def sleep(self): return 'BAZ.SLEEP'
    self.assertEquals(42, Bar.answer)
    baz = Baz(6, 7)
    self.assertEquals(420, baz.answer)
    self.assertEquals(Baz, type(baz))
    self.assertEquals('BAZ.EAT', baz.eat())
    self.assertEquals('BAR.DRINK', baz.drink())
    self.assertEquals('BAZ.SLEEP', baz.sleep())
    self.assertEquals(pobjects._PObjectMeta, type(Baz))
    self.assertRaisesWithLiteralMatch(
        pobjects.BadInstantiation,
        'Cannot instantiate abstract class __main__.Bar because it has '
        '@abstract method __main__.Foo.eat', Bar)
    self.assertRaisesWithLiteralMatch(
        pobjects.BadInstantiation,
        'Cannot instantiate abstract class __main__.Foo because it has '
        '@abstract method __main__.Foo.eat', Foo)

    class Foo:
      answer = 41
      @abstract
      def __init__(self, x, y): self.answer = x * y
    class Bar1(Foo):
      @abstract
      def __init__(self, x, y): assert 0, 'never Bar1'
    class Baz1(Foo):
      @abstract
      def __init__(self, x, y): assert 0, 'never Baz1'
    class Bar2(Foo):
      def __init__(self, x, y): self.answer = x * y * 10
    class Bar3(Foo):
      pass  # This cancels the abstractness of Foo.
    self.assertEquals(41, Foo.answer)
    self.assertEquals(41, Bar1.answer)
    self.assertEquals(41, Bar2.answer)
    bar2 = Bar2(6, 7)
    self.assertEquals(420, bar2.answer)
    bar3 = Bar3(6, 7)
    self.assertEquals(42, bar3.answer)
    self.assertEquals(Bar2, type(bar2))
    self.assertEquals(pobjects._PObjectMeta, type(Bar2))
    self.assertRaisesWithLiteralMatch(
        pobjects.BadInstantiation,
        'Cannot instantiate abstract class __main__.Bar1', Bar1)
    self.assertRaisesWithLiteralMatch(
        pobjects.BadInstantiation,
        'Cannot instantiate abstract class __main__.Foo', Foo)

  def testAbstractMultipleInheritance(self):
    class ParentOne(object):
      @abstract
      def foo(self): pass
    class ParentTwo(object):
      value = 41
      def __init__(self): self.value = 42
      def bar(self): return 'PARENTTWO.BAR'
    class Child(ParentOne, ParentTwo):
      def foo(self): return 'CHILD.FOO'
    class Child2(ParentOne, ParentTwo):
      def __init__(self):
        self.c = 56
        super(Child2, self).__init__()
      def foo(self): return 'CHILD2.FOO'
    child = Child()
    self.assertEquals('PARENTTWO.BAR', child.bar())
    self.assertEquals('CHILD.FOO', child.foo())
    self.assertEquals(42, child.value)
    child2 = Child2()
    self.assertEquals('PARENTTWO.BAR', child2.bar())
    self.assertEquals('CHILD2.FOO', child2.foo())
    self.assertEquals(42, child2.value)
    self.assertEquals(56, child2.c)

  def testAbstractMultipleInheritanceOrigInit(self):
    class ParentOne(object):
      p1 = 31
      def __init__(self):
        super(ParentOne, self).__init__()  # This calls ParentTwo.__init__.
        self.p1 = 32
      @abstract
      def foo(self): pass
    class ParentTwo(object):
      value = 41
      def __init__(self): self.value = 42
      def bar(self): return 'PARENTTWO.BAR'
    class Child(ParentOne, ParentTwo):
      def foo(self): return 'CHILD.FOO'
    child = Child()
    self.assertEquals('PARENTTWO.BAR', child.bar())
    self.assertEquals('CHILD.FOO', child.foo())
    self.assertEquals(32, child.p1)
    self.assertEquals(42, child.value)

  def testMini(self):
    class M:
      def F(): pass
    class N(M):
      @override
      def F(): pass

    assert type(N) is type

    class E(Exception):
      @final
      def Eoo(): pass

    self.assertCompileError(
        pobjects.BadClass,
        'Cannot create class testmod.B because method Foo overrides @final '
        'method testmod.A.Foo.', r"""if 1:
        class A:
          #__metaclass__ = type  # TODO(pts): Test this.
          @final
          def Foo(): pass
        assert type(A) is not type
        class B(A):
          def Foo(): pass
        """)


if __name__ == '__main__':
  unittest.main()
