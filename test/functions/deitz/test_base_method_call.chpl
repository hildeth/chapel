class C {
  var x: int;
  def foo() {
    writeln("C: ", this);
  }
}

class D: C {
  var y: int;
  def foo() {
    writeln("D: ", this);
  }
}

var d: D = D();
(d:C).foo();
