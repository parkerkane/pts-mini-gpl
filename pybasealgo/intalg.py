#! /usr/bin/python2.6

"""Number theory and other integer algorithms in pure Python 2.x.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

Works with Python 2.4, 2.5, 2.6 and 2.7. Developed using Python 2.6. Doesn't
work with Python 3.x.

Functions in this module don't use floating point calculations unless the
docstring of the function mentions float.

This module assumes that `/' on integers rounds down. (Not true in Python
3.x.)

Python has arbitrary precision integers built-in: the long type.

Python has arbitrary precision rationals: fractions.Fraction.

Python has arbitrary precision decimal floating point numbers:
decimal.Decimal. The target precision has to be set before the operations,
the default is 28 digits after the dot.

Python has modular exponentiation built-in: pow(a, b, c).
"""

# TODO(pts): Add more unit tests.
# TODO(pts): Try http://pypi.python.org/pypi/NZMATH/1.1.0
# TODO(pts): Document https://github.com/epw/pyfactor/blob/master/factor.c
# TODO(pts): Pollard Rho and Pollard p-1 in sympy (sympy/ntheory/factor_.py)
# TODO(pts): Use C extensions (or sympy etc.), if available, for faster operation (especially prime factorization and prime tests).
# TODO(pts): perfect_power etc. in sympy/ntheory/factor_.py
# TODO(pts): Copy fast gcd from pyecm.py. Is it that fast?
#   Maybe faster in C, but not in Python.
#   http://en.wikipedia.org/wiki/Binary_GCD_algorithm
# TODO(pts): Use faster C modules for all algorithms, e.g.
#   C code for prime factorization etc. Use gmpy, python-gmp,
#   numpy, sympy, sage (?), scipy. Example:
#   from gmpy import gcd, invert, mpz, next_prime, sqrt, root.
# TODO(pts): Add invert (modular inverse) from pyecm. Isn't that too long?
# TODO(pts): pyecm.py has a strange next_prime implementation. Is it faster?
#   How does it work? Copy it. Does it contain an inlined Rabin-Miller test?

__author__ = 'pts@fazekas.hu (Peter Szabo)'

import array
import bisect
import _random
import struct


_HEX_BIT_COUNT_MAP = {
    '0': 0, '1': 1, '2': 2, '3': 2, '4': 3, '5': 3, '6': 3, '7': 3}


def bit_count(a):
  """Returns the number of bits needed to represent abs(a). Returns 1 for 0."""
  if not isinstance(a, (int, long)):
    raise TypeError
  if not a:
    return 1
  # Example: hex(-0xabc) == '-0xabc'. 'L' is appended for longs.
  # TODO(pts): Would s = bin(a) be faster?
  # See also
  # http://stackoverflow.com/questions/2654149/count-bits-of-a-integer-in-python/13663234
  s = hex(a)
  d = len(s)
  if s[-1] == 'L':
    d -= 1
  if s[0] == '-':
    d -= 4
    c = s[3]
  else:
    d -= 3
    c = s[2]
  return _HEX_BIT_COUNT_MAP.get(c, 4) + (d << 2)


# Use int.bit_length and long.bit_length introduced in Python 2.7 and 3.x.
if getattr(0, 'bit_length', None):
  __doc = bit_count.__doc__
  def bit_count(a):
    return a.bit_length() or 1
  bit_count.__doc__ = __doc


def sqrt_floor(n):
  """Returns like int(math.sqrt(n)), but correct for very large n.

  Uses Newton-iteration.
  """
  # This is like Newton-iteration, but all divisions are rounding down to
  # the nearest integer, so it handles +-1 fluctuations caused by the
  # rounding errors.
  if n < 4:
    if n < 0:
      raise ValueError('Negative input for square root.')
    if n:
      return 1
    else:
      return 0
  #if n < (1 << 328):
  #  # This is a bit faster than computing bit_count(n), but checking the
  #  # condition is slow.
  #  low = 2
  #  high = n >> 1
  b = a = 1 << ((bit_count(n) - 1) >> 1)  # First approximation.
  a = (a + n / a) >> 1; c = a; a = (a + n / a) >> 1
  while a != b:
    b = a; a = (a + n / a) >> 1; c = a; a = (a + n / a) >> 1
  if a < c:
    return int(a)
  else:
    return int(c)


