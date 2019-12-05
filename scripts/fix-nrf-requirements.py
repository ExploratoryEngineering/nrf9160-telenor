#!/usr/bin/env python3

# Too strict version range requirements in nrfutil 5.x cause dependency
# conflicts. Nordic have relaxed the dependencies in master, but it's
# only available in the 6.0.0 alpha. So force the alpha version until
# Nordic release 6.0.0

f = open("deps/nrf/scripts/requirements.txt", "r+")
fixed = f.read().replace("nrfutil\n", "nrfutil>=6.0.0a1\n")
f.seek(0)
f.write(fixed)
f.close()
