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

#include <methcla/plugins/osc.h>

#include <iostream>
#include <algorithm> 
#include <oscpp/server.hpp>
#include <unistd.h>
#include <math.h>  

#define TWOPI 6.283185307179586

 typedef enum {
     kOsc_freq,
     kOsc_phase,
     kOsc_amp,
     kOsc_add,
     kOsc_output_0,
     kOscPorts
 } PortIndex;
 

// Synth Struct, size of Synth ist dieses Struct
typedef struct {
    float* ports[kOscPorts];
    float* table;
} Synth;

struct Options {
    int waveForm;
};

extern "C" {
    
    static bool
    port_descriptor( const Methcla_SynthOptions* outOptions
                    , Methcla_PortCount index
                    , Methcla_PortDescriptor* port )
    {
        switch ((PortIndex)index) {
        case kOsc_freq:
        case kOsc_phase:
        case kOsc_amp:
        case kOsc_add:
            port->type = kMethcla_ControlPort;
            port->direction = kMethcla_Input;
            port->flags = kMethcla_PortFlags;
            return true;
        case kPan2_output_0:
            port->type = kMethcla_AudioPort;
            port->direction = kMethcla_Output;
            port->flags = kMethcla_PortFlags;
            return true;
        default:
            return false;
        }

    }
    
    float* sinTable (int length=1024){
        int tableLength = length;
        float* table = new float[length + 2];
        
        float step = TWOPI / length;
        for (int i = 0; i < length + 2; i++) {
            table[i] = sin(step * i);
        }
        return table;
    }

    float* sawTable (int harms, int length=1024){
        float *amps = new float[harms];
        for(int i = 0; i < harms; i++){
            amps[i] = 1.0 / (i+1);
        };
        float *table = fourierTable(harms,amps,length, -0.25);
        delete[] amps;
        return table;
    }
      
    float* triTable (int harms, int length=1024){
        float *amps = new float[harms];
        memset(amps, 0, sizeof(float)*harms);
        for(int i=0; i < harms; i+=2){
            amps[i] = 1.0/((i+1)*(i+1));
        };
        float *table = fourierTable(harms,amps,length, 0);
        delete[] amps;
        return table;
    }

    float* sqrTable (int harms, int length=1024){
        float *amps = new float[harms];
        memset(amps, 0, sizeof(float)*harms);
        for(int i=0; i < harms; i+=2){
            amps[i] = 1.0/(i+1);
        }   
        float *table = fourierTable(harms,amps,length, -0.25);
        delete[] amps;
        return table;
    }

    static void
    configure(const void* tags, size_t tags_size, const void* args, size_t args_size, Methcla_SynthOptions* outOptions)
    {
    OSCPP::Server::ArgStream argStream(OSCPP::ReadStream(tags, tags_size), OSCPP::ReadStream(args, args_size));
    Options* options = (Options*)outOptions;
    options->waveForm = argStream.int32();
    }


    static void
    construct( const Methcla_World* world
              , const Methcla_SynthDef* /* synthDef */
              , const Methcla_SynthOptions* inOptions
              , Methcla_Synth* synth )
    {
        const Options* options = (const Options*)inOptions;
        Synth* self = (Synth*)synth;
        
        
        self->table = cTable;
        self->level = options->iLevel;
        self->slopeFactor = 1/float(methcla_world_block_size(world));
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

        const float freq = *self->ports[kOsc_freq];
        float* out = self->ports[kOsc_output_0];
        

        for (size_t k = 0; k < numFrames; k++){

        }

} // extern "C"


static void
destroy(const Methcla_World* world, Methcla_Synth* synth)
{
    Synth* self = (Synth*)synth;
    delete(self->table);
    methcla_world_free(world, self->ports);
}

static const Methcla_SynthDef descriptor =
{
    METHCLA_PLUGINS_OSC_URI,
    sizeof(Synth),
    sizeof(Options),
    configure,
    port_descriptor,
    construct,
    connect,
    NULL,
    process,
    destroy
};

static const Methcla_Library library = { NULL, NULL };

METHCLA_EXPORT const Methcla_Library* methcla_plugins_osc(const Methcla_Host* host, const char* /* bundlePath */)
{
    methcla_host_register_synthdef(host, &descriptor);
    return &library;
}