def root_floor(n, k):
  """Finds the floor of the kth root of n.

  Based on the root(...) function in pyecm.py, which is based on gmpy's root(...)
  function.

  Uses Newton-iteration.

  For k == 2, uses exactly the same algorithm as sqrt_floor, and it's only a
  tiny bit slower.

  Returns:
    A tuple (r, is_exact), where r is the floor of the kth root of n, and
    is_exact is a bool indicating whether the root is exact, i.e. r ** k == n.
  """
  if k <= 0 or n < 0:
    raise ValueError
  if k == 1 or n <= 1:
    return int(n), True
  if k == 2:  # Fast Newton-iteration to compute the square root.
    #if n < (1 << 328):
    #  # This is a bit faster than computing bit_count(n), but checking the
    #  # condition is slow.
    #  low = 2
    #  high = n >> 1
    b = a = 1 << ((bit_count(n) - 1) >> 1)  # First approximation.
    a = (a + n / a) >> 1; c = a; a = (a + n / a) >> 1
    while a != b:
      b = a; a = (a + n / a) >> 1; c = a; a = (a + n / a) >> 1
    if a < c:
      return int(a), a * a == n
    else:
      return int(c), c * c == n
  if not (n >> k):  # 2 <= n < 2 ** k
    return 1, False
  # Now: n >= 2, k >= 3, n >= 2 ** k >= 8.

  # Use the base-2 length of n to narrow the [low, high) interval:
  # the return value will have about bit_count(n) / k bits.
  c = bit_count(n)
  b = (c - 1) / k
  if b > 1:
    # True: assert 2 <= 1 << b < 2 << b <= (n >> 2) + 1
    low = 1 << b
    high = 2 << b
  else:
    low = 2
    high = 4 - (c <= 4)  # c == 4 iff 8 <= n <= 15.

  # Use Newton-iteration to quickly narrow the [low, high) interval.

  x = low
  k1 = k - 1
  # Newton-iteration for f(x)  = x ** k - n:
  # y = x - f(x) / f'(x) = x - (x ** k - n) / (k * x ** (k - 1)) =
  #   = x - x / k + n / x ** (k - 1) / k ==
  #   = (x * (k - 1) + n / x ** (k - 1)) / k
  # Our y is less accurate, because the division truncates.
  # TODO(pts): Try rounding up -- or proper rounding. Does it speed it up?
  y = (k1 * x + n / x ** k1) / k
  # For example: with root_floor(3 ** 1000, 1000), y is insanely large, but
  # high is just 4, so fit y.
  if y < low or y >= high:
    y = (low + high) >> 1
  while True:
    z = (k1 * y + n / y ** k1) / k
    mr = cmp(z ** k, n)
    if mr < 0:
      if low < z:  # It seems to be always `low <= z', but we can't prove it.
        low = z
    elif mr:
      if high > z:  # z <= high is not always true.
        high = z
    else:
      return int(z), True
    if abs(z - y) >= abs(x - y):  # We can't make any more improvements.
      # Maybe z is the correct return. Adjust `high', because we couldn't do
      # that because of the roundings.
      if y == z and high > z + 1 and (z + 1) ** k > n:
        high = z + 1
      break
    x, y = y, z

  # Do a binary search as fallback if Newton-iteration wasn't accurate enough.
  # Usually low and high are very close to each other by now, and we have to
  # do only a few steps.

  # Now (and after each iteration) `low <= root(n, k) < high', which is
  # equivalent to `low <= floor(root(n, k)) < high', which is equivalent to
  # `low <= retval < high'. So we can stop the loop iff high == low + 1.
  while high > low + 1:
    # True: assert low ** k <= n < high ** k
    mid = (low + high) >> 1
    # True: assert low < mid < high
    mr = cmp(mid ** k, n)
    if mr < 0:
      low = mid
    elif mr:
      high = mid
    else:
      return int(mid), True
  return int(low), False


A0 = array.array('b', (0,))
"""Helper for primes_upto."""

A1 = array.array('b', (1,))
"""Helper for primes_upto."""


def primes_upto(n):
  """Returns a list of prime numbers <= n using the sieve algorithm.

  Please note that it uses O(n) memory for the returned list, and O(n) (a
  list of at most n / 2 items) for a temporary bool array.
  """
  # Based on http://stackoverflow.com/a/3035188/97248
  # Made it 7.04% faster and use much less memory by using array('b', ...)
  # instead of lists.
  #
  # TODO(pts): Use numpy if available (see the solution there).
  if n <= 1:
    return []
  n += 1
  s = A1 * (n >> 1)
  a0 = A0
  for i in xrange(3, sqrt_floor(n) + 1, 2):
    if s[i >> 1]:
      ii = i * i
      # itertools.repeat makes it slower by about 12%.
      s[ii >> 1 : : i] = a0 * ((((n - ii - 1) / i) >> 1) + 1)
  s = [(i << 1) | 1 for i in xrange(n >> 1) if s[i]]
  s[0] = 2  # Change from 1.
  return s


def yield_primes():
  """Yields all primes (indefinitely).

  Uses O(sqrt(n)) memory to yield the first n primes.
  """
  # Based on the postponed sieve implementation by Will Ness on
  # http://stackoverflow.com/a/10733621/97248 .
  yield 2; yield 3; yield 5; yield 7
  d = {}
  c = 9
  ps = yield_primes()
  p = ps.next() and ps.next()  # 3.
  q = p * p                    # 9.
  while 1:
    if c not in d:
      if c < q:
        yield c
      else:
        s = 2 * p
        x = c + s
        while x in d:
          x += s
        d[x] = s
        p = ps.next()
        q = p * p
    else:
      s = d.pop(c)
      x = c + s
      while x in d:
        x += s
      d[x] = s
    c += 2


def yield_primes_upto(n):
  """Yields primes <= n.

  Slower (by a factor of about 5.399) than primes_upto(...), but uses only
  O(sqrt(pi(n))) memory (instead of O(n)), where pi(n) is the number of
  primes <= n.
  """
  # TODO(pts): Add some kind of caching variant.
  #
  # itertools.takewhile(lambda x: x < n, yield_primes()) was tried, but it
  # 1.419% (slightly) faster.
  for p in yield_primes():
    if p > n:
      break
    yield p


def yield_first_primes(i):
  """Yields the first i primes.

  Uses O(sqrt(i)) temporary memory, because it uses yield_primes().
  """
  # TODO(pts): Add a variant based on primes_upto, possibly
  # doing a bit more than sqrt(i) sieve steps.
  y = yield_primes()
  for i in xrange(0, i):
    yield y.next()


def yield_composites():
  """Yields composite numbers in increasing order from 4."""
  n = 4
  for p in yield_primes():
    while n < p:
      yield n
      n += 1
    n = p + 1


