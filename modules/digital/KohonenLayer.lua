--   Copyright (C) 2009, 2010 Andrew Timashov
--
--   This file is part of NeuroWombat.
--
--   NeuroWombat is free software: you can redistribute it and/or modify
--   it under the terms of the GNU General Public License as published by
--   the Free Software Foundation, either version 3 of the License, or
--   (at your option) any later version.
--
--   NeuroWombat is distributed in the hope that it will be useful,
--   but WITHOUT ANY WARRANTY; without even the implied warranty of
--   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
--   GNU General Public License for more details.
--
--   You should have received a copy of the GNU General Public License
--   along with NeuroWombat.  If not, see <http://www.gnu.org/licenses/>.


module( ..., package.seeall )


-- Function for neurons competition;
local function getWinner( x )
   local index = 1;
   local min = x[ 1 ];
   for i = 2, #x do
      if x[ i ] < min then
         min = x[ i ];
         index = i;
         end
      end

   return index;
   end


-- Function for calculating initial weights;
local function getInitWeight( vectors, i )
   local min = vectors[ 1 ][ i ];
   local max = min;
   for j = 2, #vectors do
      if vectors[ j ][ i ] > max then max = vectors[ j ][ i ]; end
      if vectors[ j ][ i ] < min then min = vectors[ j ][ i ]; end
      end

   return ( max + min ) / 2.0;
   end


function create( inputs, neurons )
   local network = {};
   network.inputs = inputs;
   network.neuronsCount = neurons;
   network.neurons = {};
   network.actFunc = createActFunc( ACT_FUNC.LINEAR, 1.0, 0.0 );
   network.connectorsCount = inputs + 1 + neurons;
   network.connectors = createDigitalConnectors( network.connectorsCount );
   network.memorySize = ( inputs + 1 ) * neurons;
   network.memory = createMemoryModule( network.memorySize );
   network.procUnit = createProcUnit( PROC_UNIT.RADIAL_BASIS, COEFF_USAGE.ADD_TO );

   -- Set 1.0 value for input'th connector;
   setValues( network.connectors, inputs, { 1.0 } );

   -- Create neurons layer;
   inputConnectors = {};
   for i = 1, inputs do
      inputConnectors[ i ] = i - 1;
      end

   inputConnectors[ inputs + 1 ] = inputs;

   for i = 0, neurons - 1 do
      network.neurons[ i + 1 ] = createDigitalNeuron(
         inputs + 1,
         inputConnectors,
         network.connectors, inputs + 1 + i,
         network.memory, ( inputs + 1 ) * i,
         network.procUnit,
         network.actFunc
         );
      end

   return network;
   end


function destroy( network )
   for i = 1, #network.neurons do
      closeId( network.neurons[ i ] );
      end

   closeId( network.procUnit );
   closeId( network.memory );
   closeId( network.connectors );
   closeId( network.actFunc );
   end;


function train( network, vectors, epochs, wSpeed, bSpeed )
   -- Initial setup;
   local w = {};
   w[ network.inputs + 1 ] = network.neuronsCount * math.exp( 1.0 );
   for i = 1, network.inputs do
      w[ i ] = getInitWeight( vectors, i );
      end

   for i = 1, network.neuronsCount do
      setDigitalWeights( network.neurons[ i ], w );
      end

   -- Setup activity parameters;
   local c = {};
   for i = 1, network.neuronsCount do c[ i ] = 1.0 / network.neuronsCount; end

   local x = {};
   for i = 0, epochs - 1 do
      -- Compute;
      winner = compute( network, vectors[ i % #vectors + 1 ] );

      for j = 1, network.neuronsCount do
         w = getDigitalWeights( network.neurons[ j ] );

         if j == winner then
            -- Modify weights;
            for k = 1, network.inputs do
               w[ k ] = w[ k ] + wSpeed * ( vectors[ i % #vectors + 1 ][ k ] - w[ k ] );
               end

            c[ j ] = c[ j ] + bSpeed * ( 1.0 - c[ j ] );
         else
            c[ j ] = c[ j ] * ( 1.0 - bSpeed );
         end

         w[ network.inputs + 1 ] = math.exp( 1.0 - math.log( c[ j ] ) );
         setDigitalWeights( network.neurons[ j ], w );
         end
      end
   end


function compute( network, x )
   setValues( network.connectors, 0, x );
   computeDigitalNeurons( network.neurons, 1 );
   return getWinner( getValues( network.connectors, network.inputs + 1, network.neuronsCount ) );
   end

