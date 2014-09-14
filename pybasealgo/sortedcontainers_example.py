#! /usr/bin/python
# by pts@fazekas.hu at Sun Sep 14 18:54:18 CEST 2014

import sorteddict
import sortedlist
import sortedlistwithkey
import sortedset

sl = sortedlist.SortedList(xrange(1000, 10, -5))
l = list(sl)
print sl
assert sl[0] == 15
assert sl == l
assert not (sl < l)


slk = sortedlistwithkey.SortedListWithKey(xrange(1000, 10, -5), lambda x: -x)
lk = list(slk)
print slk
assert slk[0] == 1000
assert slk == lk
assert not (slk < lk)


ss = sortedset.SortedSet(xrange(1000, 10, -5))
s = set(ss)
print ss
assert ss[0] == 15
assert ss == s
assert not (ss < s)

sd = sorteddict.SortedDict((x, -x) for x in xrange(1000, 10, -5))
d = dict(sd)
print sd
assert iter(sd.iteritems()).next() == (15, -15)
assert sd == d
assert sd < d
