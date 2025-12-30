//-----------------------------------------------------------------------------
//(C) 2020 ultraembedded https://github.com/ultraembedded/core_jpeg
//(C) 2024-2025 Victor Suarez Rovere <suarezvictor@gmail.com>
//License: Apache-2.0
//-----------------------------------------------------------------------------

#include "jpeg_dqt.h"
#include "jpeg_dht.h"
#include "jpeg_idct.h"
#include "jpeg_bit_buffer.h"
#include "jpeg_mcu_block.h"

struct jpeg_state
{
	jpeg_dqt        m_dqt;
	jpeg_dht        m_dht;
	jpeg_idct       m_idct;
	jpeg_bit_buffer m_bit_buffer;
	jpeg_mcu_block  m_mcu_dec;


	jpeg_state() :
	 m_bit_buffer(bit_buffer, sizeof(bit_buffer)),
	 m_mcu_dec(&m_bit_buffer, &m_dht)
	{
	}
private:
	uint8_t bit_buffer[1<<20];
};

static uint16_t m_width;
static uint16_t m_height;

typedef enum eJpgMode
{
    JPEG_MONOCHROME,
    JPEG_YCBCR_444,
    JPEG_YCBCR_420,
    JPEG_UNSUPPORTED
} t_jpeg_mode;

static t_jpeg_mode m_mode;

static uint8_t m_dqt_table[3];

#define get_byte(_buf, _idx)  _buf[_idx++]
#define get_word(_buf, _idx)  ((_buf[_idx++] << 8) | (_buf[_idx++]))

static uint8_t *m_output_r;
static uint8_t *m_output_g;
static uint8_t *m_output_b;
static unsigned m_stride; 

#define dprintf_blk(_name, _arr, _max) for (int __i=0;__i<_max;__i++) { dprintf("%s: %d -> %d\n", _name, __i, _arr[__i]); }

