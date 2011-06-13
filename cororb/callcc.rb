#! /usr/bin/ruby -w
#
# callcc.rb: a Fiber implementation for Ruby 1.8, based on callcc
# by pts@fazekas.hu at Tue Nov 30 21:36:00 CET 2010
#
# SUXX: It has terrible memory leaks in Ruby 1.8.
#
# This script works in Ruby 1.9 (using the built-in Fiber class) and Ruby 1.8
# (using the emulation provided here).
#
# TODO(pts): Run the Ruby unit tests.
#
# Root reason for hthe leak, on http://www.ruby-forum.com/topic/170608
#
#   The central problem is that gcc (and other compilers) tend to create
#   sparse stack frames such that, when a new frame is pushed onto the
#   stack, it does not completely overwrite the one that had been previously
#   stored there. The new frame gets activated with old VALUE pointers
#   preserved inside its holes. These become "live" again as far as any
#   conservative garbage collector is concerned. And, viola, a leak is born!
#
$-w = true

class FiberError < StandardError
end

# TODO(pts): Measure speed.
# TODO(pts): Evaluate memory use and GC memory leaks.
class Fiber;
  fail "Fiber replacement loaded multiple times" if defined?(@@current)
  fail "Fiber emulation loaded over another Fiber" if method_defined?(:alive?)
  @@current = @@root = nil
  def initialize(&block)
    if nil == block;
      raise ArgumentError, "tried to create Proc object without a block" if
          nil != @@current
      @cc = nil
      @is_alive = true
      @resumed_by = nil
      @thread = Thread.current
    else
      @is_alive = true  # Can be resumed/transferred later.
      @resumed_by = false
      @thread = Thread.current
      callcc { |cc1|
        x = callcc { |cc2|
          @cc = cc2
          cc1.call
        }
        begin
          x = block.call(*x)
          exc = nil
        rescue Exception
          exc = $!
        end
        @is_alive = false
        block = nil  # Save memory.
        # TODO(pts): Discard the current stack frame to reclaim memory.
        # TODO(pts): Avoid creating local variables.
        # Ruby 1.8 doesn't have tail recursion optimization, and 1.9.2 doesn't
        # have it either, see also http://redmine.ruby-lang.org/issues/show/1256
        if @resumed_by;
          resumed_by = @resumed_by
          @resumed_by = nil  # Break the reference to save memory.
          # TODO(pts): Add a begin/rescue to the transfer call below.
          if !resumed_by.alive?
            # TODO(pts): Do we have to use the parant of resumed_by instead?
            resumed_by = @@root
            exc = FiberError.new('dead fiber called')
          end
        else
          resumed_by = @@root
        end
        @@goodbye_args.push(resumed_by, exc, x)
        resumed_by = exc = x = nil  # Save memory.
        @@goodbye.call()
        fail "unreachable"  # TODO(pts): Optimize this away.
      }
    end
  end
  def inspect()
    "#<Fiber:0x%x>" % (object_id << 1)
  end
  def to_s()
    "#<Fiber:0x%x>" % (object_id << 1)
  end
  def alive?()
    @is_alive
  end
  def transfer(*args)
    raise FiberError, "dead fiber called" if !@is_alive
    raise FiberError, "fiber called across threads" if @thread != Thread.current

    if self == @@current;
      fail if args.size > 1  # ruby 1.9.2 does this.
      return args[0]  # nil if args.empty?
    end
    fail if nil != @@current.cc
    fail if nil == @cc
    x = callcc { |cc|
      @@current.cc = cc
      cc = @cc
      @cc = nil
      @@current = self
      cc.call(args)  # TODO(pts): Forget about cc (for garbage collection).
    }
    @@goodbye_args.clear  # Save memory.
    raise x if x.kind_of? Exception
    x.size > 1 ? x : x[0]
  end
  def resume()  #args)
    return [0]
    # TODO(pts): Fix the error order.
    raise FiberError, "dead fiber called" if !@is_alive
    raise FiberError, "double resume" if @resumed_by
    raise FiberError, "fiber called across threads" if @thread != Thread.current
    @resumed_by = @@current
    transfer(*args)
  end
  def __yield(*args)  # TODO(pts): Don't expose this publicly.
    raise FiberError, "can't yield from root fiber" if
        nil == @resumed_by and self == @@root
    resumed_by = @resumed_by || @@root
    @resumed_by = nil
    # TODO(pts): Clean up our stack to save memory.
    resumed_by.transfer(*args)
  end
  class <<self;
    def current()
      @@current
    end
    def yield(*args)
      @@current.__yield(*args)
    end
  end
  def __transfer_exc(exc)  # TODO(pts): Make this code non-protected.
    # TODO(pts): Avoid code duplication.
    raise FiberError, "dead fiber called" if !@is_alive
    raise FiberError, "fiber called across threads" if @thread != Thread.current
       
    if self == @@current;
      fail if args.size > 1  # ruby 1.9.2 does this.
      return args[0]  # nil if args.empty?
    end
    fail if nil != @@current.cc
    fail if nil == @cc
    x = callcc { |cc|
      @@current.cc = cc
      cc = @cc
      @cc = nil
      @@current = self
      cc.call(exc)  # TODO(pts): Forget about cc (for garbage collection).
    }
    @@goodbye_args.clear  # Save memory.
    raise x if x.kind_of? Exception
    x.size > 1 ? x : x[0]
  end
  protected
  @@root = @@current = self.new
  @@goodbye_args = []
  callcc { |cc1|
    callcc { |cc2|
      @@goodbye = cc2
      cc1.call()
    }
    # This is the routine called by @@goodbye.call.
    loop {
      # fiber, exc, x = @@goodbye_args
      if @@goodbye_args[1];
        @@goodbye_args[0].__transfer_exc(@@goodbye_args[1])
      else
        @@goodbye_args[0].transfer(@@goodbye_args[2])
      end
    }    
  }
  attr_accessor :cc
