// Copyright 2012-2013 Samplecount S.L. + Wilm Thoben
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

#include <methcla/plugins/pinknoise.h>

#include <iostream>
#include <oscpp/server.hpp>
#include <unistd.h>
#include <math.h>
#include "newshadeofpink/pink.h"

/* M_PI is not in C11 */
static const double kPi = 3.14159265358979323846264338327950288;

typedef enum {
    kPinkNoise_amp,
    kPinkNoise_add,    
    kPinkNoise_output_0,
    kPinkNoisePorts
} PortIndex;


// Synth Struct, size of Synth ist dieses Struct
typedef struct {
    float* ports[kPinkNoisePorts];
} Synth;

extern "C" {

static bool
port_descriptor( const Methcla_SynthOptions* /* options */
               , Methcla_PortCount index
               , Methcla_PortDescriptor* port )
{
    switch ((PortIndex)index) {
        case kPinkNoise_amp:
        case kPinkNoise_add:
            port->type = kMethcla_ControlPort;
            port->direction = kMethcla_Input;
            port->flags = kMethcla_PortFlags;
            return true;
        case kPinkNoise_output_0:
            port->type = kMethcla_AudioPort;
            port->direction = kMethcla_Output;
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

    
    const float amp = *self->ports[kPinkNoise_amp];
    const float add = *self->ports[kPinkNoise_add];    
    float* out = self->ports[kPinkNoise_output_0];
    float samples[16];
    pink pinknoise;

    for (size_t k = 0; k < numFrames/16; k++) {
        pinknoise.generate16(samples);
        for (size_t j = 0; j < 16; j++){
            out[j + (k*16)] = samples[j] * amp + add;
        }
    }
}

} // extern "C"


static const Methcla_SynthDef descriptor =
{
    METHCLA_PLUGINS_PINK_NOISE_URI,
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

METHCLA_EXPORT const Methcla_Library* methcla_plugins_pink_noise(const Methcla_Host* host, const char* /* bundlePath */)
{
    methcla_host_register_synthdef(host, &descriptor);
    return &library;
}
