#!/bin/bash

set -euo pipefail

./kppc < lib/core.kl
mv output.s lib/core.s
./kppc < lib/builtin.kl
mv output.s lib/builtin.s
cd lib
${CMAKE_CXX_COMPILER} -c core.s builtin.s
rm -r *.s