# struct.pack('>257H', *([0] + [int(math.ceil(math.log(i)/math.log(2)*256)) for i in xrange(1, 257)]))
LOG2_256_TABLE = struct.unpack('>257H',
    '\x00\x00\x00\x00\x01\x00\x01\x96\x02\x00\x02S\x02\x96\x02\xcf\x03\x00\x03'
    ',\x03S\x03v\x03\x96\x03\xb4\x03\xcf\x03\xe9\x04\x00\x04\x17\x04,\x04@\x04'
    'S\x04e\x04v\x04\x87\x04\x96\x04\xa5\x04\xb4\x04\xc2\x04\xcf\x04\xdc\x04'
    '\xe9\x04\xf5\x05\x00\x05\x0c\x05\x17\x05"\x05,\x056\x05@\x05J\x05S\x05\\'
    '\x05e\x05n\x05v\x05~\x05\x87\x05\x8e\x05\x96\x05\x9e\x05\xa5\x05\xad\x05'
    '\xb4\x05\xbb\x05\xc2\x05\xc9\x05\xcf\x05\xd6\x05\xdc\x05\xe2\x05\xe9\x05'
    '\xef\x05\xf5\x05\xfb\x06\x00\x06\x06\x06\x0c\x06\x11\x06\x17\x06\x1c\x06"'
    '\x06\'\x06,\x061\x066\x06;\x06@\x06E\x06J\x06N\x06S\x06X\x06\\\x06a\x06e'
    '\x06i\x06n\x06r\x06v\x06z\x06~\x06\x82\x06\x87\x06\x8b\x06\x8e\x06\x92'
    '\x06\x96\x06\x9a\x06\x9e\x06\xa2\x06\xa5\x06\xa9\x06\xad\x06\xb0\x06\xb4'
    '\x06\xb7\x06\xbb\x06\xbe\x06\xc2\x06\xc5\x06\xc9\x06\xcc\x06\xcf\x06\xd2'
    '\x06\xd6\x06\xd9\x06\xdc\x06\xdf\x06\xe2\x06\xe6\x06\xe9\x06\xec\x06\xef'
    '\x06\xf2\x06\xf5\x06\xf8\x06\xfb\x06\xfe\x07\x00\x07\x03\x07\x06\x07\t'
    '\x07\x0c\x07\x0f\x07\x11\x07\x14\x07\x17\x07\x1a\x07\x1c\x07\x1f\x07"\x07'
    '$\x07\'\x07)\x07,\x07/\x071\x074\x076\x079\x07;\x07>\x07@\x07B\x07E\x07G'
    '\x07J\x07L\x07N\x07Q\x07S\x07U\x07X\x07Z\x07\\\x07^\x07a\x07c\x07e\x07g'
    '\x07i\x07k\x07n\x07p\x07r\x07t\x07v\x07x\x07z\x07|\x07~\x07\x80\x07\x82'
    '\x07\x85\x07\x87\x07\x89\x07\x8b\x07\x8d\x07\x8e\x07\x90\x07\x92\x07\x94'
    '\x07\x96\x07\x98\x07\x9a\x07\x9c\x07\x9e\x07\xa0\x07\xa2\x07\xa3\x07\xa5'
    '\x07\xa7\x07\xa9\x07\xab\x07\xad\x07\xae\x07\xb0\x07\xb2\x07\xb4\x07\xb6'
    '\x07\xb7\x07\xb9\x07\xbb\x07\xbd\x07\xbe\x07\xc0\x07\xc2\x07\xc3\x07\xc5'
    '\x07\xc7\x07\xc9\x07\xca\x07\xcc\x07\xce\x07\xcf\x07\xd1\x07\xd2\x07\xd4'
    '\x07\xd6\x07\xd7\x07\xd9\x07\xdb\x07\xdc\x07\xde\x07\xdf\x07\xe1\x07\xe2'
    '\x07\xe4\x07\xe6\x07\xe7\x07\xe9\x07\xea\x07\xec\x07\xed\x07\xef\x07\xf0'
    '\x07\xf2\x07\xf3\x07\xf5\x07\xf6\x07\xf8\x07\xf9\x07\xfb\x07\xfc\x07\xfe'
    '\x07\xff\x08\x00')


def log2_256_more(a):
  """Returns a nonnegative integer at least 256 * log2(a).

  TODO(pts): How close is this upper bound?

  Input: a is an integer >= 0.
  """
  if not isinstance(a, (int, long)):
    raise TypeError
  if a < 0:
    raise ValueError
  if a <= 1:
    return 0
  if a < 256:
    return LOG2_256_TABLE[a]
  d = bit_count(a) - 8
  m = a >> d
  return LOG2_256_TABLE[m + (m << d != a)] + (d << 8)


def log_more(a, b):
  """Returns a nonnegative integer at least a * ln(b).

  Input: a and b are integers, b >= 0.

  If b == 2 and a < 10 ** 16, then the result is the smallest possible.
  (TODO(pts): Verify this claim, especially the 16.)

  For larger values of b, the returned value might be off by a factor of 1/256.
  (TODO(pts): Verify this claim, especially relating to math.log(2) / 256
  approximation.)
  """
  if not isinstance(a, (int, long)):
    raise TypeError
  if not isinstance(b, (int, long)):
    raise TypeError
  if b < 0:
    raise ValueError
  if a <= 0 or b == 1:
    return 0
  if b == 2:
    # TODO(pts): Do it with smaller numbers for small values of a.
    return int((a * 69314718055994531 + 99999999999999999) / 100000000000000000)

  # Maple evalf(log(2) / 256, 30): 0.00270760617406228636491106297445
  # return a * log2_256_more(b) / 256.0 * math.log(2)
  # TODO(pts): Be more accurate with a different algorithm.
  return (a * log2_256_more(b) * 27076062 + 9999999999) / 10000000000


FIRST_PRIMES = (  # The first 54 primes, i.e. primes less than 256.
    '\x02\x03\x05\x07\x0b\r\x11\x13\x17\x1d\x1f%)+/5;=CGIOSYaegkmq\x7f\x83'
    '\x89\x8b\x95\x97\x9d\xa3\xa7\xad\xb3\xb5\xbf\xc1\xc5\xc7\xd3\xdf\xe3'
    '\xe5\xe9\xef\xf1\xfb')
