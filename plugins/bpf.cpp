// Copyright 2012-2013 Samplecount S.L.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <methcla/plugins/bpf.h>

#include <iostream>
#include <oscpp/server.hpp>
#include <unistd.h>
#include <math.h>

#define PI 3.141592653589793

typedef enum {
    kBPF_freq,
    kBPF_bw,
    kBPF_input_0,
    kBPF_output_0,
    kBPFPorts
} PortIndex;


// Synth Struct, size of Synth ist dieses Struct
typedef struct {
    float* ports[kBPFPorts];
    double freqmul;
    size_t samplerate;
    float del_in[2];
    float del_out[2];
} Synth;

extern "C" {

static bool
port_descriptor( const Methcla_SynthOptions* /* options */
               , Methcla_PortCount index
               , Methcla_PortDescriptor* port )
{
    switch ((PortIndex)index) {
        case kBPF_freq:
        case kBPF_bw:
            port->type = kMethcla_ControlPort;
            port->direction = kMethcla_Input;
            port->flags = kMethcla_PortFlags;
            return true;
        case kBPF_output_0:
            port->type = kMethcla_AudioPort;
            port->direction = kMethcla_Output;
            port->flags = kMethcla_PortFlags;
            return true;
        case kBPF_input_0:
            port->type = kMethcla_AudioPort;
            port->direction = kMethcla_Input;
            port->flags = kMethcla_PortFlags;
            return true;
        default:
            return false;
    }
}

static void
construct( const Methcla_World* world
         , const Methcla_SynthDef* /* synthDef */
         , const Methcla_SynthOptions* inOptions
         , Methcla_Synth* synth )
{
    Synth* self = (Synth*)synth;
    self->samplerate = methcla_world_samplerate(world);    
    for (int i = 0; i < 2; ++i)
    {
        self->del_in[i]=0;
        self->del_out[i]=0;
    }
}

static void
connect( Methcla_Synth* synth
       , Methcla_PortCount index
       , void* data )
{
    ((Synth*)synth)->ports[index] = (float*)data;
}

static void
process(const Methcla_World* world, Methcla_Synth* synth, size_t numFrames)
{
    Synth* self = (Synth*)synth;
    
    const float freq = *self->ports[kBPF_freq];
    const float bw = *self->ports[kBPF_bw];
    float* in = self->ports[kBPF_input_0];
    float* out = self->ports[kBPF_output_0];
    int sR = self->samplerate;
    float n, w, a0, a1, a2, b1, b2;

    for (size_t k = 0; k < numFrames; k++) {
        
        w = 1.f / tan ((PI*bw)/sR);
        
        n = 2.f * cos ((2.f*PI*freq)/sR);
        
        a0 = 1.f / (1.f + w);
        a1 = 0.f;
        a2 = -1.f * a0;
        b1 = -1.f * w * n * a0;
        b2 = a0 * (w - 1.f);
        
        /*
        w = tan (PI*freq/sR);

        n = 1/(pow(w,2) + w/r + 1);

        a1 = n*w/r;
        a2 = 0.f;
        a3 = -a1;
        b1 = 2*n*(pow(w,2)-1);
        b2 = n*(pow(w,2) - w/r + 1);
        */
        
        out[k] = self->del_in[0]*a1 + self->del_in[1]*a2 - self->del_out[0]*b1 - self->del_out[1]*b2;

        self->del_out[1] = self->del_out[0];
        self->del_out[0] = out[k];

        self->del_in[1] = self->del_in[0];
        self->del_in[0] = in[k];

    }   
}

} // extern "C"


static const Methcla_SynthDef descriptor =
{
    METHCLA_PLUGINS_BPF_URI,
    sizeof(Synth),
    0, 
    NULL,
    port_descriptor,
    construct,
    connect,
    NULL,
    process,
    NULL
};

static const Methcla_Library library = { NULL, NULL };

METHCLA_EXPORT const Methcla_Library* methcla_plugins_bpf(const Methcla_Host* host, const char* /* bundlePath */)
{
    methcla_host_register_synthdef(host, &descriptor);
    return &library;
}
