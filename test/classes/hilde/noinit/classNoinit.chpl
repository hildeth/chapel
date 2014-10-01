// classNoinit.chpl
//
// The spec says:
// "If the \sntx{no-initialization-part} is present, the variable
// declaration does not initialize the variable to any value, as
// described in~\rsec{Noinit_Capability}. The result of any read of an
// uninitialized variable is undefined until that variable is written."
//
// Does that also apply to class variables?
//

class C { var i:int; var r:real; }

proc foo()
{
  var c:C = noinit;
  if c == nil then
    writeln("Liar.");
  else
    // Expect gibberish or a segfault.
    writeln(c);
}

foo();