FIRST_PRIMES_MAX = ord(FIRST_PRIMES[-1])


def first_primes_moremem(i):
  """Returns a list containing the first i primes.

  About 9.1% faster than yield_first_primes(i), but it uses about twice as
  much memory for a few primes which it will ignore soon.
  """
  if i <= len(FIRST_PRIMES):
    return map(ord, FIRST_PRIMES[:i])
  # Make n be an integer at least n * (math.log(n) + math.log(math.log(n))),
  # as given by p_i <= i * ln(i) + i * ln(ln(i)) if i >= 6, based on
  # http://en.wikipedia.org/wiki/Prime-counting_function#Inequalities
  s = primes_upto(log_more(i, log_more(i, i)))
  del s[i:]
  return s


def first_primes(i):
  """Returns a list containing the first i primes.

  Faster than yield_first_primes(i), but it uses more memory.
  """
  if i <= len(FIRST_PRIMES):
    return map(ord, FIRST_PRIMES[:i])
  # Make n be an integer at least n * (math.log(n) + math.log(math.log(n))),
  # as given by p_i <= i * ln(i) + i * ln(ln(i)) if i >= 6, based on
  # http://en.wikipedia.org/wiki/Prime-counting_function#Inequalities
  n = log_more(i, log_more(i, i))
  # The rest is equivalent to this, but the res saves memory by not creating
  # a long temporary list, and it's about 9.1% slower.
  #
  #   return primes_upto(n)[:i]
  n += 1
  s = A1 * (n >> 1)
  a0 = A0
  for j in xrange(3, sqrt_floor(n) + 1, 2):
    if s[j >> 1]:
      ii = j * j
      s[ii >> 1 : : j] = a0 * ((((n - ii - 1) / j) >> 1) + 1)
  t = []
  for j in xrange(n >> 1):  # This is the slow loop.
    if s[j]:
      t.append((j << 1) | 1)
      if len(t) == i:
        t[0] = 2  # Change from 1.
        return t
  assert 0, 'Too few primes: i=%d n=%d len(t)=%d' % (i, n, len(t))


def gcd(a, b):
  """Returns the greatest common divisor of integers a and b.

  If both a and b are 0, then it returns 0.

  Similar to fractions.gcd, but works for negatives as well.
  """
  if a < 0:
    a = -a
  if b < 0:
    b = -b
  while b:
    a, b = b, a % b
  return a


def fraction_to_float(a, b):
  """Converts the integer fraction a / b to a nearby float.

  This works even if float(a) and float(b) are too large.

  Please note that there may be a loss of precision because of the float
  return value.

  This works even in Python 2.4.

  Raises:
    ZeroDivisionError: If b == 0.
    OverflowError: If the result cannot be represented as a float.
  """
  # TODO(pts): Would this make it work faster?
  #   g = gcd(a, b)
  #   if g > 1:
  #     a /= g
  #     b /= g
  #
  # Smart implementation details in long_true_divide in Objects/longobject.c.
  return a.__truediv__(b)


def is_prime(n, accuracy=None):
  """Returns wheter the integer n is probably a prime.

  Uses the deterministic Rabin-Miller primality test if accuracy is None, and
  a probabilistic version (with a fixed set of ``pseudo-random'' primes) of
  the same test if accuracy is an integer. Uses an optimized (i.e. small) list
  of bases for the deterministic case if n <= 1 << 64.

  Please note that the correctness of the deterministic Rabin-Miller
  primality test depends on the validity of the generalized Riemann
  hypothesis for quadratic Dirichlet characters. This has not been proven
  yet. This affects us when n > 1 << 64 and accuracy is None are both true.

  Args:
    n: The integer whose primality is to be tested.
    accuracy: None for absolutely sure results or a nonnegative integer
      describing the desired accuracy (see more in the `Returns:' section).
      Please note that accuracy is ignored (i.e. assumed to be None) if n <=
      1 << 64. So the result for small values of n is always correct. If
      accuracy <= 7, then 7 is used instead.
  Returns:
    False if n is composite or -1 <= n <= 1; True if n is a prime; 1 if n is a
    prime with probability at least 1 - 4.0 ** -accuracy. If accuracy is None
    or n <= 1 << 64, then `1' is never returned, i.e. the result is always
    correct.
  """
  if n < 2:
    if n > -2:
      return False
    n = -n
  if not (n & 1):
    return n == 2  # n == 2 is prime, other even numbers are composite.
  is_accurate = True
  # These bases were published on http://miller-rabin.appspot.com/ . Suboptimal
  # (i.e. containing more bases) base lists are also at
  # http://en.wikipedia.org/wiki/Miller%E2%80%93Rabin_primality_test#Deterministic_variants_of_the_test
  if n < 1373653:
    if n == 3:
      return True  # n == 3 is prime.
    bases = (2, 3)
  elif n < 316349281:
    # 11000544 == 2**5 * 3 * 19 * 37 * 163
    # 31481107 == 7 * 241 * 18661
    # This is already checked above, so early return is not needed.
    # if n in (3, 7, 19, 37, 163, 241, 18661):
    #   return True
    bases = (11000544, 31481107)
  elif n < 105936894253:
    # 1005905886 == 2 * 3 * 113 * 1483637
    # 1340600841 == 3**3 * 17 * 19 * 347 * 443
    # 1483637 < 316349281, so early return is not needed.
    bases = (2, 1005905886, 1340600841)
  elif n < 31858317218647:
    # 3046413974 < 105936894253, so early return is not needed.
    bases = (2, 642735, 553174392, 3046413974)
  elif n < 3071837692357849:
    # 3613982119 < 31858317218647, so early return is not needed.
    bases = (2, 75088, 642735, 203659041, 3613982119)
  elif n < 18446744073709551617:  # About (1 << 64).
    # 1795265022 < 3071837692357849, so early return is not needed.
    bases = (2, 325, 9375, 28178, 450775, 9780504, 1795265022)
  elif accuracy is None:
    # According to
    # http://en.wikipedia.org/wiki/Miller%E2%80%93Rabin_primality_test#Deterministic_variants_of_the_test
    # it is enough to go up to min(n - 1, 2 * math.log(n) ** 2), but
    # the 2nd part of the min is already smaller when n >= 18. The 2nd part of
    # the min is 178 for n == (1 << 64 | 1). That's a steep slowdown from the
    # at most 7 elements in bases for smaller values of n.
    #
    # Please note that this test categorizes n == 5 and n == 7 as primes, but
    # we have better tests above for such smaller numbers.
    #
    # Please note that the correctness of this tests depends on the validity
    # of the generalized Riemann hypothesis for quadratic Dirichlet
    # characters. This has not been proven yet.
    # TODO(pts): Check this.
    bases = xrange(2, 1 + (log_more(log_more(65536, n), n) >> 15))
  else:
    is_accurate = False
    if accuracy <= len(FIRST_PRIMES):  # 54.
      bases = map(ord, FIRST_PRIMES[:max(7, accuracy)])
    else:
      bases = first_primes_moremem(accuracy)

  n1 = n - 1
  #: assert n1 > 1
  s = 0
  while not (n1 & (1 << s)):
    s += 1
  if s:
    h = (1, n1)
    n2 = n1 >> s
    for b in bases:
      a = pow(b, n2, n)
      if a not in h:
        a = (a * a) % n
        for _ in xrange(s - 1):
          if a in h:
            break
          a = (a * a) % n
        if a != n1:
          return False  # n is composite.
  if is_accurate:
    return True  # n is a prime.
  return 1  # n is probably prime: P(n is prime) >= 1 - 4.0 ** -accuracy.


