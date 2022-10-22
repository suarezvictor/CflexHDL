"""
Unless otherwise stated in individual files, the following license applies:

Copyright (C) 2020 Konrad Beckmann
Copyright (C) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Other authors retain ownership of their contributions. If a submission can reasonably be considered independently copyrightable, it's yours and we encourage you to claim it with appropriate copyright notices. This submission then falls under the "otherwise noted" category. All submissions are strongly encouraged to use the two-clause BSD license reproduced above.
"""
#originall taken from https://github.com/kbeckmann/pergola_projects/blob/master/pergola/gateware/vga_testimage.py

from migen import *
class StaticTestImageGenerator(Module):
    def __init__(self, h_ctr, v_ctr, pix_active, vsync, r, g, b, speed=0):
        self.vsync = vsync
        self.v_ctr = v_ctr
        self.h_ctr = h_ctr
        self.r = r
        self.g = g
        self.b = b
        self.pix_active = pix_active

        # # #

        self.sync += self.r.eq(self.h_ctr)
        self.sync += self.g.eq(self.v_ctr)
        self.sync += self.b.eq(127)


class TestImageGenerator(Module):
    def __init__(self, h_ctr, v_ctr, pix_active, vsync, r, g, b, speed=1):
        self.vsync = vsync
        self.v_ctr = v_ctr
        self.h_ctr = h_ctr
        self.r = r
        self.g = g
        self.b = b
        self.pix_active = pix_active
        
        self.frame = Signal(11)
        self.speed = speed

        # # #

        vsync = self.vsync
        v_ctr = self.v_ctr
        h_ctr = self.h_ctr

        frame = self.frame
        vsync_r = Signal()
        self.sync += If(~vsync_r & vsync,
            frame.eq(frame + speed)
            )
        self.sync += vsync_r.eq(vsync) #FIXME: should be independent of position

        frame_tri = Mux(frame[8], ~frame[:8], frame[:8])
        frame_tri2 = Mux(frame[9], ~frame[1:9], frame[1:9])

        X = Mux(v_ctr[6], h_ctr + frame[self.speed:], h_ctr - frame[self.speed:])
        Y = v_ctr

        self.sync += r.eq(frame_tri[1:])
        self.sync += g.eq(v_ctr * Mux(X & Y, 255, 0))
        self.sync += b.eq(~(frame_tri2 + (X ^ Y)) * 255)


class RotozoomImageGenerator(Module):
    def __init__(self, h_ctr, v_ctr, pix_active, vsync, r, g, b, speed=1, width=640, height=480):
        self.vsync = vsync
        self.v_ctr = v_ctr
        self.h_ctr = h_ctr
        self.r = r
        self.g = g
        self.b = b
        self.pix_active = pix_active

        self.frame = Signal(11)
        self.speed = speed
        self.width = width
        self.height = height

        self.r = r
        self.g = g
        self.b = b

        # # #

        frame = self.frame

        vsync_r = Signal()
        self.sync += If(~vsync_r & vsync,
            frame.eq(frame + speed)
            )
        self.sync += vsync_r.eq(vsync) #FIXME: should be independent of position

        frame_tri = Mux(frame[10], ~frame, frame)[:10]

        X = Signal((16, True))
        Y = Signal.like(X)
        T = Signal.like(X)

        XX = Signal.like(X)
        YY = Signal.like(X)
        TT = Signal.like(X)

        self.comb += [
            XX.eq(h_ctr),
            YY.eq(v_ctr),
            TT.eq(frame_tri),
            X.eq(XX - (self.width >> 1)),
            Y.eq(YY - (self.height >> 1)),
            T.eq(TT - 512),
        ]

        S = ((X+(Y*T)[6:]) & ((X*T)[6:]-Y))
        ON = (S[3:9] == 0)

        CIRCLE = (X * X + Y * Y)[8:]

        self.sync += If(CIRCLE[8:],
                r.eq(0), g.eq(0), b.eq(0),
            ).Else(
                r.eq( Mux(ON, CIRCLE^255, 0) ),
                g.eq( Mux(ON, 128, 0) ),
                b.eq( CIRCLE^255),
            )
 
