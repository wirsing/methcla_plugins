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

#include <methcla/plugins/pulse.h>

#include <iostream>
#include <oscpp/server.hpp>
#include <unistd.h>

typedef enum {
    kPulse_freq,
    kPulse_width,
    kPulse_amp,
    kPulse_add,
    kPulse_output_0,
    kPulsePorts
} PortIndex;


// Synth Struct, size of Synth ist dieses Struct
typedef struct {
    float* ports[kPulsePorts];
    double phase;
    double freqmul;
    double width;
} Synth;

extern "C" {

static bool
port_descriptor( const Methcla_SynthOptions* /* options */
               , Methcla_PortCount index
               , Methcla_PortDescriptor* port )
{
    switch ((PortIndex)index) {
        case kPulse_amp:
        case kPulse_freq:
        case kPulse_width:
        case kPulse_add:
            port->type = kMethcla_ControlPort;
            port->direction = kMethcla_Input;
            port->flags = kMethcla_PortFlags;
            return true;
        case kPulse_output_0:
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
    self->phase = 0.f;
    double samplerate = methcla_world_samplerate(world);
    self->freqmul = 1.f/samplerate;
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

    const float amp = *self->ports[kPulse_amp];
    const float freq = *self->ports[kPulse_freq];
    const float width = *self->ports[kPulse_width];
    const float add = *self->ports[kPulse_add];
    float* out = self->ports[kPulse_output_0];
    double phase = self->phase;
    double freqmul = self->freqmul;



    for (size_t k = 0; k < numFrames; k++) {

        float sig;
        if (phase >= 1.f) {
            phase -= 1.f;

            sig = width < 0.5f ? 1.f : 0.f;
        } else {
            sig = phase < width ? 1.f : 0.f;
        }
        phase += freq * freqmul;
        out[k] = sig * amp + add;
    };

    self->phase = phase;
}

} // extern "C"


static const Methcla_SynthDef descriptor =
{
    METHCLA_PLUGINS_PULSE_URI,
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

METHCLA_EXPORT const Methcla_Library* methcla_plugins_pulse(const Methcla_Host* host, const char* /* bundlePath */)
{
    methcla_host_register_synthdef(host, &descriptor);
    return &library;
}