end if RUBY_VERSION < "1.9"

# ---


if false;  # !!
  p RUBY_VERSION
  root = Fiber.current
  fail if "can't yield from root fiber" != begin
    Fiber.yield
  rescue FiberError
    $!.to_s
  end
  fail if true != root.alive?
  fail if nil != root.transfer()
  fail if 3 != root.transfer(3)
  fail if 4 != root.resume(4)
  fail if "double resume" != begin
    root.resume() && nil
  rescue FiberError
    $!.to_s
  end
  # Yes, we can yield from the root fiber, if we have previously resumed it.
  # This works in Ruby 1.9.2.
  fail if 5 != Fiber.yield(5)
  fail if "can't yield from root fiber" != begin
    Fiber.yield 6
  rescue FiberError
    $!.to_s
  end

  fi = Fiber.new { |*x| root.transfer({:x=>x}).inspect }
  fail if true != fi.alive?
  fail if {:x=>[]} != fi.transfer()
  fail if true != fi.alive?
  fail if 56.inspect != fi.transfer(56)
  fail if false != fi.alive?
  fail if "dead fiber called" != begin
    fi.transfer()
  rescue FiberError
    $!.to_s
  end
  fail if "dead fiber called" != begin
    fi.resume()
  rescue FiberError
    $!.to_s
  end
  fail if "dead fiber called" != begin  # Not a "double resume".
    fi.resume()
  rescue FiberError
    $!.to_s
  end
  fi = Fiber.new { |*x| root.transfer({:x=>x}).inspect }
  fail if {:x=>[nil]} != fi.transfer(nil)
  fail if [57, 58].inspect != fi.transfer(57, 58)
  fi = Fiber.new { |*x| root.transfer({:x=>x}) + [3, 4] }
  fail if {:x=>[[:a]]} != fi.transfer([:a])
  fail if [1, 2, 3, 4] != fi.transfer([1, 2])
  fi = Fiber.new { |*x| root.transfer({:x=>x}) }
  fail if {:x=>[:a]} != fi.transfer(:a)
  fi = Fiber.new { |*x| root.transfer({:x=>x}) }
  fail if {:x=>[:a, :b]} != fi.transfer(:a, :b)
  fi = Fiber.new { |*x| root.transfer({:x=>x}) }
  fail if {:x=>[[:a, :b]]} != fi.transfer([:a, :b])
  fi = Fiber.new { |*x| Fiber.yield({:x=>x}) }
  fail if {:x=>[[:a, :b, :c]]} != fi.resume([:a, :b, :c])
  fi = Fiber.new { |*x| Fiber.yield(x) }
  fail if [[:a, :b, :c]] != fi.resume([:a, :b, :c])
  fi = Fiber.new { |*x| Fiber.yield(x) }
  fail if [42] != fi.resume(42)
  fi = Fiber.new { |*x| Fiber.yield(x[0] * 2) }
  fail if 84 != fi.resume(42)
  fi = Fiber.new { |*x| Fiber.yield(x[0] + 1, x[0] + 2) }
  fail if [43, 44] != fi.resume(42)

  # This is to demonstrate that if a non-resume()d fiber yield()s, then it
  # will give control to the root fiber. If it gave control to any of its
  # caller fibers instead, the return value would be be multiplied by 10
  # and/or -1.
  fail if 42 != Fiber.new {
    Fiber.new { Fiber.new { Fiber.yield 42 }.transfer() * 10 }.resume() * -1
  }.transfer()

  # This is to demonstrate that if a resume()d fiber yields(), then it will
  # give control to the fiber which resumed() it.
  fail if -420 != Fiber.new {
    Fiber.new { Fiber.new { Fiber.yield 42 }.resume() * 10 }.resume() * -1
  }.transfer()

  # Combination of the rules above.
  fail if 420 != Fiber.new {
    Fiber.new { Fiber.new { Fiber.yield 42 }.resume() * 10 }.transfer() * -1
  }.resume()

  yield_for_me = lambda { |x| Fiber.yield x }
  squares = Fiber.new { |n|
    Fiber.yield :a
    i = 1
    while i * i <= n;
      Fiber.yield i * i
      i += 1
    end
  }
  double = Fiber.new { |fi1|
    Fiber.yield :b
    loop {
      x = fi1.resume()
      break if nil == x
      yield_for_me.call x * 2
    }
  }
  fail if :a != squares.resume(25)
  fail if :b != double.resume(squares)
  items = []
  loop {
    x = double.resume()
    break if nil == x
    items << x
  }
  fail if [2, 8, 18, 32, 50] != items

  # This is to demonstrate that if a transfer()-ed fiber returns, it will
  # return control to the root fiber. If control was returned to f2 and/or f3,
  # then the returned value would be multiplied by 10 and/or -1.
  f2 = Fiber.new { |fi1| fi1.transfer() * 10 }
  f3 = Fiber.new { f2.transfer(Fiber.new { 42 }) * -1 }
  fail if 42 != f3.transfer()

  # This is to demonstrate that when a resume()d fiber returns, it will
  # return control to the fiber which resumed it.
  f2 = Fiber.new { |fi1| fi1.resume() * 10 }
  f3 = Fiber.new { f2.resume(Fiber.new { 42 }) * -1 }
  fail if -420 != f3.resume()

  # Combination of the rules above.
  f2 = Fiber.new { |fi1| fi1.transfer() * 10 }
  f3 = Fiber.new { f2.resume(Fiber.new { 42 }) * -1 }
  fail if 42 != f3.resume()

  # Combination of the rules above.
  f2 = Fiber.new { |fi1| fi1.resume() * 10 }
  f3 = Fiber.new { f2.transfer(Fiber.new { 42 }) * -1 }
  fail if 420 != f3.resume()

  # This is to demonstrate that exceptions are propagated.
  f1 = Fiber.new { fail "lucky" }
  f2 = Fiber.new {
    begin
      f1.resume
    rescue Exception => e
      "un" + e.to_s
    end
  }
  fail if f2.resume != "unlucky"

  #p root.cc  # Can't call protected method.
  foo = Fiber.new { |*x|
    fail if x != [41, 410]
    fail if [26, 260] != root.transfer(15)
    fail if 27 != root.transfer(16)
    root.transfer(17)
    fail
  }
  fail if 15 != foo.transfer(41, 410)
  fail if 16 != foo.transfer(26, 260)
  fail if 17 != foo.transfer(27)
