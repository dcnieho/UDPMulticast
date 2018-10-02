# load in correct pyd, depending on whether this is an 32 or 64 bit environment
import sys
if sys.maxsize > 2**32:
    from bit64 import *
else:
    from bit32 import *


# TODO: add pure python functions as per
# http://www.boost.org/doc/libs/1_62_0/libs/python/doc/html/tutorial/tutorial/techniques.html
# e.g., for debug, getting single commands, etc