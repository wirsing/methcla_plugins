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

#include <methcla/plugins/delay.h>

#include <iostream>
#include <oscpp/server.hpp>
#include <unistd.h>
#include <math.h>

#define PI 3.141592653589793

typedef enum {
    kDel_time,
    kDel_fb,
    kDel_input_0,
    kDel_output_0,
    kDelPorts
} PortIndex;


// Synth Struct, size of Synth ist dieses Struct
typedef struct 
{
    float* ports[kDelPorts];
    float* delay;
    int sampleRate;
    float maxDelay;
    int wp;
} Synth;

struct Options
{
    float maxDelay;
};

extern "C" {

static bool
port_descriptor( const Methcla_SynthOptions* /* options */
               , Methcla_PortCount index
               , Methcla_PortDescriptor* port )
{
    switch ((PortIndex)index) {
        case kDel_time:
        case kDel_fb:
            port->type = kMethcla_ControlPort;
            port->direction = kMethcla_Input;
            port->flags = kMethcla_PortFlags;
            return true;
        case kDel_output_0:
            port->type = kMethcla_AudioPort;
            port->direction = kMethcla_Output;
            port->flags = kMethcla_PortFlags;
            return true;
        case kDel_input_0:
            port->type = kMethcla_AudioPort;
            port->direction = kMethcla_Input;
            port->flags = kMethcla_PortFlags;
            return true;
        default:
            return false;
    }
}

static void
configure(const void* tags, size_t tags_size, const void* args, size_t args_size, Methcla_SynthOptions* outOptions)
{
    OSCPP::Server::ArgStream argStream(OSCPP::ReadStream(tags, tags_size), OSCPP::ReadStream(args, args_size));
    Options* options = (Options*)outOptions;
    options->maxDelay = argStream.int32();       
}

static void
construct( const Methcla_World* world
         , const Methcla_SynthDef* /* synthDef */
         , const Methcla_SynthOptions* inOptions
         , Methcla_Synth* synth )
{
    Synth* self = (Synth*)synth;
    Options* options = (Options*)inOptions;
    self->sampleRate = methcla_world_samplerate(world); 
    self->maxDelay = options->maxDelay;
    self->wp = 0;
    int maxSamples = ceil(self->maxDelay*self->sampleRate);

    float* cDelay = new float[maxSamples];
        
    for (int i = 0; i < maxSamples; i++) {
            cDelay[i] = 0;
    }

    self->delay = cDelay;
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
    
    const float vdtime = *self->ports[kDel_time];
    const float fb = *self->ports[kDel_fb];
    float* in = self->ports[kDel_input_0];
    float* out = self->ports[kDel_output_0];
    float* delay = self->delay;
    int wp = self->wp;


    int mdt, rpi;
    float rp, vdt, frac, next;

    // calculate current delaytime in samples (float)
    vdt = vdtime*self->sampleRate;

    // calculate max delaytime in samples (int)
    mdt = (int) (self->maxDelay*self->sampleRate);

    // if current delay time > max then it gets truncated at max delaytime
    if(vdt > mdt) vdt = (float) mdt;

    // callback loop
    for (size_t k = 0; k < numFrames; k++) {

            // set read pointer to write pointer position - current delay time position
            rp = wp-vdt;

            // if read pointer equals or is greater than 0, then rp=rp if rp is lesser than maxdelaytime
            // if rp is greater than maxdelaytime then rp = rp - max dt.
            // if rp is lesser 0 then rp = rp + mdt;

            rp = (rp >= 0 ? (rp < mdt ? rp : rp - mdt) : rp + mdt);
            
            // rpi is casted rp to integer
            rpi = (int) rp;
            
            // calculate the error rest of the cast
            frac = rp-rpi;
            
            // calculate sample if integer cast read pointer unequals max delaytime - 1 then take the following 
            // sample of the read pointer. if not take sample 0 of delay buffer.
            next = (rpi != mdt-1 ? delay[rpi+1] : delay[0]);
            
            // output sample at read buffer + rest of the next sample (linear interpolation)
            out[k] = delay[rpi] + frac*(next - delay[rpi]);
            
            // copy the input to the delay buffer at index write pointer
            delay[wp] = in[k] + out[k]*fb;
                        
            // advance the write pointer by one, if it's at max delaytime - 1 wrap around.
            wp = (wp != mdt-1 ? wp+1 : 0);
    }
    self->wp = wp;
}

} // extern "C"


static const Methcla_SynthDef descriptor =
{
    METHCLA_PLUGINS_DELAY_URI,
    sizeof(Synth),
    sizeof(Options), 
    configure,
    port_descriptor,
    construct,
    connect,
    NULL,
    process,
    NULL
};

static const Methcla_Library library = { NULL, NULL };

METHCLA_EXPORT const Methcla_Library* methcla_plugins_delay(const Methcla_Host* host, const char* /* bundlePath */)
{
    methcla_host_register_synthdef(host, &descriptor);
    return &library;
}