def next_prime(n):
  """Returns the smallest positive prime larger than n."""
  if n <= 1:
    return 2
  elif n < FIRST_PRIMES_MAX:
    return ord(FIRST_PRIMES[bisect.bisect_right(FIRST_PRIMES, chr(n))])
  else:
    n += 1 + (n & 1)
    k = n % 6
    if k == 3:
      n += 2
    if is_prime(n):
      return n
    if k != 1:
      n += 2
      if is_prime(n):
        return n
    n += 4
    # This below is the same as the end of nextprime() sympy-0.7.2. There is
    # no such function in pycrypto.
    while 1:
      if is_prime(n):
        return n
      n += 2
      if is_prime(n):
        return n
      n += 4

#ps = primes_upto(100000)
#i = 0
#for x in xrange(-42, 100001):
#  if x == ps[i]:
#    i += 1
#    if i == len(ps):
#      break
#  y = next_prime(x)
#  assert y == ps[i], (ps[i], y, x)


def choose(n, k):
  """Returns the binomial coefficient n choose k."""
  if not isinstance(n, (int, long)):
    raise TypeError
  if not isinstance(k, (int, long)):
    raise TypeError
  if n < 0:
    raise ValueError
  if k < 0 or k > n:
    return 0
  if k > n >> 1:
    k = n - k
  if k:
    c = 1
    n += 1
    for i in xrange(1, k + 1):
      c = c * (n - i) / i
    return c
  else:
    return 1


def yield_slow_factorize(n):
  """Yields an increasing list of prime factors whose product is n.

  The implementation is slow because it uses trial division. Please use
  factorize instead, which is fast.

  Among the factorization algorithms using trial division, this implementation is
  pretty fast.
  """
  if n <= 0:
    raise ValueError
  s = 0
  while not (n & (1 << s)):
    yield 2
    s += 1
  n >>= s
  while not (n % 3):
    yield 3
    n /= 3
  d = 5
  q = sqrt_floor(n)
  # We have quite a bit of code repetition below just so that we can have
  # d += 4 every second time instead of d += 2.
  while d <= q:
    if not (n % d):
      yield d
      n /= d
      while not (n % d):
        yield d
        n /= d
      q = sqrt_floor(n)
    d += 2
    if d > q:
      break
    if not (n % d):
      yield d
      n /= d
      while not (n % d):
        yield d
        n /= d
      q = sqrt_floor(n)
    d += 4
  if n > 1:
    yield n


def divisor_count(n):
  if n <= 0:
    raise ValueError
  q = 0
  e = 0
  c = 1
  for p in factorize(n):
    if p == q:
      e += 1
    else:
      c *= e + 1
      q = p
      e = 1
  return c * (e + 1)


def rle(seq):
  """Does run-length encoding.

  E.g. transforms 'foo' ,'bar', 'bar', 'baz'...
  to ('foo', 1), ('bar', 2), ('baz', 1), ...
  """
  prev = count = 0  # The value of prev is arbitrary here.
  for x in seq:
    if not count:
      prev = x
      count = 1
    elif prev == x:
      count += 1
    else:
      yield prev, count
      prev = x
      count = 1
  if count:
    yield prev, count


def divisor_sum(n):
  """Returns the sum of positive integer divisors of n (including 1 and n)."""
  if not isinstance(n, (int, long)):
    raise TypeError
  if n <= 0:
    raise ValueError
  s = 1
  for prime, exp in rle(factorize(n)):
    s *= (prime ** (exp + 1) - 1) / (prime - 1)
  return s


