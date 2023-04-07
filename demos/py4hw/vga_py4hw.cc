// This CflexHDL file was automatically created by py4hw RTL generator
MODULE VGATestPattern (
	const bool& reset,
	bool& hactive,
	bool& vactive,
	uint8& r,
	uint8& g,
	uint8& b) {
// Code generated from clock method
// variable declaration 
int i0=0;
int i1=0;
int i2=0;
int x=0,y=0;
int vr,vg,vb,divx;
// process 
while(always())
{
x = x+1; // sync
if (x>=840)
{
x = 0; // sync
y = y+1; // sync
if (y>=520)
{
y = 0; // sync

}

}
divx = x/80; // sync
vr = i0&1 ? 255 : 0; // sync
vg = i1&1 ? 255 : 0; // sync
vb = i2&1 ? 255 : 0; // sync
r = vr; // sync
g = vg; // sync
b = vb; // sync
vactive = y<480 ? 1 : 0; // sync
hactive = x<640 ? 1 : 0; // sync
i0 = divx>>0; // sync
i1 = divx>>1; // sync
i2 = divx>>2; // sync
}
}

