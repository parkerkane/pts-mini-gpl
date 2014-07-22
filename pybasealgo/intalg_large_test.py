#! /usr/bin/python2.6

"""Slow unit tests for the intalg module."""

__author__ = 'pts@fazekas.hu (Peter Szabo)'

import unittest

import intalg


class IntalgSmokeTest(unittest.TestCase):
  def testSqrtFloor(self):
    for n in xrange(100000):
      r = intalg.sqrt_floor(n)
      rk = r * r
      assert rk <= n < (r + 1) ** 2, (n, r)

    for ni in xrange(500):
      n = int(37 + 100 * 1.5 ** ni)
      r = intalg.sqrt_floor(n)
      rk = r * r
      assert rk <= n < (r + 1) ** 2, (n, r)

  def testRootFloor(self):
    for k in (2, 3, 4, 5, 6, 7):
      for n in xrange(100000):
        r, is_exact = intalg.root_floor(n, k)
        rk = r ** k
        assert rk <= n < (r + 1) ** k
        assert (rk == n) == is_exact, (n, k, r, is_exact, rk)

    for k in (2, 3, 4, 5, 6, 7):
      for ni in xrange(500):
        n = int(37 + 100 * 1.5 ** ni)
        r, is_exact = intalg.root_floor(n, k)
        rk = r ** k
        assert rk <= n < (r + 1) ** k
        assert (rk == n) == is_exact, (n, k, r, is_exact, rk)

  def testFactorizeMedium(self):
    n = intalg.next_prime(intalg._SMALL_PRIME_LIMIT) ** 2 - 10
    limit = n + 20025
    while n < limit:  # xrange can't take a long (n).
      a = list(intalg.yield_slow_factorize(n))
      b = intalg.finder_slow_factorize(n)
      c = intalg.factorize(n)
      c = intalg.factorize(n, divisor_finder=intalg.brent)
      d = intalg.factorize(n, divisor_finder=intalg.pollard)
      assert a == b == c == d, (n, a, b, c, d)
      n += 1

  def testFactorizeSmall(self):
    for n in xrange(1, 100025):
      a = list(intalg.yield_slow_factorize(n))
      b = intalg.finder_slow_factorize(n)
      c = intalg.factorize(n, divisor_finder=intalg.brent)
      d = intalg.factorize(n, divisor_finder=intalg.pollard)
      assert a == b == c == d, (n, a, b, c, d)

  def testBrentPrime(self):
    random_obj = intalg.MiniIntRandom(42)
    for n in intalg.primes_upto(100000):
      b = intalg.brent(n, random_obj)
      self.assertEquals(b, n)

  def testPollardPrime(self):
    random_obj = intalg.MiniIntRandom(42)
    for n in intalg.primes_upto(100000):
      b = intalg.pollard(n, random_obj)
      self.assertEquals(b, n)

  def testBrentComposite(self):
    random_obj = intalg.MiniIntRandom(42)
    for n in intalg.yield_composites():
      if n > 100000:
        break
      b = intalg.brent(n, random_obj)
      assert b > 1
      assert b <= n
      self.assertEquals(0, n % b, (b, n))

  def testPollardComposite(self):
    random_obj = intalg.MiniIntRandom(42)
    for n in intalg.yield_composites():
      if n > 100000:
        break
      b = intalg.pollard(n, random_obj)
      assert b > 1
      assert b <= n
      self.assertEquals(0, n % b, (b, n))

  def testFactorizeLargePrimes(self):
    random_obj = intalg.MiniIntRandom(42)
    for ps in ([1073741827, 1152921504606847009],
               [34359738421, 1180591620717411303449],
               [7, 1763668414462081],
               [100003],
               [10000000019],
               [1000000000039],  # Slow but tolerable.
               [10000000019] * 9,
               [1000000007] * 10,
               [1000000007] + [1000000009] * 2 + [1000000021] * 3):
      n = 1
      for p in ps:
        n *= p
      divisors = set()  # All divisors based on ps, except for 1.
      for i in xrange(1, 1 << len(ps)):
        r = 1
        for j in xrange(0, len(ps)):
          if i & (1 << j):
            r *= ps[j]
        divisors.add(r)

      r = intalg.brent(n, random_obj)
      assert r in divisors
      r = intalg.pollard(n, random_obj)
      assert r in divisors
      sps = intalg.finder_slow_factorize(n)
      assert sps == ps, (n, sps, ps)
      sps = intalg.factorize(n)
      assert sps == ps, (n, sps, ps)

    #print intalg.brent(100000000000000000039)  # very slow
    #print intalg.brent(1000000000000037)  # very slow
    #print intalg.factorize(1000000000000037 ** 9)  # slow
    #print intalg.factorize(1000000000039 ** 9)  # slow


if __name__ == '__main__':
  unittest.main()