def simplify_nonneg(a, b):
  """Simplifies the nonnegative rational a / b. Returns (aa, bb)."""
  x, y = a, b
  while y:  # Calculate the GCD.
    x, y = y, x % y
  return a / x, b / x


class MiniIntRandom(_random.Random):
  """Class which can generate random integers, without using floating point.

  Uses Python's built-in Mersenne twister (MT19937).

  Constructor arguments: Pass an integer to specify the seed, or omit to use
  the current time as seed.
  """
  cached_bit_count = (None, None)

  def randrange(self, start, stop):
    """Returns a random integer in the range: start <= retval < stop."""
    n = stop - start
    if n <= 0:
      raise ValueError
    # TODO(pts): Measure the speed of this method vs random.Random.randrange.
    # Caching the result of bit_count(n), because bit counting can be
    # potentially slow.
    # TODO(pts): Measure the speed of caching.
    # TODO(pts): Measure how the random number generation speed affects the
    # factorization speed.
    cc, cn = self.cached_bit_count
    if cn == n:
      c = cc
    else:
      c = bit_count(n)
      self.cached_bit_count = (c, n)
    # TODO(pts): Come up with something faster.
    r = self.getrandbits(c)
    while r >= n:
      r = self.getrandbits(c)
    return r + start


def pollard(n, random_obj):
  """Try to find a non-trivial divisor of n using Pollard's Rho algorithm.

  This is similar but slower than brent(...). Please use brent(...) instead.

  This implementation is based on a merge of
  http://comeoncodeon.wordpress.com/2010/09/18/pollard-rho-brent-integer-factorization/
  and http://code.activestate.com/recipes/577037-pollard-rho-prime-factorization/
  , but several bugs were fixed.

  pycrypto doesn't contain code for integer factorization.
  SymPy (sympy/ntheory/factor_.py) contains Pollard Rho and Pollard p-1.

  Args:
    n: An integer >= 2 to find a non-trivial divisor of.
    random_obj: An object which can generate random numbers using the
      .randrange method (see Random.randrange for documentation); or None to
      create a default one using n as the seed. .randrange may be called several
      times, which modifies random_obj's internal state.
  Returns:
    For prime n: returns n quite slowly.

    For composite n: usually 2 <= retval < n and n % retval == 0; but sometimes
    retval == n, so no non-trivial divisors could be found.

    May return n even for small composite numbers (e.g. 9, 15, 21, 25, 27).
  """
  if n <= 1:
    raise ValueError
  if not (n & 1):
    return 2
  # Calling random_obj.randrange with the same argument to take advantage of
  # the bit count caching in MiniIntRandom.
  y = random_obj.randrange(1, n)
  c = random_obj.randrange(1, n)
  while c == n - 2:  # -2 is a bad choice for c.
    c = random_obj.randrange(1, n)
  x = y
  g = 1
  while g == 1:
    x = (x * x + c) % n
    y = (y * y + c) % n
    y = (y * y + c) % n
    g = gcd(abs(x - y), n)
    if g == n:
      break  # Failed, try again with a different c.
  return g


def brent(n, random_obj):
  """Try to find a non-trivial divisor of n using Brent's algorithm.

  This is similar but faster than pollard(...).

  This implementation is based on the merge of
  http://comeoncodeon.wordpress.com/2010/09/18/pollard-rho-brent-integer-factorization/
  and http://mail.python.org/pipermail/python-list/2009-November/559592.html and
  http://code.activestate.com/recipes/577037-pollard-rho-prime-factorization/ and
  http://forums.xkcd.com/viewtopic.php?f=11&t=97639
  , but several bugs were fixed and several speed improvements were made.

  pycrypto doesn't contain code for integer factorization. SymPy doesn't
  contain Brent's method.

  Args:
    n: An integer >= 2 to find a non-trivial divisor of.
    random_obj: An object which can generate random numbers using the
      .randrange method (see Random.randrange for documentation); or None to
      create a default one using n as the seed. .randrange may be called several
      times, which modifies random_obj's internal state.
  Returns:
    For prime n: returns n quite slowly.

    For composite n: usually 2 <= retval < n and n % retval == 0; but sometimes
    retval == n, so no non-trivial divisors could be found.

    May return n even for small composite numbers (e.g. 9, 15, 21, 25, 27).
  """
  if n <= 1:
    raise ValueError
  if not (n & 1):
    return 2
  # Calling random_obj.randrange with the same argument to take advantage of
  # the bit count caching in MiniIntRandom.
  y = random_obj.randrange(1, n)
  c = random_obj.randrange(1, n)
  while c == n - 2:  # -2 is a bad choice for c.
    c = random_obj.randrange(1, n)
  if c == n - 2:  # -2 is a bad choice for c.
    c = n - 1
  m = random_obj.randrange(1, n)
  g = r = q = 1
  while g == 1:
    x = y
    for i in xrange(r):
      # Some implementations use `y = (pow(y, 2, n) + c) % n' instead, but that's
      # about 3.65 times slower.
      y = (y * y + c) % n
    k = 0
    while k < r and g == 1:  # Always true in the beginning.
      ys = y
      for i in xrange(min(m, r - k)):
        # Assigning to y and q in parallel here (and at PA2 below) would
        # change the algorithm, but it appears that factorization using that
        # still produces correct result, but it is about 4.5% slower.
        y = (y * y + c) % n
        q = (q * abs(x - y)) % n
      g = gcd(q, n)
      k += m
    r <<= 1
  if g == n:
    while 1:
      ys = (ys * ys + c) % n  # PA2: ys, g = ...
      g = gcd(abs(x - ys), n)
      if g > 1:
        break  # At this point we may still return n.
  return int(g)


