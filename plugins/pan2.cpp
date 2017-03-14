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

#include <methcla/plugins/pan2.h>

#include <iostream>
#include <algorithm> 
#include <oscpp/server.hpp>
#include <unistd.h>
#include <math.h>  

#define TWOPI 6.283185307179586

 typedef enum {
     kPan2_pos,
     kPan2_amp,
     kPan2_input_0,
     kPan2_output_0,
     kPan2_output_1,
     kPan2Ports
 } PortIndex;
 

// Synth Struct, size of Synth ist dieses Struct
typedef struct {
    float* ports[kPan2Ports];
    float slopeFactor;
    float level;
    float* table;
} Synth;

struct Options {
    float iLevel;
};

extern "C" {
    
    static bool
    port_descriptor( const Methcla_SynthOptions* outOptions
                    , Methcla_PortCount index
                    , Methcla_PortDescriptor* port )
    {
        switch ((PortIndex)index) {
        case kPan2_pos:
        case kPan2_amp:
            port->type = kMethcla_ControlPort;
            port->direction = kMethcla_Input;
            port->flags = kMethcla_PortFlags;
            return true;
        case kPan2_input_0:
            port->type = kMethcla_AudioPort;
            port->direction = kMethcla_Input;
            port->flags = kMethcla_PortFlags;
            return true;
        case kPan2_output_0:
        case kPan2_output_1:
            port->type = kMethcla_AudioPort;
            port->direction = kMethcla_Output;
            port->flags = kMethcla_PortFlags;
            return true;
        default:
            return false;
        }

    }
    /*
    double* createTable(int length = 1024){
        int tableLength = length;
        double* table = new double[length + 2];
        
        double step = TWOPI / length;
        for (int i = 0; i < length + 2; i++) {
            table[i] = sin(step * i);
        }
        return table;
    }
    */  

    static void
    configure(const void* tags, size_t tags_size, const void* args, size_t args_size, Methcla_SynthOptions* outOptions)
    {
    OSCPP::Server::ArgStream argStream(OSCPP::ReadStream(tags, tags_size), OSCPP::ReadStream(args, args_size));
    Options* options = (Options*)outOptions;
    options->iLevel = argStream.float32();
    }


    static void
    construct( const Methcla_World* world
              , const Methcla_SynthDef* /* synthDef */
              , const Methcla_SynthOptions* inOptions
              , Methcla_Synth* synth )
    {
        const Options* options = (const Options*)inOptions;
        Synth* self = (Synth*)synth;
       
        float* cTable = new float[8192 + 2];
        
        float step = TWOPI / 8192;
        for (int i = 0; i < 8192 + 2; i++) {
            cTable[i] = sin(step * i);
        }

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

        const float nextAmp = *self->ports[kPan2_amp];
        const float pos = *self->ports[kPan2_pos];
        float* in = self->ports[kPan2_input_0];
        float* outL = self->ports[kPan2_output_0];
        float* outR = self->ports[kPan2_output_1];
        
        float amp = self->level;
         
        

        if (amp != nextAmp) {
            
            float levelSlope = (nextAmp - amp) * self->slopeFactor; // 1/blocksize

            for (size_t k = 0; k < numFrames; k++) {
                    int ipos = (int)(1024.f * pos + 1024.f + 0.5f);    
                    ipos = std::max(0, std::min(ipos, 2048));

                    float leftamp  = amp * self->table[2048 - ipos]; 
                    float rightamp = amp * self->table[ipos];

                    outL[k] = in[k] * leftamp;
                    outR[k] = in[k] * rightamp;
                    amp += levelSlope;
            };
            self->level = amp;        
        } 
        else {

            for (size_t k = 0; k < numFrames; k++){
                
                int ipos = (int)(1024.f * pos + 1024.f + 0.5f);    
                ipos = std::max(0, std::min(ipos, 2048));

                float leftamp  = amp * self->table[2048 - ipos]; 
                float rightamp = amp * self->table[ipos];

                outL[k] = in[k] * leftamp;
                outR[k] = in[k] * rightamp;
            }
        }
    }
} // extern "C"


static void
destroy(const Methcla_World* world, Methcla_Synth* synth)
{
    Synth* self = (Synth*)synth;
    delete(self->table);
    //methcla_world_free(world, self->ports);
}

static const Methcla_SynthDef descriptor =
{
    METHCLA_PLUGINS_PAN2_URI,
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

METHCLA_EXPORT const Methcla_Library* methcla_plugins_pan2(const Methcla_Host* host, const char* /* bundlePath */)
{
    methcla_host_register_synthdef(host, &descriptor);
    return &library;
}
