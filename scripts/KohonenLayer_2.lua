--   Copyright (C) 2009 Andrew Timashov
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


-- Kohonen layer script ( 2 neurons ). The goal of training is to 
-- clusterize 4 base vectors. Then the task is to classify 4 test 
-- vectors.

require "abstract.KohonenLayer";

-- Function for testing Kohonen layer;
function testNetwork()
   local clusters = { { 0.1, 0.1, 0.1 }, { 0.9, 0.9, 0.9 } };
   local r = 0.05;

   -- Determine winner for first class;
   local x1 = abstract.KohonenLayer.compute( net, clusters[ 1 ] );
   local x2 = 3 - x1;

   local hitsCount = 0;
   for i = 1, 100 do
      local realClass = math.random( 2 );
      local v = {};
      v[ 1 ] = clusters[ realClass ][ 1 ] - r + ( 2 * r * math.random() );
      v[ 2 ] = clusters[ realClass ][ 2 ] - r + ( 2 * r * math.random() );
      v[ 3 ] = clusters[ realClass ][ 3 ] - r + ( 2 * r * math.random() );
      local winner = abstract.KohonenLayer.compute( net, v );
      if ( winner == x1 ) and ( realClass == 1 ) or ( winner == x2 ) and ( realClass == 2 ) then
         hitsCount = hitsCount + 1;
         end
      end

   return ( hitsCount >= 90 );
end

-- Function for estimating time to fail destribution of Hopfield network;
function estimateTimeToFailDestribution( times, lambda )
   local d = {};
   for i = 1, times do
      abstract.KohonenLayer.train( net, trainVectors, 100, 0.5 );
      local destr = createExponentialDestribution( lambda );
      local manager = createAbstractWeightsManager( destr, net.weights );
      local engine = createSimulationEngine();
      appendInterruptManager( engine, manager );

      while stepOverEngine( engine ) do
         if not testNetwork() then break end
         end

      d[ i ] = getCurrentTime( engine );

      closeId( engine );
      closeId( manager );
      closeId( destr );
      end

   return d;
   end

-- Function for estimating time to fail of Kohonen layer;
function estimateTimeToFail( times, lambda )
   local t = 0.0;
   local tsqr = 0.0;
   for i = 1, times do
      abstract.KohonenLayer.train( net, trainVectors, 100, 0.5 );
      local destr = createExponentialDestribution( lambda );
      local manager = createAbstractWeightsManager( destr, net.weights );
      local engine = createSimulationEngine();
      appendInterruptManager( engine, manager );

      repeat
         if not testNetwork() then
            local x = getCurrentTime( engine );
            t = t + x;
            tsqr = tsqr + x * x;
            break;
            end

         until not stepOverEngine( engine )

      closeId( engine );
      closeId( manager );
      closeId( destr );
      end

   t = t / times;
   tsqr = tsqr / times;
   local d = calcMeanCI( t, tsqr, times, 0.95 );
   return t, t - d, t + d;
   end

-- Function for estimating survival function of Kohonen layer;
function estimateSurvivalFunction( times, lambda, t )
   local p = 0.0;
   for i = 1, times do
      abstract.KohonenLayer.train( net, trainVectors, 100, 0.5 );
      local destr = createExponentialDestribution( lambda );
      local manager = createAbstractWeightsManager( destr, net.weights );
      local engine = createSimulationEngine();
      appendInterruptManager( engine, manager );

      local tIsTooLarge = true;
      repeat
         if getCurrentTime( engine ) >= t then
            if testNetwork() then p = p + 1.0 end
            tIsTooLarge = false;
            break;
            end

         until not stepOverEngine( engine )

      if tIsTooLarge then
         if testNetwork() then p = p + 1.0 end
         end;

      closeId( engine );
      closeId( manager );
      closeId( destr );
      end

   p = p / times;
   return p, calcACProbabilityCI( p, times, 0.05 );
   end

-- Function for estimating faults count destribution of Kohonen layer;
function estimateFaultsCountDestribution( times, lambda )
   local d = {};
   for i = 0, ( inputs + 1 ) * neurons do d[ i ] = 0 end
   for i = 1, times do
      abstract.KohonenLayer.train( net, trainVectors, 100, 0.5 );
      local destr = createExponentialDestribution( lambda );
      local manager = createAbstractWeightsManager( destr, net.weights );
      local engine = createSimulationEngine();
      appendInterruptManager( engine, manager );

      failesCount = 0;
      while stepOverEngine( engine ) do
         failesCount = failesCount + 1;
         if not testNetwork() then break end

         end

      d[ failesCount ] = d[ failesCount ] + 1;

      closeId( engine );
      closeId( manager );
      closeId( destr );
      end

   for i = 0, ( inputs + 1 ) * neurons do d[ i ] = d[ i ] / times end
   return d;
   end