def finder_slow_factorize(n, divisor_finder=None, random_obj=None):
  """Factorize a number recursively, by finding divisors.

  This function is slow. Please use factorize(...) instead.

  This function is slow because it doesn't propagate prime divisors of d to
  n / d; and it also doesn't use trial divison to find small prime divisors
  quickly.

  Factorization uses pseudorandom numbers, but it's completely deterministic,
  because the random seed depends only on the arguments (n and random_obj).

  Args:
    n: Positive integer to factorize.
    divisor_finder: A function which takes a positive composite integer k and
      random_obj. Always returns k or a non-trivial divisor of k. Can use
      random. Not called for prime numbers. If None is passed, then a
      reasonable fast default is used (currently brent(...)).
    random_obj: An object which can generate random numbers using the
      .randrange method (see Random.randrange for documentation); or None to
      create a default one using n as the seed. .randrange may be called several
      times, which modifies random_obj's internal state.
  Returns:
    List of prime factors (with multiplicity), in increasing order.
  """
  if n < 1:
    raise ValueError
  if n == 1:
    return []
  if divisor_finder is None:
    divisor_finder = brent
  if random_obj is None:
    random_obj = MiniIntRandom(n)
  numbers_to_factorize = [n]
  ps = []  # Prime factors found so far (with multiplicity).
  while numbers_to_factorize:
    n = numbers_to_factorize.pop()
    while not is_prime(n):
      d = divisor_finder(n, random_obj)
      while d == n:  # Couldn't find a non-trivial divisor.
        d = divisor_finder(n, random_obj)  # Try again.
      numbers_to_factorize.append(d)
      n /= d
    ps.append(n)
  ps.sort()
  return ps


# Empty or contains primes 3, 5, ..., <= _SMALL_PRIME_LIMIT.
_small_primes_for_factorize = []

#def _compute_small_primes_for_factorize():
#  assert not small_primes_for_factorize
#  small_primes_for_factorize[:] = primes_upto(65536)

_SMALL_PRIME_LIMIT = 65536
# The larger `nextprime(_SMALL_PRIME_LIMIT) ** 2' would also work.
_SMALL_PRIME_CUTOFF = (_SMALL_PRIME_LIMIT + 1) ** 2


def factorize(n, divisor_finder=None, random_obj=None):
  """Returns an increasing list of prime factors whose product is n.

  All primes which fit to an int are added as an int (rather than a long).

  Factorization uses pseudorandom numbers, but it's completely deterministic,
  because the random seed depends only on the arguments (n and random_obj).

  This algorithm is not the fastest known integer factorization algorithm,
  but it represents a reasonable balance between speed and complexity. For a
  more complex, but faster method, see
  http://sourceforge.net/projects/pyecm/ , but for small numbers (< 10 **
  33) that's still faster than this algorithm. Pyecm uses Elliptic curve
  factorization method (ECM), which is the third-fastest known factoring method.
  The second fastest is the multiple polynomial quadratic sieve and the
  fastest is the general number field sieve.

  Args:
    n: Positive integer to factorize.
    divisor_finder: A function which takes a positive composite integer k and
      random_obj. Always returns k or a non-trivial divisor of k. Can use
      random. Not called for prime numbers. If None is passed, then a
      reasonable fast default is used (currently brent(...)).
    random_obj: An object which can generate random numbers using the
      .randrange method (see Random.randrange for documentation); or None to
      create a default one using n as the seed. .randrange may be called several
      times, which modifies random_obj's internal state.
  Returns:
    List of prime factors (with multiplicity), in increasing order.
  """
  if n <= 0:
    raise ValueError
  if n == 1:
    return []
  if is_prime(n):
    return [n]
  # We will extend ps with all prime factors of n (with multiplicity).
  # We will extend pds with all prime factors of n (each prime once).
  # We extend with int instead of long if the number fits in an int.
  ps = []
  pds = []

  # Trial division for small primes.

  if not _small_primes_for_factorize:
    # TODO(pts): Adjust _SMALL_PRIME_LIMIT.
    #
    # Depending on _SMALL_PRIME_LINIT, we have:
    #   1000:  168 primes,  p == 1/12.3509756739 == 0.0809652635068
    #   65536: 6542 primes, p == 1/19.7576704153 == 0.0506132544466
    #
    # p is the probability that we can't factorize n trying only
    # _small_primes_for_factorize (because n has only larger prime divisors
    # than _SMALL_PRIME_LIMIT).
    #
    # p was computed using:
    #
    #   a = b = 1
    #   for p in primes_upto(_SMALL_PRIME_LIMIT):
    #     a *= p - 1
    #     b *= p
    #   p = fraction_to_float(a, b)

    # This is thread-safe because of the global interpreter lock.
    _small_primes_for_factorize[:] = primes_upto(_SMALL_PRIME_LIMIT)[1:]
  q = sqrt_floor(n)
  if q >= 2 and not (n & 1):
    pds.append(2)
    ps.append(2)
    p = 1
    while not (n & (1 << p)):
      p += 1
      ps.append(2)
    n >>= p
    q = sqrt_floor(n)
  for p in _small_primes_for_factorize:
    if p > q:
      break
    if not (n % p):
      pds.append(p)
      ps.append(p)
      n /= p
      while not (n % p):
        ps.append(p)
        n /= p
      q = sqrt_floor(n)
  n = int(n)  # This is fast: O(1) for int and long.
  if n == 1:
    ps.sort()
    return ps
  if n < _SMALL_PRIME_CUTOFF:
    ps.append(n)  # n is prime, because we've tried all its prime divisors.
    ps.sort()
    return ps

  # Now n doesn't have any small divisors, so we continue with Brent's
  # algorithm (or the slower Pollard's Rho algorithm, if configured so) to
  # find a non-trivial divisor d. Since d is not necessarily a prime, we
  # recursively factorize d and n / d until we find primes. The implementation
  # below is a bit tricky, because it uses a stack instead of recursion to
  # avoid the Python stack overflow, and it also propagates prime divisors from
  # d to n / d, i.e. if it finds that p is a prime divisor of d, and then it
  # will try p first (i.e. before another call to Brent's algorithm) when
  # factorizing n / d.

  if divisor_finder is None:
    divisor_finder = brent
  if random_obj is None:
    random_obj = MiniIntRandom(n)
  stack = [(int(n), len(pds))]
  while stack:
    n, i = stack.pop()

    # Propagate prime divisors from d to n / d.
    #
    # It's not true that this time i starts where the last time this loop
    # finished. In fact, i may start earlier. So we have to keep the i values
    # in the stack.
    for i in xrange(i, len(pds)):
      p = int(pds[i])
      while not (n % p):
        ps.append(p)
        n /= p

    # True: assert gcd(product(ps), n) == 1.
    if n == 1:
      continue
    n = int(n)  # This is fast: O(1) for int and long.
    while 1:
      if n < _SMALL_PRIME_CUTOFF or is_prime(n):  # n is prime.
        ps.append(n)
        pds.append(n)
        break
      d = divisor_finder(n, random_obj)
      while d == n:
        d = divisor_finder(n, random_obj) # Retry with different random numbers.
      # True: assert 1 < d < n and n % d == 0
      n /= d
      n = int(n)
      d = int(d)
      # True: assert n > 1 and d > 1
      if d > n:
        stack.append((d, len(pds)))
      else:
        stack.append((n, len(pds)))
        n = d
  ps.sort()
  return ps


