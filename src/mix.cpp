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

#include <methcla/plugins/mix.h>

#include <iostream>
#include <oscpp/server.hpp>
#include <unistd.h>

// Synth Struct, size of Synth ist dieses Struct
typedef struct {
    float** ports;
    size_t numPorts;
    size_t inKPorts;
    size_t inAPorts;
} Synth;

struct Options
    {
        size_t inKPorts;
        size_t inAPorts;
        size_t numPorts;
};

extern "C" {
    
    static bool
    port_descriptor( const Methcla_SynthOptions* outOptions
                    , Methcla_PortCount index
                    , Methcla_PortDescriptor* port )
    {
          
        Options* options = (Options*)outOptions; 
        int inK = options->inKPorts;
        int inA = options->inAPorts;

        if (index < inK) {
          port->type = kMethcla_ControlPort;
          port->direction = kMethcla_Input;
          port->flags = kMethcla_PortFlags;
          std::cout << "k_ins" << index << std::endl;
          return true;
        }
        
        else if ((index >= inK) && (index < (inK+inA))) {
          port->type = kMethcla_AudioPort;
          port->direction = kMethcla_Input;
          port->flags = kMethcla_PortFlags;
          std::cout << "a_ins" << index << std::endl;
          return true;
        }

        else if ((index >= (inK+inA)) && (index < (inK+inA+1))) {
          port->type = kMethcla_AudioPort;
          port->direction = kMethcla_Output;
          port->flags = kMethcla_PortFlags;
          std::cout << "out" << index << std::endl;
          return true;
        }

        else {
          return false;
        }
    }
    
    static void
    configure(const void* tags, size_t tags_size, const void* args, size_t args_size, Methcla_SynthOptions* outOptions)
    {
        OSCPP::Server::ArgStream argStream(OSCPP::ReadStream(tags, tags_size), OSCPP::ReadStream(args, args_size));
        Options* options = (Options*)outOptions;
        options->inKPorts = argStream.int32();
        options->inAPorts = argStream.atEnd() ? false : argStream.int32();
        options->numPorts = options->inKPorts + options->inAPorts + 1;
       
    }
    
    static void
    construct( const Methcla_World* world
              , const Methcla_SynthDef* /* synthDef */
              , const Methcla_SynthOptions* inOptions
              , Methcla_Synth* synth )
    {
        Options* options = (Options*)inOptions;
        Synth* self = (Synth*)synth;
        self->ports = (float**)methcla_world_alloc(world, options->numPorts * sizeof(float*));
        self->numPorts = options->numPorts;
        self->inKPorts = options->inKPorts;
        self->inAPorts = options->inAPorts;
    }
    
    static void
    process(const Methcla_World* world, Methcla_Synth* synth, size_t numFrames)
    {
        Synth* self = (Synth*)synth; 
        size_t inKPorts = self->inKPorts;
        size_t inAPorts = self->inAPorts;
        size_t outPort = self->numPorts;
        
        float* out = self->ports[outPort];
        
        for (size_t j=0; j < inAPorts; j++) {
        const float amp = *self->ports[j];
        float* in = self->ports[j+inKPorts];

            for (size_t k = 0; k < numFrames; k++)
            {
                out[k]=in[k]*amp;
            }
        }

    }
    
} // extern "C"

static void
destroy(const Methcla_World* world, Methcla_Synth* synth)
{
    Synth* self = (Synth*)synth;
    methcla_world_free(world, self->ports);
}


static void
connect( Methcla_Synth* synth
        , Methcla_PortCount index
        , void* data )
{
    ((Synth*)synth)->ports[index] = (float*)data;
}

static const Methcla_SynthDef descriptor =
{
    METHCLA_PLUGINS_MIX_URI,
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

METHCLA_EXPORT const Methcla_Library* methcla_plugins_mix(const Methcla_Host* host, const char* /* bundlePath */)
{
    methcla_host_register_synthdef(host, &descriptor);
    return &library;
}