-- Function for estimating weight importance of Kohonen layer;
function estimateWeightImportance( times, lambda, weightIndex, t )
   local p = 0.0;
   local m = 0; local s = -1;
   for i = 1, times do
      abstract.KohonenLayer.train( net, trainVectors, 100, 0.5 );
      local destr = createExponentialDestribution( lambda );
      local manager = createAbstractWeightsManager( destr, net.weights );
      local engine = createSimulationEngine();
      appendInterruptManager( engine, manager );

      if t >= getFutureTime( engine ) then
         while stepOverEngine( engine ) do
            if t < getFutureTime( engine ) then break end
            end         
         end

      if testNetwork() then
         setAbstractWeights( weights, weightIndex, { 0.0 } );
         if not testNetwork() then p = p + 1.0 end
         end

      closeId( engine );
      closeId( manager );
      closeId( destr );
      end

   p = p / times;
   return p, calcACProbabilityCI( p, times, 0.05 );
   end

print( "API version: " .. apiVersion() );

-- Create neurons layer;
io.write( "Assembling Kohonen layer ( 2 neurons ) ... " ); io.flush();
inputs = 3;
neurons = 2;
net = abstract.KohonenLayer.create( inputs, neurons );
print( "[OK]" );

-- Train Kohonen layer;
io.write( "Training ... " ); io.flush();
trainVectors = {
   { 0.1, 0.2, 0.15 }, { 0.8, 0.9, 0.85 },
   { 0.1, 0.1, 0.1 }, { 0.9, 0.8, 0.79 },
   { 0.19, 0.12, 0.09 }, { 0.87, 0.9, 0.75 },
   { 0.91, 0.91, 0.92 }, { 0.1, 0.07, 0.14 }
   };

abstract.KohonenLayer.train( net, trainVectors, 100, 0.5 );
print( "[OK]" );

-- Print weights;
for i = 1, neurons do
   w = getAbstractWeights( net.neurons[ i ] );
   for j = 1, inputs + 1 do
      print( "w[ " .. i .. " ][ " .. j .. " ] = " .. w[ j ] );
      end
   end

-- Test results;
vectors = { { 0.1, 0.1, 0.1 }, { 0.8, 0.8, 0.8 }, { 0.0, 0.2, 0.1 }, { 0.8, 0.9, 0.75 } };

for i = 1, #vectors do
   winner = abstract.KohonenLayer.compute( net, vectors[ i ] );
   print( "{ " .. vectors[ i ][ 1 ] .. ", " .. vectors[ i ][ 2 ] .. ", " .. vectors[ i ][ 3 ] .. " } => " .. winner );
   end

-- Simulate Hopfield network;
io.write( "Simulating ... " ); io.flush();
-- timeToFail, dtl, dth = estimateTimeToFail( 200, 0.00001 );
-- p, dpl, dph = estimateSurvivalFunction( 200, 0.00001, 20000 );
-- print( "[OK]" );
-- print( "timeToFail = " .. timeToFail .. "; CI = [" .. dtl .. ", " .. dth .. "]" );
-- print( "p = " .. p .. "; CI = [" .. dpl .. ", " .. dph .. "]" );
print( "[OK]" );

length = 10; x = {};
-- start = 0.00001; stop = 0.0001; delta = ( stop - start ) / ( length - 1 );
start = 0.0; stop = 20000.0; delta = ( stop - start ) / ( length - 1 );
-- start = 0.0; stop = 30000.0; delta = ( stop - start ) / ( length - 1 );
for i = 0, length - 1 do
   x[ i ] = start + i * delta;
   -- y, dyl, dyh = estimateTimeToFail( 500, x[ i ] );
   y, dyl, dyh = estimateSurvivalFunction( 2000, 0.0001, x[ i ] );
   -- y, dyl, dyh = estimateWeightImportance( 2000, 0.0001, 0, x[ i ] );
   -- y1, dyl1, dyh1 = estimateWeightImportance( 2000, 0.0001, 4, x[ i ] );
   -- print( x[ i ] * 10000 .. " " .. dyl / 1000 .. " " .. y / 1000 .. " " .. dyh / 1000 );
   print( x[ i ] / 1000 .. " " .. dyl .. " " .. y .. " " .. dyh );
   -- print( x[ i ] / 1000 .. " " .. dyl .. " " .. y .. " " .. dyh .. " " .. dyl1 .. " " .. y1 .. " " .. dyh1 );
   end;

-- destr = estimateTimeToFailDestribution( 1000, 0.0001 );
-- for i = 1, 1000 do
--    io.write( destr[ i ] .. ", " );
--    end

-- Close network;
abstract.KohonenLayer.destroy( net );

