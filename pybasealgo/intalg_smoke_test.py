#! /usr/bin/python2.6

"""Fast unit tests for the intalg module."""

__author__ = 'pts@fazekas.hu (Peter Szabo)'

import math
import unittest

import intalg


class IntalgSmokeTest(unittest.TestCase):
  def setUp(self):
    intalg.clear_prime_cache()

  def tearDown(self):
    intalg.clear_prime_cache()

  def testBitCount(self):
    self.assertEquals(1, intalg.bit_count(0))
    self.assertEquals(1, intalg.bit_count(1))
    self.assertEquals(2, intalg.bit_count(2))
    self.assertEquals(2, intalg.bit_count(3))
    self.assertEquals(6, intalg.bit_count(63))
    self.assertEquals(7, intalg.bit_count(64))
    self.assertEquals(7, intalg.bit_count(75))
    self.assertEquals(11, intalg.bit_count(2047))
    self.assertEquals(12, intalg.bit_count(2048))
    self.assertEquals(12, intalg.bit_count(-4007))
    self.assertEquals(12, intalg.bit_count(4095))
    self.assertEquals(13, intalg.bit_count(4096))
    self.assertEquals(1204, intalg.bit_count(1 << 1203))
    self.assertEquals(1204, intalg.bit_count(-(1 << 1203)))
    self.assertEquals(1205, intalg.bit_count(1 << 1204))
    self.assertEquals(1206, intalg.bit_count(1 << 1205))
    self.assertEquals(1207, intalg.bit_count(1 << 1206))

  def testLog256MoreTable(self):
    self.assertEquals(257, len(intalg.LOG2_256_MORE_TABLE))
    self.assertEquals(1762, intalg.LOG2_256_MORE_TABLE[0x76])
    self.assertEquals(1531, intalg.LOG2_256_MORE_TABLE[63])
    self.assertEquals(1536, intalg.LOG2_256_MORE_TABLE[64])
    self.assertEquals(1542, intalg.LOG2_256_MORE_TABLE[65])

  def testLog256More(self):
    self.assertEquals(1762, intalg.log2_256_more(0x76))  # Accurate.
    self.assertEquals(6882, intalg.log2_256_more(123456789))  # Accurate (by luck).
    self.assertEquals(2048, intalg.log2_256_more(256))
    self.assertEquals(2051, intalg.log2_256_more(257))  # 2050 is more accurate.
    self.assertEquals(2051, intalg.log2_256_more(258))
    self.assertEquals(2054, intalg.log2_256_more(259))
    self.assertEquals(123 << 8, intalg.log2_256_more(1 << 123))
    self.assertEquals(1203 << 8, intalg.log2_256_more(1 << 1203))
    self.assertEquals(1204 << 8, intalg.log2_256_more(1 << 1204))
    self.assertEquals(1205 << 8, intalg.log2_256_more(1 << 1205))
    self.assertEquals(1206 << 8, intalg.log2_256_more(1 << 1206))

  def testLog256LessTable(self):
    self.assertEquals(256, len(intalg.LOG2_256_LESS_TABLE))
    self.assertEquals(1761, intalg.LOG2_256_LESS_TABLE[0x76])
    self.assertEquals(1530, intalg.LOG2_256_LESS_TABLE[63])
    self.assertEquals(1536, intalg.LOG2_256_LESS_TABLE[64])
    self.assertEquals(1541, intalg.LOG2_256_LESS_TABLE[65])

  def testLog256Less(self):
    self.assertEquals(1761, intalg.log2_256_less(0x76))  # Accurate.
    self.assertEquals(6880, intalg.log2_256_less(123456789))  # 6681 is more accurate.
    self.assertEquals(2048, intalg.log2_256_less(256))
    self.assertEquals(2048, intalg.log2_256_less(257))  # Same as for 256, last bit ignored.
    self.assertEquals(2050, intalg.log2_256_less(258))
    self.assertEquals(2050, intalg.log2_256_less(259))  # Same as for 256, last bit ignored.
    self.assertEquals(123 << 8, intalg.log2_256_less(1 << 123))
    self.assertEquals(1203 << 8, intalg.log2_256_less(1 << 1203))
    self.assertEquals(1204 << 8, intalg.log2_256_less(1 << 1204))
    self.assertEquals(1205 << 8, intalg.log2_256_less(1 << 1205))
    self.assertEquals(1206 << 8, intalg.log2_256_less(1 << 1206))

  def testLogMore(self):
    b_coeff = 123457 * math.log(3)
    for i in xrange(100):
      a = intalg.log_more(123457, 3 ** i)
      b = b_coeff * i
      assert b <= a, (i, a, b)
      assert b + b / 256 >= a, (i, a, b)

  def testSqrtFloor(self):
    assert intalg.sqrt_floor(41 ** 6) == 41 ** 3
    assert intalg.sqrt_floor((41 ** 3 + 1) ** 2 - 1) == 41 ** 3
    assert intalg.sqrt_floor(10 ** 5000) == 10 ** 2500  # 10e5000 is float overflow.

    for n in xrange(100):
      r = intalg.sqrt_floor(n)
      rk = r * r
      assert rk <= n < (r + 1) ** 2, (n, r)

    for ni in xrange(100):
      n = int(37 + 100 * 1.5 ** ni)
      r = intalg.sqrt_floor(n)
      rk = r * r
      assert rk <= n < (r + 1) ** 2, (n, r)

  def testRootFloor(self):
    self.assertEquals((41 ** 3, True), intalg.root_floor(41 ** 6, 2))
    self.assertEquals((41 ** 2, True), intalg.root_floor(41 ** 6, 3))
    self.assertEquals((41, False), intalg.root_floor(42 ** 6 - 30 ** 6, 6))
    self.assertEquals((42 ** 2 - 1, False), intalg.root_floor(42 ** 6 - 13 ** 6, 3))
    self.assertEquals((10 ** 2500, True), intalg.root_floor(10 ** 5000, 2))  # 10e5000 > max_float.
    self.assertEquals((2, True), intalg.root_floor(2 ** 1000, 1000))
    self.assertEquals((3, True), intalg.root_floor(3 ** 1000, 1000))
    self.assertEquals((42 ** 3, True), intalg.root_floor(42 ** 9000, 3000))

    for k in (2, 3, 4, 5, 6, 7):
      for n in xrange(100):
        r, is_exact = intalg.root_floor(n, k)
        rk = r ** k
        assert rk <= n < (r + 1) ** k
        assert (rk == n) == is_exact, (n, k, r, is_exact, rk)

    for k in (2, 3, 4, 5, 6, 7):
      for ni in xrange(20):
        n = int(37 + 100000 * 3 ** ni)
        r, is_exact = intalg.root_floor(n, k)
        rk = r ** k
        assert rk <= n < (r + 1) ** k
        assert (rk == n) == is_exact, (n, k, r, is_exact, rk)

  def testFractionToFloat(self):
    self.assertEquals(32.5, intalg.fraction_to_float(2 ** 10005 + 2 ** 9999, 2 ** 10000))
    self.assertEquals(32.5, intalg.fraction_to_float(2 ** 10005 + 2 ** 9999 + 1, 2 ** 10000))

  def testPrimesUpto(self):
    self.assertEquals([2, 3, 5, 7, 11, 13, 17, 19], intalg.primes_upto(22))
    self.assertEquals([2, 3, 5, 7, 11, 13, 17, 19, 23], intalg.primes_upto(23))
    self.assertEquals([2, 3, 5, 7, 11, 13, 17, 19, 23], intalg.primes_upto(24))
    primes = intalg.primes_upto(100)
    self.assertEquals([1], intalg._prime_cache_limit_ary[:])
    self.assertEquals(None, intalg.prime_index(100))
    self.assertEquals(len(primes) - 1, intalg.prime_index(97))
    self.assertEquals(primes, intalg.primes_upto(100))
    self.assertEquals(primes, intalg.primes_upto(97))

    intalg._prime_cache[:] = [6, 77, 8]
    primes3 = intalg.primes_upto(100)
    self.assertEquals([6, 77, 8], primes3)  # Because of the fake _prime_cache.

  def testFirstPrimesMoremem(self):
    self.assertEquals([], intalg.first_primes_moremem(0))
    self.assertEquals([2], intalg.first_primes_moremem(1))
    self.assertEquals([2, 3, 5, 7, 11, 13, 17, 19],
                      intalg.first_primes_moremem(8))
    self.assertEquals([2, 3, 5, 7, 11, 13, 17, 19, 23],
                      intalg.first_primes_moremem(9))
    primes = intalg.first_primes_moremem(100)
    self.assertEquals(100, len(primes))
    self.assertEquals([1], intalg._prime_cache_limit_ary[:])
    self.assertEquals(len(primes) - 1, intalg.prime_index(primes[-1]))
    self.assertEquals([1024], intalg._prime_cache_limit_ary[:])
    primes2 = intalg.first_primes_moremem(100)
    self.assertEquals(primes, primes2)

    intalg._prime_cache[0] = 6
    primes3 = intalg.first_primes_moremem(100)
    primes3_exp = primes2[:]
    primes3_exp[0] = 6
    self.assertEquals(primes3_exp, primes3)  # Because of the fake _prime_cache.

  def testFirstPrimes(self):
    self.assertEquals([], intalg.first_primes(0))
    self.assertEquals([2], intalg.first_primes(1))
    self.assertEquals([2, 3, 5, 7, 11, 13, 17, 19],
                      intalg.first_primes(8))
    self.assertEquals([2, 3, 5, 7, 11, 13, 17, 19, 23],
                      intalg.first_primes(9))
    primes = intalg.first_primes(100)
    self.assertEquals(100, len(primes))
    self.assertEquals([1], intalg._prime_cache_limit_ary[:])
    self.assertEquals(len(primes) - 1, intalg.prime_index(primes[-1]))
    self.assertEquals([1024], intalg._prime_cache_limit_ary[:])
    primes2 = intalg.first_primes(100)
    self.assertEquals(primes, primes2)

    intalg._prime_cache[0] = 6
    primes3 = intalg.first_primes(100)
    primes3_exp = primes2[:]
    primes3_exp[0] = 6
    self.assertEquals(primes3_exp, primes3)  # Because of the fake _prime_cache.

  def testYieldSlowFactorize(self):
    self.assertEquals(list(intalg.yield_slow_factorize(1)), [])
    self.assertEquals(list(intalg.yield_slow_factorize(36)), [2, 2, 3, 3])
    self.assertEquals(list(intalg.yield_slow_factorize(180)), [2, 2, 3, 3, 5])
    self.assertEquals(list(intalg.yield_slow_factorize(3 ** 1000 * 29 ** 57)), [3] * 1000 + [29] * 57)
    self.assertEquals(list(intalg.yield_slow_factorize(2 * 3 * 5 * 7 * 7 * 11 * 13 * 17 * 19)), [2, 3, 5, 7, 7, 11, 13, 17, 19])

  def testFactorizeMedium(self):
    n = intalg.next_prime(intalg._SMALL_PRIME_LIMIT) ** 2 - 10
    limit = n + 100
    while n < limit:  # xrange can't take a long (n).
      a = list(intalg.yield_slow_factorize(n))
      b = intalg.finder_slow_factorize(n)
      c = intalg.factorize(n)
      d = intalg.factorize(n, divisor_finder=intalg.brent)
      e = intalg.factorize(n, divisor_finder=intalg.pollard)
      assert a == b == c == d == e, (n, a, b, c, d, e)
      n += 1

  def testFactorizeSmall(self):
    for n in xrange(1, 100):
      a = list(intalg.yield_slow_factorize(n))
      b = intalg.finder_slow_factorize(n)
      c = intalg.factorize(n, divisor_finder=intalg.brent)
      d = intalg.factorize(n, divisor_finder=intalg.pollard)
      assert a == b == c == d, (n, a, b, c, d)

  def testBrentPrime(self):
    random_obj = intalg.MiniIntRandom(42)
    for n in intalg.primes_upto(100):
      b = intalg.brent(n, random_obj)
      self.assertEquals(b, n)

  def testPollardPrime(self):
    random_obj = intalg.MiniIntRandom(42)
    for n in intalg.primes_upto(100):
      b = intalg.pollard(n, random_obj)
      self.assertEquals(b, n)

  def testBrentComposite(self):
    random_obj = intalg.MiniIntRandom(42)
    for n in intalg.yield_composites():
      if n > 100:
        break
      b = intalg.brent(n, random_obj)
      assert b > 1
      assert b <= n
      self.assertEquals(0, n % b, (b, n))

  def testPollardComposite(self):
    random_obj = intalg.MiniIntRandom(42)
    for n in intalg.yield_composites():
      if n > 100:
        break
      b = intalg.pollard(n, random_obj)
      assert b > 1
      assert b <= n
      self.assertEquals(0, n % b, (b, n))

  def testTotient(self):
    expected = [0, 1, 1, 2, 2, 4, 2, 6, 4, 6, 4, 10, 4, 12, 6, 8, 8, 16, 6,
                18, 8, 12, 10, 22]
    self.assertEquals([0], intalg.totients_upto(0))
    self.assertEquals([0], intalg.totients_upto(0, force_recursive=1))
    self.assertEquals([0, 1], intalg.totients_upto(1, force_recursive=1))
    self.assertEquals([0, 1], intalg._totients_upto_iter(1))
    self.assertEquals([0, 1, 1], intalg.totients_upto(2, force_recursive=1))
    self.assertEquals([0, 1, 1], intalg._totients_upto_iter(2))
    self.assertEquals(expected, intalg.totients_upto(23))
    self.assertEquals(expected, intalg.totients_upto(23, force_recursive=1))
    self.assertEquals(expected, intalg._totients_upto_iter(23))
    self.assertEquals(expected, map(intalg.totient, xrange(24)))
    limit = 500
    expected2 = map(intalg.totient, xrange(limit + 1))
    self.assertEquals(expected2, intalg.totients_upto(limit))
    self.assertEquals(expected2, intalg.totients_upto(limit, force_recursive=1))
    self.assertEquals(expected2, intalg._totients_upto_iter(limit))

  def testDivisorCountsUpto(self):
    self.assertEquals(
        [0, 1, 2, 2, 3, 2, 4, 2, 4, 3, 4, 2, 6, 2, 4, 4, 5, 2, 6, 2, 6, 4, 4,
         2, 8, 3, 4, 4, 6, 2, 8, 2, 6, 4, 4, 4, 9, 2, 4, 4, 8, 2, 8, 2, 6, 6,
         4, 2, 10, 3, 6, 4, 6, 2, 8, 4, 8, 4, 4, 2, 12, 2, 4, 6, 7, 4, 8, 2,
         6, 4, 8, 2, 12, 2, 4, 6, 6, 4, 8, 2, 10, 5, 4, 2, 12, 4, 4, 4, 8, 2,
         12, 4, 6, 4, 4, 4, 12, 2, 6, 6, 9], intalg.divisor_counts_upto(100))

  def testDivisors(self):
    self.assertEquals([1], intalg.divisors(1))
    self.assertEquals([1, 2, 3, 4, 6, 12], intalg.divisors(12))

  def testInvTotient(self):
    self.assertEquals([1], intalg.inv_totient(1))
    self.assertEquals([3, 4, 6], intalg.inv_totient(2))
    self.assertEquals([], intalg.inv_totient(3))
    self.assertEquals([5, 8, 10, 12], intalg.inv_totient(4))
    self.assertEquals(
        [73, 91, 95, 111, 117, 135, 146, 148, 152, 182, 190,
         216, 222, 228, 234, 252, 270], intalg.inv_totient(72))

  def testPrimeIndex(self):
    self.assertEquals(2, intalg.prime_index(5))
    self.assertEquals([256], intalg._prime_cache_limit_ary[:])
    self.assertEquals(2, intalg.prime_index(5))
    self.assertEquals(None, intalg.prime_index(4))
    self.assertEquals(1, intalg.prime_index(3))
    self.assertEquals(0, intalg.prime_index(2))
    self.assertEquals(None, intalg.prime_index(1))
    self.assertEquals(None, intalg.prime_index(0))
    self.assertEquals(None, intalg.prime_index(255))
    self.assertEquals(len(intalg._prime_cache) - 1, intalg.prime_index(251))
    self.assertEquals(len(intalg._prime_cache) - 1, intalg.prime_index(251))
    self.assertEquals([256], intalg._prime_cache_limit_ary[:])
    # Grows the cache.
    self.assertEquals(intalg.prime_index(251) + 1, intalg.prime_index(257))
    self.assertEquals([512], intalg._prime_cache_limit_ary[:])
    self.assertEquals(intalg.prime_index(251) + 1, intalg.prime_index(257, 600))
    self.assertEquals([512], intalg._prime_cache_limit_ary[:])
    self.assertEquals(None, intalg.prime_index(600, 600))
    self.assertEquals([600], intalg._prime_cache_limit_ary[:])
    self.assertEquals(109, intalg.prime_index(601, limit=155))
    self.assertEquals([620], intalg._prime_cache_limit_ary[:])

  def testPrimeCount(self):
    self.assertEquals(3, intalg.prime_count(6))
    self.assertEquals(3, intalg.prime_count(5))
    self.assertEquals([256], intalg._prime_cache_limit_ary[:])
    self.assertEquals(3, intalg.prime_count(5))
    self.assertEquals(2, intalg.prime_count(4))
    self.assertEquals(2, intalg.prime_count(3))
    self.assertEquals(1, intalg.prime_count(2))
    self.assertEquals(0, intalg.prime_count(1))
    self.assertEquals(0, intalg.prime_count(0))
    self.assertEquals(54, intalg.prime_count(255))
    self.assertEquals(len(intalg._prime_cache), intalg.prime_count(251))
    self.assertEquals(len(intalg._prime_cache), intalg.prime_count(251))
    self.assertEquals([256], intalg._prime_cache_limit_ary[:])
    # Grows the cache.
    self.assertEquals(intalg.prime_count(251) + 1, intalg.prime_count(257))
    self.assertEquals([512], intalg._prime_cache_limit_ary[:])
    self.assertEquals(intalg.prime_count(251) + 1, intalg.prime_count(257, 600))
    self.assertEquals([512], intalg._prime_cache_limit_ary[:])
    self.assertEquals(109, intalg.prime_count(600, 600))
    self.assertEquals([600], intalg._prime_cache_limit_ary[:])
    self.assertEquals(110, intalg.prime_count(601, limit=155))
    self.assertEquals([620], intalg._prime_cache_limit_ary[:])
    self.assertEquals(110, intalg.prime_count(602))

  def testPrimeCountMore(self):
    prime_counts = map(intalg.prime_count, xrange(3000))
    prime_counts2 = map(intalg.prime_count_more, xrange(3000))
    self.assertEquals([True], list(set((
        a <= b for a, b in zip(prime_counts, prime_counts2)))))
    self.assertFalse(prime_counts == prime_counts2)  # Not accurate.
    self.assertEquals(prime_counts[:127], prime_counts2[:127])
    # Default accuracy.
    prime_counts3 = [intalg.prime_count_more(n, 100000) for n in xrange(3000)]
    self.assertEquals([True], list(set((
        a <= b for a, b in zip(prime_counts, prime_counts3)))))
    self.assertEquals(prime_counts2, prime_counts3)
    self.assertEquals([True], list(set((
        a * 11 > b * 10  # Accurate enough.
        for a, b in zip(prime_counts, prime_counts3)[2:]))))

    # Better accuracy.
    prime_counts4 = [intalg.prime_count_more(n, 50000) for n in xrange(3000)]
    self.assertEquals([True], list(set((
        a <= b for a, b in zip(prime_counts, prime_counts4)))))
    self.assertFalse(prime_counts == prime_counts4)  # Not accurate.
    self.assertEquals(prime_counts[:547], prime_counts4[:547])
    self.assertEquals([True], list(set((
        a * 105 > b * 100  # Accurate enough.
        for a, b in zip(prime_counts, prime_counts4)[2:]))))

  def testIsPrime(self):
    limit = 100
    primes = intalg.primes_upto(limit)
    self.assertEquals(97, primes[-1])
    primes2 = [n for n in xrange(limit + 1) if intalg.is_prime(n)]
    self.assertEquals(primes, primes2)
    self.assertEquals('True', repr(intalg.is_prime(97)))
    self.assertEquals('False', repr(intalg.is_prime(98)))
    self.assertEquals('False', repr(intalg.is_prime(99)))
    self.assertEquals('False', repr(intalg.is_prime(100)))

    self.assertEquals([1], intalg._prime_cache_limit_ary[:])
    intalg.prime_index(257)
    self.assertEquals([512], intalg._prime_cache_limit_ary[:])
    primes3 = [n for n in xrange(99) if intalg.is_prime(n)]
    self.assertEquals(primes, primes3)

    limit = 1009
    primes = intalg.primes_upto(limit)
    self.assertEquals(1009, primes[-1])
    primes2 = [n for n in xrange(limit + 1) if intalg.is_prime(n)]
    self.assertEquals(primes, primes2)

    del intalg._prime_cache[:]
    primes4 = [n for n in xrange(99) if intalg.is_prime(n)]
    self.assertEquals([2], primes4)  # Because of the fake empty _prime_cache.


if __name__ == '__main__':
  unittest.main()
