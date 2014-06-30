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

#include <methcla/plugins/pan.h>

#include <iostream>
#include <oscpp/server.hpp>
#include <unistd.h>


// Synth Struct, size of Synth ist dieses Struct
typedef struct {
    //float* ports[kPanPorts];
    float** ports;
    size_t numPorts;
    size_t inPorts;
    size_t outPorts;
} Synth;

struct Options
    {
        size_t outPorts;
        size_t inPorts;
        size_t numPorts;
};

extern "C" {
    
    static bool
    port_descriptor( const Methcla_SynthOptions* outOptions
                    , Methcla_PortCount index
                    , Methcla_PortDescriptor* port )
    {
          
        Options* options = (Options*)outOptions; 
        int ins = options->inPorts;
        int outs = options->outPorts;
        int ports = options->numPorts;        

        if (index < ins) {
          port->type = kMethcla_AudioPort;
          port->direction = kMethcla_Input;
          port->flags = kMethcla_PortFlags;
          //std::cout << "ins" << std::endl;
          return true;
        }
        
        else if ((index >= ins) && (index < (ins+outs))) {
          port->type = kMethcla_AudioPort;
          port->direction = kMethcla_Output;
          port->flags = kMethcla_PortFlags;
          //std::cout << "outs" << std::endl;
          return true;
        }

        else if ((index >= (ins+outs)) && (index < (ports))) {
          port->type = kMethcla_ControlPort;
          port->direction = kMethcla_Input;
          port->flags = kMethcla_PortFlags;
          //std::cout << "controls" << std::endl;
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
        options->inPorts = argStream.int32();
        options->outPorts = argStream.atEnd() ? false : argStream.int32();
        options->numPorts = options->outPorts + (options->inPorts*2);
       
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
        self->inPorts = options->inPorts;
        self->outPorts = options->outPorts;
    }
    
    static void
    process(const Methcla_World* world, Methcla_Synth* synth, size_t numFrames)
    {
        Synth* self = (Synth*)synth; 
        outPorts = self->outPorts;
        float* out = self->ports[kWhiteNoise_output_0];

        for (size_t k = 0; k < numFrames; k++) 
        {
        
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
    METHCLA_PLUGINS_PAN_URI,
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

METHCLA_EXPORT const Methcla_Library* methcla_plugins_pan(const Methcla_Host* host, const char* /* bundlePath */)
{
    methcla_host_register_synthdef(host, &descriptor);
    return &library;
}
