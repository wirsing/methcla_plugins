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

#include <methcla/plugins/hpf.h>

#include <iostream>
#include <oscpp/server.hpp>
#include <unistd.h>
#include <math.h>

#define PI 3.141592653589793

typedef enum {
    kHPF_freq,
    kHPF_res,
    kHPF_input_0,
    kHPF_output_0,
    kHPFPorts
} PortIndex;


// Synth Struct, size of Synth ist dieses Struct
typedef struct {
    float* ports[kHPFPorts];
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
        case kHPF_freq:
        case kHPF_res:
            port->type = kMethcla_ControlPort;
            port->direction = kMethcla_Input;
            port->flags = kMethcla_PortFlags;
            return true;
        case kHPF_output_0:
            port->type = kMethcla_AudioPort;
            port->direction = kMethcla_Output;
            port->flags = kMethcla_PortFlags;
            return true;
        case kHPF_input_0:
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
    
    const float freq = *self->ports[kHPF_freq];
    const float r = *self->ports[kHPF_res];
    float* in = self->ports[kHPF_input_0];
    float* out = self->ports[kHPF_output_0];
    int sR = self->samplerate;
    float n, w, a1, a2, a3, b1, b2;

    for (size_t k = 0; k < numFrames; k++) {
        
        w = tan (PI*freq/sR);

        n = 1/(pow(w,2) + w/r + 1);

        a1 = n;
        a2 = (-2)*n;
        a3 = n;
        b1 = 2*n*(pow(w,2)-1);
        b2 = n*(pow(w,2) - w/r + 1);

        out[k] = in[k]*a1 + self->del_in[0]*a2 + self->del_in[1]*a3 - self->del_out[0]*b1 - self->del_out[1]*b2;

        self->del_out[1] = self->del_out[0];
        self->del_out[0] = out[k];

        self->del_in[1] = self->del_in[0];
        self->del_in[0] = in[k];

    }   
}

} // extern "C"


static const Methcla_SynthDef descriptor =
{
    METHCLA_PLUGINS_HPF_URI,
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

METHCLA_EXPORT const Methcla_Library* methcla_plugins_hpf(const Methcla_Host* host, const char* /* bundlePath */)
{
    methcla_host_register_synthdef(host, &descriptor);
    return &library;
}