//-----------------------------------------------------------------------------
// ConvertYUV2RGB: Convert from YUV to RGB
//-----------------------------------------------------------------------------
static void ConvertYUV2RGB(int block_num, int *y, int *cb, int *cr)
{
    int x_blocks = (m_width / 8);

    // If width is not a multiple of 8, round up
    if (m_width % 8)
        x_blocks++;

    int x_start = (block_num % x_blocks) * 8;
    int y_start = (block_num / x_blocks) * 8;

    if (m_mode == JPEG_MONOCHROME)
    {
        for (int i=0;i<64;i++)
        {
            int r = 128 + y[i];
            int g = 128 + y[i];
            int b = 128 + y[i];

            // Avoid overflows
            r = (r & 0xffffff00) ? (r >> 24) ^ 0xff : r;
            g = (g & 0xffffff00) ? (g >> 24) ^ 0xff : g;
            b = (b & 0xffffff00) ? (b >> 24) ^ 0xff : b;

            int _x = x_start + (i % 8);
            int _y = y_start + (i / 8);
            int offset = (_y * m_stride) + _x;

            dprintf("RGB: r=%d g=%d b=%d -> %d\n", r, g, b, offset);
            m_output_r[offset*4] = r;
            m_output_g[offset*4] = g;
            m_output_b[offset*4] = b;
        }
    }
    else
    {
        for (int i=0;i<64;i++)
        {
            int r = 128 + y[i] + (cr[i] * 1.402);
            int g = 128 + y[i] - (cb[i] * 0.34414) - (cr[i] * 0.71414);
            int b = 128 + y[i] + (cb[i] * 1.772);

            // Avoid overflows
            r = (r & 0xffffff00) ? (r >> 24) ^ 0xff : r;
            g = (g & 0xffffff00) ? (g >> 24) ^ 0xff : g;
            b = (b & 0xffffff00) ? (b >> 24) ^ 0xff : b;

            int _x = x_start + (i % 8);
            int _y = y_start + (i / 8);
            int offset = (_y * m_stride) + _x;

            if (_x < m_width && _y < m_height)
            {
                dprintf("RGB: r=%d g=%d b=%d -> %d [x=%d,y=%d]\n", r, g, b, offset, _x, _y);
                m_output_r[offset*4] = r;
                m_output_g[offset*4] = g;
                m_output_b[offset*4] = b;
            }
        }
    }
}
//-----------------------------------------------------------------------------
// DecodeImage: Decode image data section (supports 4:4:4, 4:2:0, monochrom)
//-----------------------------------------------------------------------------
static bool DecodeImage(jpeg_state& st)
{
	jpeg_dqt& m_dqt = st.m_dqt;
	jpeg_dht& m_dht = st.m_dht;
	jpeg_idct& m_idct = st.m_idct;
	jpeg_bit_buffer& m_bit_buffer = st.m_bit_buffer;
	jpeg_mcu_block& m_mcu_dec = st.m_mcu_dec;
	
    int16_t dc_coeff_Y = 0;
    int16_t dc_coeff_Cb= 0;
    int16_t dc_coeff_Cr= 0;
    int     sample_out[64];
    int     block_out[64];
    int     y_dct_out[4*64];
    int     cb_dct_out[64];
    int     cr_dct_out[64];
    int     count = 0;
    int     loop = 0;

    int block_num = 0;
    while (!m_bit_buffer.eof())
    {
        // [Y0 Y1 Y2 Y3 Cb Cr] x N
        if (m_mode == JPEG_YCBCR_420)
        {
            // Y0
            count = m_mcu_dec.decode(DHT_TABLE_Y_DC_IDX, dc_coeff_Y, sample_out);
            m_dqt.process_samples(m_dqt_table[0], sample_out, block_out, count);
            dprintf_blk("DCT-IN", block_out, 64);
            m_idct.process(block_out, &y_dct_out[0]);

            // Y1
            count = m_mcu_dec.decode(DHT_TABLE_Y_DC_IDX, dc_coeff_Y, sample_out);
            m_dqt.process_samples(m_dqt_table[0], sample_out, block_out, count);
            dprintf_blk("DCT-IN", block_out, 64);
            m_idct.process(block_out, &y_dct_out[64]);

            // Y2
            count = m_mcu_dec.decode(DHT_TABLE_Y_DC_IDX, dc_coeff_Y, sample_out);
            m_dqt.process_samples(m_dqt_table[0], sample_out, block_out, count);
            dprintf_blk("DCT-IN", block_out, 64);
            m_idct.process(block_out, &y_dct_out[128]);

            // Y3
            count = m_mcu_dec.decode(DHT_TABLE_Y_DC_IDX, dc_coeff_Y, sample_out);
            m_dqt.process_samples(m_dqt_table[0], sample_out, block_out, count);
            dprintf_blk("DCT-IN", block_out, 64);
            m_idct.process(block_out, &y_dct_out[192]);

            // Cb
            count = m_mcu_dec.decode(DHT_TABLE_CX_DC_IDX, dc_coeff_Cb, sample_out);
            m_dqt.process_samples(m_dqt_table[1], sample_out, block_out, count);
            dprintf_blk("DCT-IN", block_out, 64);
            m_idct.process(block_out, &cb_dct_out[0]);

            // Cr
            count = m_mcu_dec.decode(DHT_TABLE_CX_DC_IDX, dc_coeff_Cr, sample_out);
            m_dqt.process_samples(m_dqt_table[2], sample_out, block_out, count);
            dprintf_blk("DCT-IN", block_out, 64);
            m_idct.process(block_out, &cr_dct_out[0]);

            // Expand Cb/Cr samples to match Y0-3
            int cb_dct_out_x2[256];
            int cr_dct_out_x2[256];

            for (int i=0;i<64;i++)
            {
                int x = i % 8;
                int y = i / 16;
                int sub_idx = (y * 8) + (x / 2);
                cb_dct_out_x2[i] = cb_dct_out[sub_idx];
                cr_dct_out_x2[i] = cr_dct_out[sub_idx];
            }

            for (int i=0;i<64;i++)
            {
                int x = i % 8;
                int y = i / 16;
                int sub_idx = (y * 8) + 4 + (x / 2);
                cb_dct_out_x2[64 + i] = cb_dct_out[sub_idx];
                cr_dct_out_x2[64 + i] = cr_dct_out[sub_idx];
            }

            for (int i=0;i<64;i++)
            {
                int x = i % 8;
                int y = i / 16;
                int sub_idx = 32 + (y * 8) + (x / 2);
                cb_dct_out_x2[128+i] = cb_dct_out[sub_idx];
                cr_dct_out_x2[128+i] = cr_dct_out[sub_idx];
            }

            for (int i=0;i<64;i++)
            {
                int x = i % 8;
                int y = i / 16;
                int sub_idx = 32 + (y * 8) + 4 + (x / 2);
                cb_dct_out_x2[192 + i] = cb_dct_out[sub_idx];
                cr_dct_out_x2[192 + i] = cr_dct_out[sub_idx];
            }

            int mcu_width = m_width / 8;
            if (m_width % 8)
                mcu_width++;

            // Output all 4 blocks of pixels
            ConvertYUV2RGB((block_num/2) + 0, &y_dct_out[0],  &cb_dct_out_x2[0], &cr_dct_out_x2[0]);
            ConvertYUV2RGB((block_num/2) + 1, &y_dct_out[64], &cb_dct_out_x2[64], &cr_dct_out_x2[64]);
            ConvertYUV2RGB((block_num/2) + mcu_width + 0, &y_dct_out[128], &cb_dct_out_x2[128], &cr_dct_out_x2[128]);
            ConvertYUV2RGB((block_num/2) + mcu_width + 1, &y_dct_out[192], &cb_dct_out_x2[192], &cr_dct_out_x2[192]);
            block_num += 4;

            if (++loop == (mcu_width / 2))
            {
                block_num += (mcu_width * 2);
                loop = 0;
            }
        }
        // [Y Cb Cr] x N
        else if (m_mode == JPEG_YCBCR_444)
        {
            // Y
            count = m_mcu_dec.decode(DHT_TABLE_Y_DC_IDX, dc_coeff_Y, sample_out);
            m_dqt.process_samples(m_dqt_table[0], sample_out, block_out, count);
            dprintf_blk("DCT-IN", block_out, 64);
            m_idct.process(block_out, &y_dct_out[0]);

            // Cb
            count = m_mcu_dec.decode(DHT_TABLE_CX_DC_IDX, dc_coeff_Cb, sample_out);
            m_dqt.process_samples(m_dqt_table[1], sample_out, block_out, count);
            dprintf_blk("DCT-IN", block_out, 64);
            m_idct.process(block_out, &cb_dct_out[0]);

            // Cr
            count = m_mcu_dec.decode(DHT_TABLE_CX_DC_IDX, dc_coeff_Cr, sample_out);
            m_dqt.process_samples(m_dqt_table[2], sample_out, block_out, count);
            dprintf_blk("DCT-IN", block_out, 64);
            m_idct.process(block_out, &cr_dct_out[0]);

            ConvertYUV2RGB(block_num++, y_dct_out, cb_dct_out, cr_dct_out);
        }
        // [Y] x N
        else if (m_mode == JPEG_MONOCHROME)
        {
            // Y
            count = m_mcu_dec.decode(DHT_TABLE_Y_DC_IDX, dc_coeff_Y, sample_out);
            m_dqt.process_samples(m_dqt_table[0], sample_out, block_out, count);
            dprintf_blk("DCT-IN", block_out, 64);
            m_idct.process(block_out, &y_dct_out[0]);

            ConvertYUV2RGB(block_num++, y_dct_out, cb_dct_out, cr_dct_out);
        }
    }

    return true;
}


