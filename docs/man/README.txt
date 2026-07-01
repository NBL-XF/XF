XF / LambdaScript / ByteLang mandoc bundle

Included pages:
  man1/xf.1
  man1/lambdascript.1
  man1/bytelang.1
  man7/xf-lang.7
  man7/xf-core.7
  man7/xf-vm.7
  man7/xf-gc.7
  man7/xf-mt.7
  man7/lambdascript.7
  man7/bytelang.7

Not included in this first bundle:
  libxf(3), libls(3), libbl(3)

Reason:
  the needed public headers / API sources were not all uploaded in the final set.

Install locally:
  mkdir -p ~/.local/share/man/man1 ~/.local/share/man/man7
  cp man1/*.1 ~/.local/share/man/man1/
  cp man7/*.7 ~/.local/share/man/man7/
  mandb 2>/dev/null || true

Preview:
  man ./man1/xf.1
  man ./man1/lambdascript.1
  man ./man1/bytelang.1
