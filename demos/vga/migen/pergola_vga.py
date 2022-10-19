#TestImageGenerator RotozoomImageGenerator StaticTestImageGenerator 
from pergola_vga_testimage import RotozoomImageGenerator as videogen

from migen import *

m = videogen(
  Signal(12, name="pix_x"),
  Signal(12, name="pix_y"),
  Signal(name="pix_active"),
  Signal(name="pix_vblank"),
  Signal(8, name="pix_r"),
  Signal(8, name="pix_g"),
  Signal(8, name="pix_b"),
  speed=2,
  )

import os
if os.environ["MIGEN2C_TARGET"] == "verilog":
  from migen.fhdl import verilog as target
else:
  import migen2cflexhdl as target
                  
print(target.convert(m, name="frame_display", ios={
  m.h_ctr,
  m.v_ctr,
  m.pix_active,
  m.vsync,
  m.r,
  m.g,
  m.b,
  }))

