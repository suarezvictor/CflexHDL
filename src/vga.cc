// Basic VGA signal generation (aimed to simulation)
// (C) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>

MODULE vga_timing(uint1 &vga_hs, uint1 &vga_vs, uint1 &vga_de, uint10& vga_out_x, uint10 &vga_out_y)
{

#ifdef CFLEX_NO_COROUTINES
	static uint10 vga_x = 0;
	static uint10 vga_y = 0;
#else
	uint10 vga_x = 0;
	uint10 vga_y = 0;
	while(always())
#endif
	{
		vga_hs = (vga_x == FRAME_WIDTH-1);
	  	vga_vs = (vga_y == FRAME_HEIGHT-1);

	  	vga_de = !vga_hs && !vga_hs;

	  	if(vga_x == FRAME_WIDTH)
	  	{
			vga_x = 0;
			if(vga_y == FRAME_HEIGHT)
			  vga_y = 0;
			else
			  vga_y = vga_y + 1;
		  }
		  else
			vga_x = vga_x + 1;

		 vga_out_x = vga_x;
		 vga_out_y = vga_y;
	}
}

#if 0
//TODO: test generation
void vga_demo(
  uint10 &pix_x,
  uint10 &pix_y,
  uint1 &hsync,
  uint1 &pix_vblank,
  uint1 &pix_active,
  uint_color_depth &pix_r,
  uint_color_depth &pix_g,
  uint_color_depth &pix_b
)
{
    vga_timing(pix_x, pix_y, hsync, pix_vblank, pix_active);
    frame_display(pix_x, pix_y, pix_vblank, pix_active, pix_r, pix_g, pix_b);
}
#endif

