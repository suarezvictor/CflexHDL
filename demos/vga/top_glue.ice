// SL 2020-04-23
// VSR 2022
// Main file for all vga demo projects
// -------------------------
// MIT license, see LICENSE_MIT in Silice repo root
// https://github.com/sylefeb/Silice


$$color_depth = 8

algorithm vga_demo(
  output!  uint10             pix_x,
  output!  uint10             pix_y,
  output! uint$color_depth$ video_r,
  output! uint$color_depth$ video_g,
  output! uint$color_depth$ video_b,
  output!  uint1             video_hs,
  output!  uint1             video_vs,
  output!  uint1             video_de,
)
{
  vga_timing vga (
      vga_hs :> video_hs,
	  vga_vs :> video_vs,
	  vga_de :> video_de,
      vga_out_x :> pix_x,
      vga_out_y :> pix_y,
  );

  frame_display display (
	  pix_x      <: pix_x,
	  pix_y      <: pix_y,
	  pix_active <: video_de,
	  pix_vblank <: video_vs,
	  pix_r      :> video_r,
	  pix_g      :> video_g,
	  pix_b      :> video_b,
  );
}

// -------------------------