end

if false;
  def f(n, root)
    if n > 0;
      return 1 + f(n - 1, root)
    end
    while true;
      root.transfer 5
    end
  end

  def g(n, fi)
    if n > 0;
      return 1 + g(n - 1, fi)
    end
    100000.times {
      fail if 5 != fi.transfer
    }
    0
  end
#depth = 230  # SystemStackError above that.
#root = Fiber.current
#g(depth, Fiber.new { f(depth, root) })
end

p "All OK."

class ZFiber
  def f()
    #callcc { |cc| @cc = cc; return }  # wastes
    callcc { |cc1|  # wastes
      callcc { |cc2|
        @cc = cc2
        cc1.call
      }
    }
    self
  end
end

def foo()
  callcc { |cc| @cc = cc; return }
end

if true;
  GC.enable
  GC.start
  # SUXX: The Ruby 1.8 garbage collector doesn't get triggered automatically.
  # Wouldn't work even with 3GB of RAM wouth calling the GC manually.
  Process.setrlimit(Process::RLIMIT_AS, 70 << 20)  # 30MB
  # On narancs at r171, 510 fits, 520 doesn't fit to 30MB -- so we do 5200.
  (1..520000).each { |i|
    #Fiber.new {}  # This itself doesn't waste memory.
    #Fiber.new {}.alive?  # This itself wastes memory!
    #ZFiber.new.object_id  # This itself wastes memory with return.
    ZFiber.new.f  # This doesn't waste memory.
    ZFiber.new.f.object_id  # This itself wastes memory with return.
    #foo()  # This doesn't waste memory.
    #fail if Fiber.new { [i * 0] }.resume != [0]
    #GC.start if (i & 127) == 0  # This fixes the problem.
  }
end

# depth=230 iter=100000 ./callcc.rb  1.80s user 0.06s system 100% cpu 1.825 total
# depth=2   iter=100000 ./callcc.rb  0.74s user 0.00s system 100% cpu 0.737 total