def totient(n):
  """Returns the Euler totient of a nonnegative integer.

  http://en.wikipedia.org/wiki/Euler%27s_totient_function

  Args:
    n: Integer >= 0.
  """
  if n == 0:
    return 0
  result = 1
  for p, a in rle(factorize(n)):
    result *= p ** (a - 1) * (p - 1)
  return result


def totients_upto(limit, force_recursive=False):
  """Computes the Euler totient of nonnegative integers up to limit.

  http://en.wikipedia.org/wiki/Euler%27s_totient_function

  Args:
    limit: Integer >= 0.
    force_recursive: bool indicating whether the recursive implementation
      should be forced.
  Returns:
    A list of length `limit + 1', whose value at index i is totient(i).
  """
  if limit <= 1:
    if limit:
      return [0, 1]
    else:
      return [0]
  if not force_recursive and limit <= 1800000:
    # For small values, _totients_upto_iter is faster. _totients_upto_iter
    # also uses double memory, so let's not use it for large values.
    return _totients_upto_iter(limit)
  primes = primes_upto(limit)
  primec = len(primes)
  result = [None] * (limit + 1)
  result[0] = 0
  result[1] = 1

  def generate(i, n, t):  # Populates `result'.
    #print 'generate(i=%d, n=%d, t=%d, p=%d)' % (i, n, t, primes[i])
    while 1:
      p = primes[i]
      i += 1
      n0, t0 = n, t
      n *= p
      t *= p - 1
      while n <= limit:
        # assert result[n] is None  # True but speed.
        result[n] = t
        if i < primec:
          generate(i, n, t)
        n *= p
        t *= p
      n, t = n0, t0
      if i >= primec or n * primes[i] > limit:
        break

  generate(0, 1, 1)
  # assert not [1 for x in result if x is None]  # True but speed.
  return result


def _totients_upto_iter(limit):
  """Like totients_upto, but iterative.

  Input: limit is an integer >= 1.
  """
  ns = [1]
  result = [None] * (limit + 1)
  result[0] = 0
  result[1] = 1
  limit_sqrt = sqrt_floor(limit) + 1
  primes = primes_upto(limit)
  limit_idx2 = 0
  for p in primes:
    if p > limit_sqrt:  # No sorting anymore.
      t = p - 1
      for i in xrange(limit_idx2):
        n = ns[i]
        pan = p * n
        if pan > limit:
          break
        #assert result[pan] is None, (p, n, pan)  # True but speed.
        result[pan] = result[n] * t
      continue
    ns.sort()  # Fast sort, because of lots of increasing runs.
    pa = p
    ta = p - 1
    limit_idx = len(ns)
    while pa <= limit:
      ns.append(pa)
      result[pa] = ta
      for i in xrange(1, limit_idx):
        n = ns[i]
        pan = pa * n
        if pan > limit:
          break
        ns.append(pan)
        result[pan] = ta * result[n]
      pa *= p
      ta *= p
    limit_idx2 = len(ns)
  return result


def divisor_counts_upto(limit):
  """Computes the number of divisors of nonnegative integers up to limit.

  Args:
    limit: Integer >= 0.
  Returns:
    A list of length `limit + 1', whose value at index i is the number of
    positive integers who divide i. At index 0, 0 is returned.
  """
  if limit <= 1:
    if limit:
      return [0, 1]
    else:
      return [0]
  primes = primes_upto(limit)
  primec = len(primes)
  result = [None] * (limit + 1)
  result[0] = 0
  result[1] = 1

  def generate(i, n, t):  # Populates `result'.
    #print 'generate(i=%d, n=%d, t=%d, p=%d)' % (i, n, t, primes[i])
    while 1:
      p = primes[i]
      i += 1
      n0, t0 = n, t
      n *= p
      t += t0
      while n <= limit:
        # assert result[n] is None  # True but speed.
        result[n] = t
        if i < primec:
          generate(i, n, t)
        n *= p
        t += t0
      n, t = n0, t0
      if i >= primec or n * primes[i] > limit:
        break

  generate(0, 1, 1)
  # assert not [1 for x in result if x is None]  # True but speed.
  return result
