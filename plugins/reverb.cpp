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

#include <methcla/plugins/reverb.h>

#include <iostream>
#include <oscpp/server.hpp>
#include <unistd.h>
#include <math.h>
#include "freeverb/revmodel.hpp"

revmodel model;

typedef enum {
    kReverb_room,
    kReverb_damp,
    kReverb_wet,
    kReverb_dry,
    kReverb_input_0,
    kReverb_input_1,
    kReverb_output_0,
    kReverb_output_1,
    kReverbPorts
} PortIndex;


// Synth Struct, size of Synth ist dieses Struct
typedef struct 
{
    float* ports[kReverbPorts];
} Synth;

extern "C" {

static bool
port_descriptor( const Methcla_SynthOptions* /* options */
               , Methcla_PortCount index
               , Methcla_PortDescriptor* port )
{
    switch ((PortIndex)index) {
        case kReverb_room:
        case kReverb_damp:
        case kReverb_wet:
        case kReverb_dry:
            port->type = kMethcla_ControlPort;
            port->direction = kMethcla_Input;
            port->flags = kMethcla_PortFlags;
            return true;
        case kReverb_output_0:
        case kReverb_output_1:        
            port->type = kMethcla_AudioPort;
            port->direction = kMethcla_Output;
            port->flags = kMethcla_PortFlags;
            return true;
        case kReverb_input_0:
        case kReverb_input_1:
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
}

static void
connect( Methcla_Synth* synth
       , Methcla_PortCount index
       , void* data )
{
    ((Synth*)synth)->ports[index] = (float*)data;
}

// partially based on the Audio Programming Book by Lazarini
static void
process(const Methcla_World* world, Methcla_Synth* synth, size_t numFrames)
{
    Synth* self = (Synth*)synth;
    
    float* in_L = self->ports[kReverb_input_0];
    float* in_R = self->ports[kReverb_input_1];
    float* out_L = self->ports[kReverb_output_0];
    float* out_R = self->ports[kReverb_output_1];

    const float room = *self->ports[kReverb_room];
    const float damp = *self->ports[kReverb_damp];
    const float wet = *self->ports[kReverb_wet];
    const float dry = *self->ports[kReverb_dry];

    model.setroomsize(room);
    model.setdamp(damp);
    model.setwet(wet);
    model.setdry(dry);
    model.processreplace(in_L, in_R, out_L, out_R, numFrames, 1);
}

} // extern "C"


static const Methcla_SynthDef descriptor =
{
    METHCLA_PLUGINS_REVERB_URI,
    sizeof(Synth),
    NULL, 
    NULL,
    port_descriptor,
    construct,
    connect,
    NULL,
    process,
    NULL
};

static const Methcla_Library library = { NULL, NULL };

METHCLA_EXPORT const Methcla_Library* methcla_plugins_reverb(const Methcla_Host* host, const char* /* bundlePath */)
{
    methcla_host_register_synthdef(host, &descriptor);
    return &library;
}
