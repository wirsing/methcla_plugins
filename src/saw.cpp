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

#include <methcla/plugins/saw.h>

#include <iostream>
#include <oscpp/server.hpp>
#include <unistd.h>

typedef enum {
    kSaw_freq,
    kSaw_amp,
    kSaw_add,
    kSaw_input_0,
    kSaw_input_1,
    kSaw_input_2,
    kSaw_output_0,
    kSawPorts
} PortIndex;


// Synth Struct, size of Synth ist dieses Struct
typedef struct {
    float* ports[kSawPorts];
    double phase;
    double freqmul;
    size_t samplerate;
} Synth;

extern "C" {
    
    static bool
    port_descriptor( const Methcla_SynthOptions* /* options */
                    , Methcla_PortCount index
                    , Methcla_PortDescriptor* port )
    {
        switch ((PortIndex)index) {
            case kSaw_amp:
            case kSaw_freq:
            case kSaw_add:
                port->type = kMethcla_ControlPort;
                port->direction = kMethcla_Input;
                port->flags = kMethcla_PortFlags;
                return true;
            case kSaw_output_0:
                port->type = kMethcla_AudioPort;
                port->direction = kMethcla_Output;
                port->flags = kMethcla_PortFlags;
                return true;
            case kSaw_input_0:
            case kSaw_input_1:
            case kSaw_input_2:
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
        self->phase = 0.;
        double samplerate = methcla_world_samplerate(world);
        self->freqmul = 2.f/samplerate;
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
        
        const float amp = *self->ports[kSaw_amp];
        const float add = *self->ports[kSaw_add];
        const float freq = *self->ports[kSaw_freq];
        
        float* out = self->ports[kSaw_output_0];
        
        float* a_freq = self->ports[kSaw_input_0];
        float* a_amp = self->ports[kSaw_input_1];
        float* a_add = self->ports[kSaw_input_2];
        
        double phase = self->phase;
        double freqmul = self->freqmul;
        
        for (size_t k = 0; k < numFrames; k++) {
            
            float z = phase;
            phase += (freq + a_freq[k]) * freqmul;
            if (phase >= 1.f) phase -= 2.f;
            else if (phase <= -1.f) phase += 2.f;
            //out[k] = (amp + a_amp[k]) * z + (add + a_add[k]);
            out[k] = a_freq[k];
        }
        
        self->phase = phase;
    }
    
} // extern "C"


static const Methcla_SynthDef descriptor =
{
    METHCLA_PLUGINS_SAW_URI,
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

METHCLA_EXPORT const Methcla_Library* methcla_plugins_saw(const Methcla_Host* host, const char* /* bundlePath */)
{
    methcla_host_register_synthdef(host, &descriptor);
    return &library;
}
