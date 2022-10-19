from migen import  *
from litex.soc.cores.video import ColorBarsPattern

class ColorbarsModule(Module):
  def __init__(self):
     self.pix_x = Signal(12)
     self.pix_y = Signal(12)
     self.pix_active = Signal()
     self.pix_vblank = Signal()
     self.pix_r = Signal(8)
     self.pix_g = Signal(8)
     self.pix_b = Signal(8)

     # # #

     colorbars = ColorBarsPattern()
     self.submodules.colorbars = colorbars
     self.comb += [
		colorbars.source.ready.eq(1),
     	self.pix_r.eq(colorbars.source.r),
     	self.pix_g.eq(colorbars.source.g),
		self.pix_b.eq(colorbars.source.b),
		colorbars.vtg_sink.ready.eq(1),
		colorbars.vtg_sink.valid.eq(1),
		colorbars.vtg_sink.first.eq(1),
		colorbars.vtg_sink.de.eq(self.pix_active),
		colorbars.vtg_sink.hres.eq(640),
		colorbars.vtg_sink.vres.eq(480),
     ]

m = ColorbarsModule()

import os
if os.environ["MIGEN2C_TARGET"] == "verilog":
  from migen.fhdl import verilog as target
else:
  import migen2cflexhdl as target
                  
print(target.convert(m, name="frame_display", ios={
  m.pix_x,
  m.pix_y,
  m.pix_active,
  m.pix_vblank,
  m.pix_r,
  m.pix_g,
  m.pix_b,
  }))

