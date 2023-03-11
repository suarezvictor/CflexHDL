#ifndef __CFLEXVCD_H__
#define __CFLEXVCD_H__

#include <vector>

struct vcd_signal
{
  const char *name;
  int id;
  uint8_t *data;
  uint8_t width;
};

static const char *bit_rep[16] = {
    [ 0] = "0000", [ 1] = "0001", [ 2] = "0010", [ 3] = "0011",
    [ 4] = "0100", [ 5] = "0101", [ 6] = "0110", [ 7] = "0111",
    [ 8] = "1000", [ 9] = "1001", [10] = "1010", [11] = "1011",
    [12] = "1100", [13] = "1101", [14] = "1110", [15] = "1111",
};

void print_byte(uint8_t byte)
{
}

class CflexVCD
{
  typedef std::vector<vcd_signal> signals_t;
  signals_t signals;
  FILE *f;
    
public:
  typedef uint16_t clock_t;
  
  CflexVCD(const char *fname)
  {
    f = fopen(fname, "wb");
  }
  
  template<class T>
  void add_signal(const char *name, const T& signal, int width=1)
  {
    uint8_t id = '$' + signals.size();
    signals.push_back(vcd_signal{name, id, (uint8_t*)&signal, width});
  }
  
  void start()
  {
    fprintf(f, "$date Sept 10 2008 12:00:05 $end\n");
    fprintf(f, "$version CflexHDL Simulator$end\n");
    fprintf(f, "$timescale 1 ns $end\n");
    fprintf(f, "$scope module top $end\n");
    for(auto e: signals)
    {
      fprintf(f, "$var wire %d %c %s $end\n", e.width, e.id, e.name);
    }
    fprintf(f, "$upscope $end\n");
    fprintf(f, "$enddefinitions $end\n");
  }
  
  void clock(clock_t t)
  {
    fprintf(f, "\n#%hu\n", t);
    for(auto e: signals)
    {
      if(e.width == 1)
        fprintf(f, "%d%c\n", e.data[0], e.id);
      else
      {
        fprintf(f, "b");
        int w = (e.width-1)/8;
        uint8_t *p = e.data + w;
        for(;;)
        {
          fprintf(f, "%s%s", bit_rep[*p >> 4], bit_rep[*p & 0x0F]);
          if(w == 0)
            break;
          --p;
          --w;
        }
        fprintf(f, " %c\n", e.id);
      }
    }
  }
};

#endif //__CFLEXVCD_H__
