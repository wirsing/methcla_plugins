// Copyright 2012-2013 Samplecount S.L.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License assert
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <methcla/plugins/ampfol.h>

#include <algorithm>
#include <iostream>
#include <oscpp/server.hpp>
#include <oscpp/client.hpp>
#include <unistd.h>
#include <math.h>


typedef enum {
    kAmpFol_input_0,
    kAmpFol_output_0,
    kAmpFolPorts
} PortIndex;

// Synth Struct, size of Synth ist dieses Struct
typedef struct {
    float* ports[kAmpFolPorts];
} Synth;

float clip( float n, float lower, float upper )
{
    n = ( n > lower ) * n + !( n > lower ) * lower;
    return ( n < upper ) * n + !( n < upper ) * upper;
}
    
extern "C" {

static const size_t kNumItems = 1;

static bool
port_descriptor( const Methcla_SynthOptions* /* options */
               , Methcla_PortCount index
               , Methcla_PortDescriptor* port )
{
    switch ((PortIndex)index) {
        case kAmpFol_input_0:
            port->type = kMethcla_AudioPort;
            port->direction = kMethcla_Input;
            port->flags = kMethcla_PortFlags;
            return true;
        case kAmpFol_output_0:
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
    
static void perform_world_free(const Methcla_World* world, void* data)
    {
        methcla_world_free(world, data);
    }
    
static void get_amplitude(const Methcla_Host* host, void* data)
    {
        float* values = (float*)data;
        // Allocate data for OSC packet
        const size_t packetSize = 128 + kNumItems + kNumItems*sizeof(float);
        void* packetData = methcla_host_alloc(host, packetSize);
        assert(packetData != nullptr); // TODO: error handling
        
        // Construct OSC packet
        OSCPP::Client::DynamicPacket packet(packetSize);
        // There's no namespace schema for notifications yet
        packet.openMessage("/amplitude", kNumItems);
        for (size_t i=0; i < kNumItems; i++) {
            packet.float32(values[i]);
        }
        packet.closeMessage();
        
        // Send packet to client
        // There's no convenience function yet, trivial to add
        host->notify(host, packet.data(), packet.size());
        
        // Free packet data (client is required to make a copy)
        methcla_host_free(host, packetData);
        
        // Free allocated memory in realtime thread
        methcla_host_perform_command(host, perform_world_free, values);
    }
    
static void
process(const Methcla_World* world, Methcla_Synth* synth, size_t numFrames)
{
    Synth* self = (Synth*)synth;

    float* in = self->ports[kAmpFol_input_0];
    float* out = self->ports[kAmpFol_output_0];
    float average = 0.f;

    for (size_t k = 0; k < numFrames; k++) {
        average += in[k] * in[k];
        out[k] = in[k];
    }

    float* value = (float*)methcla_world_alloc(world, kNumItems * sizeof(float));
    assert(value != nullptr); // TODO: error handling
    
    value[0] = clip(sqrt(average/numFrames), 0, 1);
    methcla_world_perform_command(world, get_amplitude, value);

}

} // extern "C"


static const Methcla_SynthDef descriptor =
{
    METHCLA_PLUGINS_AMPLITUDE_FOLLOWER_URI,
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

METHCLA_EXPORT const Methcla_Library* methcla_plugins_amplitude_follower(const Methcla_Host* host, const char* /* bundlePath */)
{
    methcla_host_register_synthdef(host, &descriptor);
    return &library;
}
