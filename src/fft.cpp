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

#include <methcla/plugins/fft.h>

#include <algorithm>
#include <iostream>
#include <oscpp/server.hpp>
#include <oscpp/client.hpp>
#include <unistd.h>
#include <math.h>
#include <vector>
#include "ffft/FFTReal.h"

static const double kPi = 3.14159265358979323846264338327950288;

typedef enum {
    kFFT_input_0,
    kFFT_output_0,
    kFFTPorts
} PortIndex;

// Synth Struct, size of Synth ist dieses Struct
typedef struct {
    float* ports[kFFTPorts];
    size_t fftSize;
    size_t fftCycles;
    size_t fftCurCycle;
    float* fftBuf;
    float* sigBuf;    
    float* win;
} Synth;

struct Options {
    size_t fftSize;
};

extern "C" {

size_t kNumItems;

static bool
port_descriptor( const Methcla_SynthOptions* /* options */
               , Methcla_PortCount index
               , Methcla_PortDescriptor* port )
{
    switch ((PortIndex)index) {
        case kFFT_input_0:
            port->type = kMethcla_AudioPort;
            port->direction = kMethcla_Input;
            port->flags = kMethcla_PortFlags;
            return true;
        case kFFT_output_0:
            port->type = kMethcla_AudioPort;
            port->direction = kMethcla_Output;
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
        options->fftSize = argStream.int32();       
    }

static void
construct( const Methcla_World* world
         , const Methcla_SynthDef* /* synthDef */
         , const Methcla_SynthOptions* inOptions
         , Methcla_Synth* synth )
{
    Synth* self = (Synth*)synth;
    
    Options* options = (Options*)inOptions;
    self->fftSize = options->fftSize*2;
    
    self->fftCycles = (self->fftSize)/ methcla_world_block_size(world);
    self->fftBuf = (float *)malloc(self->fftSize * sizeof(float));
    self->win = (float *)malloc(self->fftSize * sizeof(float));
    self->sigBuf = (float *)malloc(self->fftSize * sizeof(float));
    for (int i=0; i<(int)self->fftSize; i++) {
        self->win[i]=0.5*(1-cos((2.f*kPi*i)/(self->fftSize-1)));
        self->fftBuf[i]=0;
        self->sigBuf[i]=0;
    }
    
    kNumItems = options->fftSize;
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
    
static void get_fft(const Methcla_Host* host, void* data)
    {
        float* values = (float*)data;

        // Allocate data for OSC packet
        size_t packetSize = 128 + kNumItems + kNumItems*sizeof(float);
        void* packetData = methcla_host_alloc(host, packetSize);
        assert(packetData != nullptr); // TODO: error handling
        
        // Construct OSC packet
        OSCPP::Client::DynamicPacket packet(packetSize);
        // There's no namespace schema for notifications yet
        packet.openMessage("/fft",  OSCPP::Tags::array(kNumItems));
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
    size_t kNumItems = self->fftSize/2;

    float* in = self->ports[kFFT_input_0];
    float* out = self->ports[kFFT_output_0];

    if (kNumItems < numFrames){
        for (size_t k = 0; k < kNumItems; k++) {
            self->sigBuf[k] = in[k] * self->win[k];
            out[k] = in[k];
        }
        
        ffft::FFTReal<float> fft(self->fftSize);
        float* value = (float*)methcla_world_alloc(world, kNumItems * sizeof(float));
        assert(value != nullptr); // TODO: error handling
        
        //do fft
        fft.do_fft(self->fftBuf, self->sigBuf);
        
        for (int i = 0; i < kNumItems; ++i)
        {
            //normalize and correct fft for hanning window
            value[i] =  (sqrt(pow(self->fftBuf[i],2) + pow(self->fftBuf[i + self->fftSize/2], 2))) / ((self->fftSize/2)/2);
        }
        
        methcla_world_perform_command(world, get_fft, value);
        self->fftCurCycle=1;
        
    }
    
    else {
        for (size_t k = 0; k < numFrames; k++) {
            self->sigBuf[k + numFrames * (self->fftCurCycle-1) ] = in[k] * self->win[k + numFrames * (self->fftCurCycle-1)];
            //self->sigBuf[k + numFrames* (self->fftCurCycle-1) ] = in[k];
            out[k] = in[k];
        }
    }
        
    //std::cout  << self->win[fftSize] << std::endl;

    self->fftCurCycle++;

    if(self->fftCurCycle == self->fftCycles){
        
        ffft::FFTReal<float> fft(self->fftSize);
        float* value = (float*)methcla_world_alloc(world, kNumItems * sizeof(float));
        assert(value != nullptr); // TODO: error handling
    
        //do fft
        fft.do_fft(self->fftBuf, self->sigBuf);

        for (int i = 0; i < kNumItems; ++i)
        {
            //normalize and correct fft for hanning window
            value[i] =  (sqrt(pow(self->fftBuf[i],2) + pow(self->fftBuf[i + self->fftSize/2], 2))) / ((self->fftSize/2)/2);
        }
        
        methcla_world_perform_command(world, get_fft, value);
        self->fftCurCycle=1;
    };
}

} // extern "C"

static void
destroy(const Methcla_World* world, Methcla_Synth* synth)
{
    Synth* self = (Synth*)synth;
    methcla_world_free(world, self->fftBuf);
    methcla_world_free(world, self->sigBuf);
    methcla_world_free(world, self->win);
}

static const Methcla_SynthDef descriptor =
{
    METHCLA_PLUGINS_FFT_URI,
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

METHCLA_EXPORT const Methcla_Library* methcla_plugins_fft(const Methcla_Host* host, const char* /* bundlePath */)
{
    methcla_host_register_synthdef(host, &descriptor);
    return &library;
}