extern "C" int ultraembedded_jpeg_decompress(const uint8_t *jpegdata, size_t jpegdata_size, uint8_t *dst, unsigned stride)
{
#warning this needs to be initialized here until calling c++ constructors from the linker script works

	static jpeg_state st;
	jpeg_dqt& m_dqt = st.m_dqt;
	jpeg_dht& m_dht = st.m_dht;
	jpeg_idct& m_idct = st.m_idct;
	jpeg_bit_buffer& m_bit_buffer = st.m_bit_buffer;
	jpeg_mcu_block& m_mcu_dec = st.m_mcu_dec;

    // source data file
    uint8_t *buf = (uint8_t *) jpegdata;
    int      len = jpegdata_size;

    m_dqt.reset();
    m_dht.reset();
    m_idct.reset();
    m_mode = JPEG_UNSUPPORTED;

    m_output_r = dst+0;
    m_output_g = dst+1;
    m_output_b = dst+2;
    m_stride = stride/4;


    uint8_t last_b = 0;
    bool decode_done = false;
    for (int i=0;i<len;)
    {
        if(last_b == 0xFF)
          dprintf("New section at offset %d:\r\n", i);

        uint8_t b = buf[i++];

        //-----------------------------------------------------------------------------
        // SOI: Start of image
        //-----------------------------------------------------------------------------
        if (last_b == 0xFF && b == 0xd8)
            dprintf("Section: SOI\r\n");
        //-----------------------------------------------------------------------------
        // SOF0: Indicates that this is a baseline DCT-based JPEG
        //-----------------------------------------------------------------------------
        else if (last_b == 0xFF && b == 0xc0)
        {
            dprintf("Section: SOF0\r\n");
            int seg_start = i;

            // Length of the segment
            uint16_t seg_len   = get_word(buf, i);

            // Precision of the frame data
            uint8_t  precision = get_byte(buf,i);

            // Image height in pixels
            m_height = get_word(buf, i);

            // Image width in pixels
            m_width = get_word(buf, i);
            
            if(m_width != 640 || m_height != 480)
            	return 0;

            // # of components (n) in frame, 1 for monochrom, 3 for colour images
            uint8_t num_comps = get_byte(buf,i);
            assert(num_comps <= 3);

            dprintf(" x=%d, y=%d, components=%d\r\n", m_width, m_height, num_comps);
            uint8_t comp_id[3];
            uint8_t comp_sample_factor[3];
            uint8_t horiz_factor[3];
            uint8_t vert_factor[3];

            for (int x=0;x<num_comps;x++)
            {
                // First byte identifies the component
                comp_id[x] = get_byte(buf,i);
                // id: 1 = Y, 2 = Cb, 3 = Cr

                // Second byte represents sampling factor (first four MSBs represent horizonal, last four LSBs represent vertical)
                comp_sample_factor[x] = get_byte(buf,i);
                horiz_factor[x]       = comp_sample_factor[x] >> 4;
                vert_factor[x]        = comp_sample_factor[x] & 0xF;

                // Third byte represents which quantization table to use for this component
                m_dqt_table[x]        = get_byte(buf,i);
                dprintf(" num: %d a: %02x b: %02x\r\n", comp_id[x], comp_sample_factor[x], m_dqt_table[x]);
                dprintf(" horiz_factor: %d, vert_factor: %d\r\n", horiz_factor[x], vert_factor[x]);
            }

            m_mode = JPEG_UNSUPPORTED;

            // Single component (Y)
            if (num_comps == 1)
            {
                dprintf(" Mode: Monochrome\r\n");
                m_mode = JPEG_MONOCHROME;
            }
            // Colour image (YCbCr)
            else if (num_comps == 3)
            {
                // YCbCr ordering expected
                if (comp_id[0] == 1 && comp_id[1] == 2 && comp_id[2] == 3)
                {
                    if (horiz_factor[0] == 1 && vert_factor[0] == 1 &&
                        horiz_factor[1] == 1 && vert_factor[1] == 1 &&
                        horiz_factor[2] == 1 && vert_factor[2] == 1)
                    {
                        m_mode = JPEG_YCBCR_444;
                        dprintf(" Mode: YCbCr 4:4:4\r\n");
                    }
                    else if (horiz_factor[0] == 2 && vert_factor[0] == 2 &&
                             horiz_factor[1] == 1 && vert_factor[1] == 1 &&
                             horiz_factor[2] == 1 && vert_factor[2] == 1)
                    {
                        m_mode = JPEG_YCBCR_420;
                        dprintf(" Mode: YCbCr 4:2:0\r\n");
                    }
                }
            }

            i = seg_start + seg_len;
        }
        //-----------------------------------------------------------------------------
        // DQT: Quantisation table
        //-----------------------------------------------------------------------------
        else if (last_b == 0xFF && b == 0xdb)
        {
            dprintf("Section: DQT Table\r\n");
            int seg_start = i;
            uint16_t seg_len   = get_word(buf, i);
            m_dqt.process(&buf[i], seg_len);
            i = seg_start + seg_len;
        }
        //-----------------------------------------------------------------------------
        // DHT: Huffman table
        //-----------------------------------------------------------------------------
        else if (last_b == 0xFF && b == 0xc4)
        {
            int seg_start = i;
            uint16_t seg_len   = get_word(buf, i);
            dprintf("Section: DHT Table\r\n");
            m_dht.process(&buf[i], seg_len);
            i = seg_start + seg_len;
        }
        //-----------------------------------------------------------------------------
        // EOI: End of image
        //-----------------------------------------------------------------------------
        else if (last_b == 0xFF && b == 0xd9)
        {
            dprintf("Section: EOI\r\n");
            break;
        }
        //-----------------------------------------------------------------------------
        // SOS: Start of Scan Segment (SOS)
        //-----------------------------------------------------------------------------
        else if (last_b == 0xFF && b == 0xda)
        {
            dprintf("Section: SOS\r\n");
            int seg_start = i;

            if (m_mode == JPEG_UNSUPPORTED)
            {
                dprintf("ERROR: Unsupported JPEG mode\r\n");
                break;
            }

            uint16_t seg_len   = get_word(buf, i);

            // Component count (n)
            uint8_t  comp_count = get_byte(buf,i);

            // Component data
            for (int x=0;x<comp_count;x++)
            {
                // First byte denotes component ID
                uint8_t comp_id = get_byte(buf,i);

                // Second byte denotes the Huffman table used (first four MSBs denote Huffman table for DC, and last four LSBs denote Huffman table for AC)
                uint8_t comp_table = get_byte(buf,i);

                dprintf(" %d: ID=%x Table=%x\r\n", x, comp_id, comp_table);
            }

            // Skip bytes
            get_byte(buf,i);
            get_byte(buf,i);
            get_byte(buf,i);

            i = seg_start + seg_len;

            //-----------------------------------------------------------------------
            // Process data segment
            //-----------------------------------------------------------------------
	        if(last_b == 0xFF)
	          dprintf("Reset bit_buffer to %d:\r\n", len);
            m_bit_buffer.reset(len);
            while (i < len)
            {
                b = buf[i];
                if (m_bit_buffer.push(b))
                    i++;
                // Marker detected (reverse one byte)
                else
                {
                    i--;
                    break;
                }
            }

            decode_done = DecodeImage(st);
        }
        //-----------------------------------------------------------------------------
        // Unsupported / Skipped
        //-----------------------------------------------------------------------------        
        else if (last_b == 0xFF && b == 0xc2)
        {
            dprintf("Section: SOF2\r\n");
            int seg_start = i;
            uint16_t seg_len   = get_word(buf, i);
            i = seg_start + seg_len;

            dprintf("ERROR: Progressive JPEG not supported\r\n");
            break; // ERROR: Not supported
        }
        else if (last_b == 0xFF && b == 0xdd)
        {
            dprintf("Section: DRI\r\n");
            int seg_start = i;
            uint16_t seg_len   = get_word(buf, i);
            i = seg_start + seg_len;            
        }
        else if (last_b == 0xFF && b >= 0xd0 && b <= 0xd7)
        {
            dprintf("Section: RST%d\r\n", b - 0xd0);
            int seg_start = i;
            uint16_t seg_len   = get_word(buf, i);
            i = seg_start + seg_len;
        }
        else if (last_b == 0xFF && b >= 0xe0 && b <= 0xef)
        {
            dprintf("Section: APP%d\r\n", b - 0xe0);
            int seg_start = i;
            uint16_t seg_len   = get_word(buf, i);
            i = seg_start + seg_len;
        }
        else if (last_b == 0xFF && b == 0xfe)
        {
            dprintf("Section: COM\r\n");
            int seg_start = i;
            uint16_t seg_len   = get_word(buf, i);
            i = seg_start + seg_len;
        }

        last_b = b;
    }

    if (decode_done)
    {
        dprintf("Done decoding\r\n");
    }
    else
        dprintf("ERROR: Can't decode\r\n");

    return decode_done ? 0 : -1;
}
