/***************************************************************************
 *   Copyright (C) 2009 Andrew Timashov                                    *
 *                                                                         *
 *   This file is part of NeuroWombat.                                     *
 *                                                                         *
 *   NeuroWombat is free software: you can redistribute it and/or modify   *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation, either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   NeuroWombat is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with NeuroWombat.  If not, see <http://www.gnu.org/licenses/>.  *
 ***************************************************************************/


#include "api/api.h"


#include <math.h>
#include <stdio.h>
#include <typeinfo>
#include <vector>


#include "kernel/Kernel.h"
#include "components/abstract/AbstractActivators.h"
#include "components/abstract/AbstractBuffers.h"
#include "components/abstract/AbstractConnectors.h"
#include "components/abstract/AbstractProcessor.h"
#include "components/abstract/AbstractWeights.h"
#include "neurons/abstract/AbstractNeuron.h"
#include "components/analog/AnalogComparators.h"
#include "components/analog/AnalogResistors.h"
#include "components/analog/AnalogWires.h"
#include "neurons/analog/AnalogNeuron.h"
#include "engine/SimulationEngine.h"
#include "math/Destribution.h"
#include "math/ActivationFunction.h"



// It is better for API functions to use this pointer instead of
// Kernel::instance() and Kernel::freeInstance() methods;
extern Kernel * kernel;


#define _readIntegersVector( L, index, integers )\
   lua_pushnil( L );\
   while ( lua_next( L, index ) != 0 )\
      {\
      integers.push_back( lua_tointeger( L, -1 ) );\
      lua_pop( L, 1 );\
      }\


#define _readKernelObjectsVector( L, index, T, objects )\
   lua_pushnil( L );\
   while ( lua_next( L, index ) != 0 )\
      {\
      T object = dynamic_cast < T >( kernel->getObject( lua_tointeger( L, -1 ) ) );\
      if ( object != NULL ) objects.push_back( object );\
      lua_pop( L, 1 );\
      }\


void registerApiFunctions( lua_State * L )
   {
   // Register common API functions;
   lua_register( L, "apiVersion", apiVersion );
   lua_register( L, "closeId", closeId );
   lua_register( L, "readArray", readArray );
   // Register abstract neuron API functions;
   lua_register( L, "createAbstractActivators", createAbstractActivators );
   lua_register( L, "createAbstractBuffers", createAbstractBuffers );
   lua_register( L, "createAbstractConnectors", createAbstractConnectors );
   lua_register( L, "getSignals", getSignals );
   lua_register( L, "setSignals", setSignals );
   lua_register( L, "createAbstractCustomProcessor", createAbstractCustomProcessor );
   lua_register( L, "createAbstractRadialBasisProcessor", createAbstractRadialBasisProcessor );
   lua_register( L, "createAbstractScalarProcessor", createAbstractScalarProcessor );
   lua_register( L, "createAbstractWeightedSumProcessor", createAbstractWeightedSumProcessor );
   lua_register( L, "createAbstractWeights", createAbstractWeights );
   lua_register( L, "getAbstractWeights", getAbstractWeights );
   lua_register( L, "setAbstractWeights", setAbstractWeights );
   lua_register( L, "createAbstractNeuron", createAbstractNeuron );
   lua_register( L, "computeAbstractNeurons", computeAbstractNeurons );
   lua_register( L, "trainBPAbstractNeurons", trainBPAbstractNeurons );
   // Register analog neuron API functions;
   lua_register( L, "createAnalogComparators", createAnalogComparators );
   lua_register( L, "createAnalogResistors", createAnalogResistors );
   lua_register( L, "setupAnalogResistors", setupAnalogResistors );
   lua_register( L, "getAnalogResistances", getAnalogResistances );
   lua_register( L, "setAnalogResistances", setAnalogResistances );
   lua_register( L, "createAnalogWires", createAnalogWires );
   lua_register( L, "getPotentials", getPotentials );
   lua_register( L, "setPotentials", setPotentials );
   lua_register( L, "createAnalogNeuron", createAnalogNeuron );
   lua_register( L, "computeAnalogNeurons", computeAnalogNeurons );
   // Math API functions;
   lua_register( L, "createCustomActFunc", createCustomActFunc );
   lua_register( L, "createGaussianActFunc", createGaussianActFunc );
   lua_register( L, "createLimActFunc", createLimActFunc );
   lua_register( L, "createLinearActFunc", createLinearActFunc );
   lua_register( L, "createLimLinearActFunc", createLimLinearActFunc );
   lua_register( L, "createPosLinearActFunc", createPosLinearActFunc );
   lua_register( L, "createSigmoidActFunc", createSigmoidActFunc );
   lua_register( L, "createThSigmoidActFunc", createThSigmoidActFunc );
   lua_register( L, "calcMeanCI", calcMeanCI );
   lua_register( L, "calcACProbabilityCI", calcACProbabilityCI );
   // Simulation engine API functions;
   lua_register( L, "createExponentialDestribution", createExponentialDestribution );
   lua_register( L, "createWeibullDestribution", createWeibullDestribution );
   lua_register( L, "createAbstractWeightsManager", createAbstractWeightsManager );
   lua_register( L, "createAnalogResistorsManager", createAnalogResistorsManager );
   lua_register( L, "createSimulationEngine", createSimulationEngine );
   lua_register( L, "appendInterruptManager", appendInterruptManager );
   lua_register( L, "stepOverEngine", stepOverEngine );
   lua_register( L, "getCurrentTime", getCurrentTime );
   lua_register( L, "getFutureTime", getFutureTime );
   lua_register( L, "getCurrentSource", getCurrentSource );
   lua_register( L, "getFutureSource", getFutureSource );
   };


unsigned int readArray( lua_State * L, int index, unsigned int length, unsigned int * array )
   {
   unsigned int i = 0;

   // Push first key;
   lua_pushnil( L );
   while ( lua_next( L, index ) != 0 && i < length )
      {
      // Read key and decrease it by 1 to provide compatibility
      // between C and Lua-style arrays;
      unsigned int key = lua_tointeger( L, -2 ) - 1;
      if ( key < length ) array[ key ] = lua_tointeger( L, -1 );

      // Remove 'value' but keep 'key' for next iteration;
      lua_pop( L, 1 );

      i ++;
      }

   return i;
   };


unsigned int readArray( lua_State * L, int index, unsigned int length, double * array )
   {
   unsigned int i = 0;

   // Push first key;
   lua_pushnil( L );
   while ( lua_next( L, index ) != 0 && i < length )
      {
      // Read key and decrease it by 1 to provide compatibility
      // between C and Lua-style arrays;
      unsigned int key = lua_tointeger( L, -2 ) - 1;
      if ( key < length ) array[ key ] = lua_tonumber( L, -1 );

      // Remove 'value' but keep 'key' for next iteration;
      lua_pop( L, 1 );

      i ++;
      }

   return i;
   };


/***************************************************************************
 *   Commonn API functions implementation                                  *
 ***************************************************************************/


int apiVersion( lua_State * L )
   {
   lua_pushstring( L, API_VERSION );
   return 1;
   };


int closeId( lua_State * L )
   {
   kernel->deleteObject( luaL_checknumber( L, 1 ) );
   return 0;
   };


int readArray( lua_State * L )
   {
   printf( "len = %i\n", lua_gettop( L ) );
   lua_pushnil( L );  /* first key */
   while ( lua_next( L, 1 ) != 0 )
      {
      /* uses 'key' ( at index -2 ) and 'value' ( at index -1 ) */
      printf( "arr[ %i ] = %i\n",
         ( int ) lua_tonumber( L, -2 ),
         ( int ) lua_tonumber( L, -1 )
         );

      /* removes 'value'; keeps 'key' for next iteration */
      lua_pop( L, 1 );
      }

   //lua_pop( L, 2 );

   //printf( "len = %i\n", lua_getn( L, -1 ) );
   return 0;
   };


/***************************************************************************
 *   Abstract neuron API functions implementation                          *
 ***************************************************************************/


int createAbstractActivators( lua_State * L )
   {
   KernelObjectId id = 0;

   unsigned int count = luaL_checkinteger( L, 1 );

   // Read activationFunction argument;
   id = luaL_checkinteger( L, 2 );
   KernelObject * object = kernel->getObject( id );
   ActivationFunction * activationFunction = dynamic_cast < ActivationFunction * >( object );

   if ( count > 0 && activationFunction != NULL )
      {
      AbstractActivators * activators = new AbstractActivators( count, activationFunction );
      id = kernel->insertObject( activators );
      }

   lua_pushnumber( L, id );
   return 1;
   };


int createAbstractBuffers( lua_State * L )
   {
   KernelObjectId id = 0;

   unsigned int count = luaL_checkinteger( L, 1 );
   if ( count > 0 )
      {
      AbstractBuffers * buffers = new AbstractBuffers( count );
      id = kernel->insertObject( buffers );
      }

   lua_pushnumber( L, id );
   return 1;
   };


int createAbstractConnectors( lua_State * L )
   {
   KernelObjectId id = 0;

   unsigned int count = luaL_checkinteger( L, 1 );
   if ( count > 0 )
      {
      AbstractConnectors * connectors = new AbstractConnectors( count );
      id = kernel->insertObject( connectors );
      }

   lua_pushnumber( L, id );
   return 1;
   };


int getSignals( lua_State * L )
   {
   // Read connectors argument;
   KernelObjectId connectorsId = luaL_checkinteger( L, 1 );
   KernelObject * object = kernel->getObject( connectorsId );
   AbstractConnectors * connectors = dynamic_cast < AbstractConnectors * >( object );

   // Read baseIndex argument;
   unsigned int baseIndex = luaL_checkinteger( L, 2 );

   // Read count argument;
   unsigned int count = luaL_checkinteger( L, 3 );

   // Create table;
   lua_newtable( L );
   for ( unsigned int i = 0; i < count; i ++ )
      {
      // Increase key by 1 to provide compatibility between C and Lua-style arrays;
      lua_pushnumber( L, i + 1 );
      lua_pushnumber( L, connectors->getSignal( baseIndex + i ) );
      lua_rawset( L, -3 );
      }

   return 1;
   };


int setSignals( lua_State * L )
   {
   // Read connectors argument;
   KernelObjectId connectorsId = luaL_checkinteger( L, 1 );
   KernelObject * object = kernel->getObject( connectorsId );
   AbstractConnectors * connectors = dynamic_cast < AbstractConnectors * >( object );

   // Read baseIndex argument;
   unsigned int baseIndex = luaL_checkinteger( L, 2 );

   // Read signals argument;
   unsigned int limit = connectors->count();
   lua_pushnil( L );
   while ( lua_next( L, 3 ) != 0 )
      {
      // Read key and decrease it by 1 to provide compatibility
      // between C and Lua-style arrays;
      unsigned int index = baseIndex + lua_tointeger( L, -2 ) - 1;
      if ( index < limit ) connectors->setSignal( index, lua_tonumber( L, -1 ) );
      lua_pop( L, 1 );
      }

   return 0;
   };


int createAbstractCustomProcessor( lua_State * L )
   {
   KernelObjectId id = 0;

   // Read luaFunction argument;
   lua_pushvalue( L, 1 );
   int processRef = luaL_ref( L, LUA_REGISTRYINDEX );

   // Read useMultiplier argument;
   bool useMultiplier = luaL_checknumber( L, 2 ) != 0;

   AbstractCustomProcessor * processor = new AbstractCustomProcessor( L, processRef, useMultiplier );
   id = kernel->insertObject( processor );

   lua_pushnumber( L, id );
   return 1;
   };


int createAbstractRadialBasisProcessor( lua_State * L )
   {
   KernelObjectId id = 0;

   // Read useMultiplier argument;
   bool useMultiplier = luaL_checknumber( L, 1 ) != 0;

   AbstractRadialBasisProcessor * processor = new AbstractRadialBasisProcessor( useMultiplier );
   id = kernel->insertObject( processor );

   lua_pushnumber( L, id );
   return 1;
   };


int createAbstractScalarProcessor( lua_State * L )
   {
   KernelObjectId id = 0;

   AbstractScalarProcessor * processor = new AbstractScalarProcessor();
   id = kernel->insertObject( processor );

   lua_pushnumber( L, id );
   return 1;
   };


int createAbstractWeightedSumProcessor( lua_State * L )
   {
   KernelObjectId id = 0;

   AbstractWeightedSumProcessor * processor = new AbstractWeightedSumProcessor();
   id = kernel->insertObject( processor );

   lua_pushnumber( L, id );
   return 1;
   };


int createAbstractWeights( lua_State * L )
   {
   KernelObjectId id = 0;

   unsigned int count = luaL_checkinteger( L, 1 );
   if ( count > 0 )
      {
      AbstractWeights * weights = new AbstractWeights( count );
      id = kernel->insertObject( weights );
      }

   lua_pushnumber( L, id );
   return 1;
   };


// ERROR WITH HANDLING RADIAL-BASIS MULTIPLYER;
int getAbstractWeights( lua_State * L )
   {
   // Read neuron argument;
   KernelObjectId neuronId = luaL_checkinteger( L, 1 );
   KernelObject * object = kernel->getObject( neuronId );
   AbstractNeuron * neuron = dynamic_cast < AbstractNeuron * >( object );

   if ( neuron != NULL )
      {
      // Create table;
      lua_newtable( L );
      for ( unsigned int i = 0; i < neuron->getInputsCount(); i ++ )
         {
         // Increase key by 1 to provide compatibility between C and Lua-style arrays;
         lua_pushnumber( L, i + 1 );
         lua_pushnumber( L, neuron->getWeight( i ) );
         lua_rawset( L, -3 );
         }
      }
   else
      {
      AbstractWeights * weights = dynamic_cast < AbstractWeights * >( object );

      // Read baseIndex argument;
      unsigned int baseIndex = luaL_checkinteger( L, 2 );

      // Read count argument;
      unsigned int count = luaL_checkinteger( L, 3 );

      // Create table;
      lua_newtable( L );
      for ( unsigned int i = 0; i < count; i ++ )
         {
         // Increase key by 1 to provide compatibility between C and Lua-style arrays;
         lua_pushnumber( L, i + 1 );
         lua_pushnumber( L, weights->getWeight( baseIndex + i ) );
         lua_rawset( L, -3 );
         }
      }

   return 1;
   };


int setAbstractWeights( lua_State * L )
   {
   // Read neuron argument;
   KernelObjectId neuronId = luaL_checkinteger( L, 1 );
   KernelObject * object = kernel->getObject( neuronId );
   AbstractNeuron * neuron = dynamic_cast < AbstractNeuron * >( object );

   if ( neuron != NULL )
      {
      // Read weights argument;
      unsigned int limit = neuron->getInputsCount();
      lua_pushnil( L );
      while ( lua_next( L, 2 ) != 0 )
         {
         // Read key and decrease it by 1 to provide compatibility
         // between C and Lua-style arrays;
         unsigned int index = lua_tointeger( L, -2 ) - 1;
         if ( index < limit ) neuron->setWeight( index, lua_tonumber( L, -1 ) );
         lua_pop( L, 1 );
         }
      }
   else
      {
      AbstractWeights * weights = dynamic_cast < AbstractWeights * >( object );

      // Read baseIndex argument;
      unsigned int baseIndex = luaL_checkinteger( L, 2 );

      // Read weights argument;
      unsigned int limit = weights->count();
      lua_pushnil( L );
      while ( lua_next( L, 3 ) != 0 )
         {
         // Read key and decrease it by 1 to provide compatibility
         // between C and Lua-style arrays;
         unsigned int index = baseIndex + lua_tointeger( L, -2 ) - 1;
         if ( index < limit ) weights->setWeight( index, lua_tonumber( L, -1 ) );
         lua_pop( L, 1 );
         }
      }

   return 0;
   };


int createAbstractNeuron( lua_State * L )
   {
   KernelObject * object = NULL;

   // Read inputsCount argument;
   unsigned int inputsCount = luaL_checkinteger( L, 1 );

   // Read inputConnectors argument;
   unsigned int * inputConnectors = NULL;
   if ( inputsCount > 0 )
      {
      inputConnectors = new unsigned int[ inputsCount ];
      readArray( L, 2, inputsCount, inputConnectors );
      }

   // Read connectors argument;
   KernelObjectId connectorsId = luaL_checkinteger( L, 3 );
   object = kernel->getObject( connectorsId );
   AbstractConnectors * connectors = dynamic_cast < AbstractConnectors * >( object );

   // Read connectorsBaseIndex argument;
   unsigned int connectorsBaseIndex = luaL_checkinteger( L, 4 );

   // Read weights argument;
   KernelObjectId weightsId = luaL_checkinteger( L, 5 );
   object = kernel->getObject( weightsId );
   AbstractWeights * weights = dynamic_cast < AbstractWeights * >( object );

   // Read weightsBaseIndex argument;
   unsigned int weightsBaseIndex = luaL_checkinteger( L, 6 );

   // Read buffers argument;
   KernelObjectId buffersId = luaL_checkinteger( L, 7 );
   object = kernel->getObject( buffersId );
   AbstractBuffers * buffers = dynamic_cast < AbstractBuffers * >( object );

   // Read buffersBaseIndex argument;
   unsigned int buffersBaseIndex = luaL_checkinteger( L, 8 );

   // Read processor argument;
   KernelObjectId processorId = luaL_checkinteger( L, 9 );
   object = kernel->getObject( processorId );
   AbstractProcessor * processor = dynamic_cast < AbstractProcessor * >( object );

   // Read activationFunction argument;
   KernelObjectId activationFunctionId = luaL_checkinteger( L, 10 );
   object = kernel->getObject( activationFunctionId );
   ActivationFunction * activationFunction = dynamic_cast < ActivationFunction * >( object );

   // Create abstract neuron;
   AbstractNeuron * neuron = NULL;
   if ( activationFunction != NULL )
      {
      neuron = new AbstractNeuron(
         inputsCount, inputConnectors,
         connectors, connectorsBaseIndex,
         weights, weightsBaseIndex,
         buffers, buffersBaseIndex,
         processor,
         activationFunction
         );
      }
   else
      {
      AbstractActivators * activators = dynamic_cast < AbstractActivators * >( object );

      // Read activatorsBaseIndex argument;
      unsigned int activatorsBaseIndex = luaL_checkinteger( L, 11 );

      neuron = new AbstractNeuron(
         inputsCount, inputConnectors,
         connectors, connectorsBaseIndex,
         weights, weightsBaseIndex,
         buffers, buffersBaseIndex,
         processor,
         activators, activatorsBaseIndex
         );
      }

   KernelObjectId id = kernel->insertObject( neuron );

   // Free inputConnectors;
   if ( inputConnectors != NULL ) delete[] inputConnectors;

   lua_pushnumber( L, id );
   return 1;
   };


int computeAbstractNeurons( lua_State * L )
   {
   // Create vector for holding AbstractNeuron pointers;
   std::vector < AbstractNeuron * > neurons;

   // Read neurons argument;
   _readKernelObjectsVector( L, 1, AbstractNeuron *, neurons );

   // Read times argument;
   unsigned int times = luaL_checkinteger( L, 2 );

   // Calculate neurons;
   for ( unsigned int i = 0; i < times; i ++ )
      {
      for ( unsigned int j = 0; j < neurons.size(); j ++ )
         {
         if ( neurons[ j ] != NULL ) neurons[ j ]->compute();
         }
      }

   return 0;
   };


int trainBPAbstractNeurons( lua_State * L )
   {
   // Create vector for holding AbstractNeuron pointers;
   std::vector < AbstractNeuron * > neurons;

   // Read neurons argument;
   _readKernelObjectsVector( L, 1, AbstractNeuron *, neurons );

   // Create vector for holding neurons per layer count;
   std::vector < unsigned int > layers;

   // Read layers argument;
   _readIntegersVector( L, 2, layers );

   // Create array for holding target;
   unsigned int targetLength = layers[ layers.size() - 1 ];
   double * target = new double[ targetLength ];

   // Read target argument;
   readArray( L, 3, targetLength, target );

   // Read damping argument;
   double damping = luaL_checknumber( L, 4 );

   // Read speed argument;
   double speed = luaL_checknumber( L, 5 );

   // Calculate neurons;
   unsigned int neuronsCount = neurons.size();
   for ( unsigned int i = 0; i < neuronsCount; i ++ )
      {
      neurons[ i ]->compute();
      }

   // Snap deltas for output layer;
   for ( unsigned int i = 0; i < targetLength; i ++ )
      {
      neurons[ neuronsCount - 1 - i ]->snapDelta(
         target[ i ] - neurons[ neuronsCount - 1 - i ]->getOutput()
         );
      }

   // Free target;
   delete[] target;

   // Snap deltas for the rest layers;
   unsigned int index = neuronsCount - targetLength - 1;
   unsigned int prevIndex = neuronsCount - 1;
   for ( int i = layers.size() - 2; i >= 0 ; i -- )
      {
      for ( int j = layers[ i ] - 1; j >= 0; j -- )
         {
         // Calculate error;
         double err = 0.0;
         for ( int k = layers[ i + 1 ] - 1; k >= 0; k -- )
            {
            err += neurons[ prevIndex - k ]->getWeightedDelta( j );
            }

         neurons[ index ]->snapDelta( err );
         index --;
         }

      prevIndex -= layers[ i + 1 ];
      }

   // Modify weights for all the neurons;
   for ( unsigned int i = 0; i < neuronsCount; i ++ )
      {
      neurons[ i ]->createDampingBuffers();
      neurons[ i ]->modifyWeights( damping, speed );
      }

   lua_pushnumber( L, 0 );
   return 1;
   };


/***************************************************************************
 *   Analog neuron API functions implementation                            *
 ***************************************************************************/


int createAnalogComparators( lua_State * L )
   {
   KernelObjectId id = 0;

   unsigned int count = luaL_checkinteger( L, 1 );
   if ( count > 0 )
      {
      AnalogComparators * comparators = new AnalogComparators( count );
      id = kernel->insertObject( comparators );
      }

   lua_pushnumber( L, id );
   return 1;
   };


int createAnalogResistors( lua_State * L )
   {
   KernelObjectId id = 0;

   unsigned int count = luaL_checkinteger( L, 1 );
   if ( count > 0 )
      {
      AnalogResistors * resistors = new AnalogResistors( count );
      id = kernel->insertObject( resistors );
      }

   lua_pushnumber( L, id );
   return 1;
   };


// Count argument must be deleted;
int setupAnalogResistors( lua_State * L )
   {
   // Read resistors argument;
   KernelObjectId resistorsId = luaL_checkinteger( L, 1 );
   KernelObject * object = kernel->getObject( resistorsId );
   AnalogResistors * resistors = dynamic_cast < AnalogResistors * >( object );

   // Read baseIndex argument;
   unsigned int baseIndex = luaL_checkinteger( L, 2 );

   // Read count argument;
   unsigned int count = luaL_checkinteger( L, 3 );

   // Read weights argument;
   double * weights = NULL;
   if ( count > 0 )
      {
      weights = new double[ count ];
      readArray( L, 4, count, weights );
      }

   // Read numCopies argument;
   unsigned int numCopies = luaL_checkinteger( L, 5 );

   // Calculate weights product;
   double wProduct = 1.0;
   unsigned int numNonZeroW = 0;
   for ( unsigned int j = 0; j < count; j ++ )
      {
      if ( weights[ j ] != 0.0 )
         {
         numNonZeroW ++;
         wProduct *= weights[ j ];
         }
      }

   // Calculate resistances;
   for ( unsigned int j = 0; j < count; j ++ )
      {
      double resistance = 0.0;
      if ( weights[ j ] != 0.0 )
         {
         if ( numNonZeroW > 1 )
            {
            resistance = pow( fabs( wProduct ), 1.0 / ( double ) ( numNonZeroW - 1 ) ) / weights[ j ];
            }
         else
            {
            resistance = weights[ j ];
            }
         }

      // Set resistances;
      for ( unsigned int k = 0; k < numCopies; k ++ )
         {
         resistors->setResistance( baseIndex + count * k + j, resistance );
         }
      }

   if ( weights != NULL ) delete[] weights;

   return 0;
   };


int getAnalogResistances( lua_State * L )
   {
   // Read resistors argument;
   KernelObjectId resistorsId = luaL_checkinteger( L, 1 );
   KernelObject * object = kernel->getObject( resistorsId );
   AnalogResistors * resistors = dynamic_cast < AnalogResistors * >( object );

   // Read baseIndex argument;
   unsigned int baseIndex = luaL_checkinteger( L, 2 );

   // Read count argument;
   unsigned int count = luaL_checkinteger( L, 3 );

   // Create table;
   lua_newtable( L );
   for ( unsigned int i = 0; i < count; i ++ )
      {
      // Increase key by 1 to provide compatibility between C and Lua-style arrays;
      lua_pushnumber( L, i + 1 );
      lua_pushnumber( L, resistors->getResistance( baseIndex + i ) );
      lua_rawset( L, -3 );
      }

   return 1;
   };


int setAnalogResistances( lua_State * L )
   {
   // Read resistors argument;
   KernelObjectId resistorsId = luaL_checkinteger( L, 1 );
   KernelObject * object = kernel->getObject( resistorsId );
   AnalogResistors * resistors = dynamic_cast < AnalogResistors * >( object );

   // Read baseIndex argument;
   unsigned int baseIndex = luaL_checkinteger( L, 2 );

   // Read weights argument;
   unsigned int limit = resistors->count();
   lua_pushnil( L );
   while ( lua_next( L, 3 ) != 0 )
      {
      // Read key and decrease it by 1 to provide compatibility
      // between C and Lua-style arrays;
      unsigned int index = baseIndex + lua_tointeger( L, -2 ) - 1;
      if ( index < limit ) resistors->setResistance( index, lua_tonumber( L, -1 ) );
      lua_pop( L, 1 );
      }

   return 0;
   };


int createAnalogWires( lua_State * L )
   {
   KernelObjectId id = 0;

   unsigned int count = luaL_checkinteger( L, 1 );
   if ( count > 0 )
      {
      AnalogWires * wires = new AnalogWires( count );
      id = kernel->insertObject( wires );
      }

   lua_pushnumber( L, id );
   return 1;
   };


int getPotentials( lua_State * L )
   {
   // Read wires argument;
   KernelObjectId wiresId = luaL_checkinteger( L, 1 );
   KernelObject * object = kernel->getObject( wiresId );
   AnalogWires * wires = dynamic_cast < AnalogWires * >( object );

   // Read baseIndex argument;
   unsigned int baseIndex = luaL_checkinteger( L, 2 );

   // Read count argument;
   unsigned int count = luaL_checkinteger( L, 3 );

   // Create table;
   lua_newtable( L );
   for ( unsigned int i = 0; i < count; i ++ )
      {
      // Increase key by 1 to provide compatibility between C and Lua-style arrays;
      lua_pushnumber( L, i + 1 );
      lua_pushnumber( L, wires->getPotential( baseIndex + i ) );
      lua_rawset( L, -3 );
      }

   return 1;
   };


int setPotentials( lua_State * L )
   {
   // Read wires argument;
   KernelObjectId wiresId = luaL_checkinteger( L, 1 );
   KernelObject * object = kernel->getObject( wiresId );
   AnalogWires * wires = dynamic_cast < AnalogWires * >( object );

   // Read baseIndex argument;
   unsigned int baseIndex = luaL_checkinteger( L, 2 );

   // Read potentials argument;
   unsigned int limit = wires->count();
   lua_pushnil( L );
   while ( lua_next( L, 3 ) != 0 )
      {
      // Read key and decrease it by 1 to provide compatibility
      // between C and Lua-style arrays;
      unsigned int index = baseIndex + lua_tointeger( L, -2 ) - 1;
      if ( index < limit ) wires->setPotential( index, lua_tonumber( L, -1 ) );
      lua_pop( L, 1 );
      }

   return 0;
   };


int createAnalogNeuron( lua_State * L )
   {
   KernelObjectId id = 0;
   KernelObject * object = NULL;

   // Read inputsCount argument;
   unsigned int inputsCount = luaL_checkinteger( L, 1 );

   // Read inputWires argument;
   unsigned int * inputWires = NULL;
   if ( inputsCount > 0 )
      {
      inputWires = new unsigned int[ inputsCount ];
      readArray( L, 2, inputsCount, inputWires );
      }

   // Read gndWireIndex argument;
   unsigned int gndWireIndex = luaL_checkinteger( L, 3 );

   // Read srcWireIndex argument;
   unsigned int srcWireIndex = luaL_checkinteger( L, 4 );

   // Read comparators argument;
   KernelObjectId comparatorsId = luaL_checkinteger( L, 5 );
   object = kernel->getObject( comparatorsId );
   AnalogComparators * comparators = dynamic_cast < AnalogComparators * >( object );

   // Read comparatorsBaseIndex argument;
   unsigned int comparatorsBaseIndex = luaL_checkinteger( L, 6 );

   // Read resistors argument;
   KernelObjectId resistorsId = luaL_checkinteger( L, 7 );
   object = kernel->getObject( resistorsId );
   AnalogResistors * resistors = dynamic_cast < AnalogResistors * >( object );

   // Read resistorsBaseIndex argument;
   unsigned int resistorsBaseIndex = luaL_checkinteger( L, 8 );

   // Read wires argument;
   KernelObjectId wiresId = luaL_checkinteger( L, 9 );
   object = kernel->getObject( wiresId );
   AnalogWires * wires = dynamic_cast < AnalogWires * >( object );

   // Read wiresBaseIndex argument;
   unsigned int wiresBaseIndex = luaL_checkinteger( L, 10 );

   // Create AnalogNeuron;
   AnalogNeuron * neuron = new AnalogNeuron(
      inputsCount,
      inputWires,
      gndWireIndex,
      srcWireIndex,
      comparators,
      comparatorsBaseIndex,
      resistors,
      resistorsBaseIndex,
      wires,
      wiresBaseIndex
      );

   id = kernel->insertObject( neuron );

   // Free inputWires;
   if ( inputWires != NULL ) delete[] inputWires;

   lua_pushnumber( L, id );
   return 1;
   };


int computeAnalogNeurons( lua_State * L )
   {
   // Create vector for holding AnalogNeuron pointers;
   std::vector < AnalogNeuron * > neurons;

   // Read neurons argument;
   _readKernelObjectsVector( L, 1, AnalogNeuron *, neurons );

   // Read times argument;
   unsigned int times = luaL_checkinteger( L, 2 );

   // Calculate neurons;
   for ( unsigned int i = 0; i < times; i ++ )
      {
      for ( unsigned int j = 0; j < neurons.size(); j ++ )
         {
         if ( neurons[ j ] != NULL ) neurons[ j ]->compute();
         }
      }

   return 0;
   };


/***************************************************************************
 *   Math API functions implementation                                     *
 ***************************************************************************/


int createCustomActFunc( lua_State * L )
   {
   // Read luaFunction argument;
   lua_pushvalue( L, 1 );
   int functionRef = luaL_ref( L, LUA_REGISTRYINDEX );

   // Read luaDerivative argument;
   lua_pushvalue( L, 2 );
   int derivativeRef = luaL_ref( L, LUA_REGISTRYINDEX );

   CustomActivationFunction * actFunc = new CustomActivationFunction( L, functionRef, derivativeRef );
   KernelObjectId id = kernel->insertObject( actFunc );

   lua_pushnumber( L, id );
   return 1;
   };


int createGaussianActFunc( lua_State * L )
   {
   // Read beta argument;
   double beta = luaL_checknumber( L, 1 );

   GaussianActivationFunction * actFunc = new GaussianActivationFunction( beta );
   KernelObjectId id = kernel->insertObject( actFunc );

   lua_pushnumber( L, id );
   return 1;
   };


int createLimActFunc( lua_State * L )
   {
   // Read xLim argument;
   double xLim = luaL_checknumber( L, 1 );

   // Read yLow argument;
   double yLow = luaL_checknumber( L, 2 );

   // Read yHigh argument;
   double yHigh = luaL_checknumber( L, 3 );

   LimActivationFunction * actFunc = new LimActivationFunction( xLim, yLow, yHigh );
   KernelObjectId id = kernel->insertObject( actFunc );

   lua_pushnumber( L, id );
   return 1;
   };


int createLinearActFunc( lua_State * L )
   {
   // Read a argument;
   double a = luaL_checknumber( L, 1 );

   // Read b argument;
   double b = luaL_checknumber( L, 2 );

   LinearActivationFunction * actFunc = new LinearActivationFunction( a, b );
   KernelObjectId id = kernel->insertObject( actFunc );

   lua_pushnumber( L, id );
   return 1;
   };


int createLimLinearActFunc( lua_State * L )
   {
   // Read a argument;
   double a = luaL_checknumber( L, 1 );

   // Read b argument;
   double b = luaL_checknumber( L, 2 );

   // Read xMin argument;
   double xMin = luaL_checknumber( L, 3 );

   // Read xMax argument;
   double xMax = luaL_checknumber( L, 4 );

   LimLinearActivationFunction * actFunc = new LimLinearActivationFunction( a, b, xMin, xMax );
   KernelObjectId id = kernel->insertObject( actFunc );

   lua_pushnumber( L, id );
   return 1;
   };


int createPosLinearActFunc( lua_State * L )
   {
   // Read a argument;
   double a = luaL_checknumber( L, 1 );

   // Read b argument;
   double b = luaL_checknumber( L, 2 );

   PosLinearActivationFunction * actFunc = new PosLinearActivationFunction( a, b );
   KernelObjectId id = kernel->insertObject( actFunc );

   lua_pushnumber( L, id );
   return 1;
   };


int createSigmoidActFunc( lua_State * L )
   {
   SigmoidActivationFunction * actFunc = new SigmoidActivationFunction();
   KernelObjectId id = kernel->insertObject( actFunc );

   lua_pushnumber( L, id );
   return 1;
   };


int createThSigmoidActFunc( lua_State * L )
   {
   ThSigmoidActivationFunction * actFunc = new ThSigmoidActivationFunction();
   KernelObjectId id = kernel->insertObject( actFunc );

   lua_pushnumber( L, id );
   return 1;
   };


int calcMeanCI( lua_State * L )
   {
   // Read mean argument;
   double mean = luaL_checknumber( L, 1 );

   // Read meansqr argument;
   double meansqr = luaL_checknumber( L, 2 );

   // Read times argument;
   double times = luaL_checkinteger( L, 3 );

   // Read beta argument;
   double beta = luaL_checknumber( L, 4 );

   double t = 0.0;
   if ( fabs( beta - 0.95 ) < 0.0001 ) t = 1.960;
   else if ( fabs( beta - 0.99 ) < 0.0001 ) t = 2.576;
   else if ( fabs( beta - 0.999 ) < 0.0001 ) t = 3.291;

   double delta = t * sqrt( ( meansqr - mean * mean ) / ( times - 1.0 ) );

   lua_pushnumber( L, delta );
   return 1;
   };


int calcACProbabilityCI( lua_State * L )
   {
   // Read p argument;
   double p = luaL_checknumber( L, 1 );

   // Read times argument;
   double times = luaL_checkinteger( L, 2 );

   // Read alpha argument;
   double alpha = luaL_checknumber( L, 3 );

   double x = 0.0;
   if ( fabs( alpha - 0.05 ) < 0.0001 ) x = 1.959964;
   else if ( fabs( alpha - 0.01 ) < 0.0001 ) x = 2.5758293;
   else if ( fabs( alpha - 0.001 ) < 0.0001 ) x = 3.2905267;

   double timesAC = times + x * x;
   double pAC = ( p * times + 0.5 * x * x ) / timesAC;
   double delta = x * sqrt( pAC * ( 1.0 - pAC ) / timesAC );

   lua_pushnumber( L, pAC - delta );
   lua_pushnumber( L, pAC + delta );
   return 2;
   };


/***************************************************************************
 *   Simulation engine API functions implementation                        *
 ***************************************************************************/


int createExponentialDestribution( lua_State * L )
   {
   KernelObjectId id = 0;

   // Read lambda argument;
   double lambda = luaL_checknumber( L, 1 );

   ExponentialDestribution * destribution = new ExponentialDestribution( lambda );
   id = kernel->insertObject( destribution );

   lua_pushnumber( L, id );
   return 1;
   };


int createWeibullDestribution( lua_State * L )
   {
   KernelObjectId id = 0;

   // Read teta argument;
   double teta = luaL_checknumber( L, 1 );

   // Read beta argument;
   double beta = luaL_checknumber( L, 2 );

   WeibullDestribution * destribution = new WeibullDestribution( teta, beta );
   id = kernel->insertObject( destribution );

   lua_pushnumber( L, id );
   return 1;
   };


int createAbstractWeightsManager( lua_State * L )
   {
   KernelObjectId id = 0;
   KernelObject * object = NULL;

   // Read destribution argument;
   KernelObjectId destributionId = luaL_checkinteger( L, 1 );
   object = kernel->getObject( destributionId );
   Destribution * destribution = dynamic_cast < Destribution * >( object );

   // Read weights argument;
   KernelObjectId weightsId = luaL_checkinteger( L, 2 );
   object = kernel->getObject( weightsId );
   AbstractWeights * weights = dynamic_cast < AbstractWeights * >( object );

   AbstractWeightsManager * manager = new AbstractWeightsManager( destribution, weights );
   id = kernel->insertObject( manager );

   lua_pushnumber( L, id );
   return 1;
   };


int createAnalogResistorsManager( lua_State * L )
   {
   KernelObjectId id = 0;
   KernelObject * object = NULL;

   // Read destribution argument;
   KernelObjectId destributionId = luaL_checkinteger( L, 1 );
   object = kernel->getObject( destributionId );
   Destribution * destribution = dynamic_cast < Destribution * >( object );

   // Read resistors argument;
   KernelObjectId resistorsId = luaL_checkinteger( L, 2 );
   object = kernel->getObject( resistorsId );
   AnalogResistors * resistors = dynamic_cast < AnalogResistors * >( object );

   AnalogResistorsManager * manager = new AnalogResistorsManager( destribution, resistors );
   id = kernel->insertObject( manager );

   lua_pushnumber( L, id );
   return 1;
   };


int createSimulationEngine( lua_State * L )
   {
   SimulationEngine * engine = new SimulationEngine();
   KernelObjectId id = kernel->insertObject( engine );

   lua_pushnumber( L, id );
   return 1;
   };


int appendInterruptManager( lua_State * L )
   {
   KernelObjectId id = 0;
   KernelObject * object = NULL;

   // Read engine argument;
   KernelObjectId engineId = luaL_checkinteger( L, 1 );
   object = kernel->getObject( engineId );
   SimulationEngine * engine = dynamic_cast < SimulationEngine * >( object );

   // Read manager argument;
   KernelObjectId managerId = luaL_checkinteger( L, 2 );
   object = kernel->getObject( managerId );
   InterruptManager * manager = dynamic_cast < InterruptManager * >( object );

   // Append manager;
   engine->appendManager( manager );

   return 0;
   };


int stepOverEngine( lua_State * L )
   {
   // Read engine argument;
   KernelObjectId engineId = luaL_checkinteger( L, 1 );
   KernelObject * object = kernel->getObject( engineId );
   SimulationEngine * engine = dynamic_cast < SimulationEngine * >( object );

   lua_pushboolean( L, engine->stepOver() );
   return 1;
   };


int getCurrentTime( lua_State * L )
   {
   // Read engine argument;
   KernelObjectId engineId = luaL_checkinteger( L, 1 );
   KernelObject * object = kernel->getObject( engineId );
   SimulationEngine * engine = dynamic_cast < SimulationEngine * >( object );

   lua_pushnumber( L, engine->getCurrentTime() );
   return 1;
   };


int getFutureTime( lua_State * L )
   {
   // Read engine argument;
   KernelObjectId engineId = luaL_checkinteger( L, 1 );
   KernelObject * object = kernel->getObject( engineId );
   SimulationEngine * engine = dynamic_cast < SimulationEngine * >( object );

   lua_pushnumber( L, engine->getFutureTime() );
   return 1;
   };


int getCurrentSource( lua_State * L )
   {
   // Read engine argument;
   KernelObjectId engineId = luaL_checkinteger( L, 1 );
   KernelObject * object = kernel->getObject( engineId );
   SimulationEngine * engine = dynamic_cast < SimulationEngine * >( object );

   KernelObjectId managerId = 0;
   int intSource = -1;

   InterruptManager * manager = engine->getCurrentIntSource();
   if ( manager != NULL )
      {
      managerId = kernel->getId( manager );
      intSource = manager->getLastIntSource();
      }

   lua_pushnumber( L, managerId );
   lua_pushnumber( L, intSource );
   return 2;
   };


int getFutureSource( lua_State * L )
   {
   // Read engine argument;
   KernelObjectId engineId = luaL_checkinteger( L, 1 );
   KernelObject * object = kernel->getObject( engineId );
   SimulationEngine * engine = dynamic_cast < SimulationEngine * >( object );

   KernelObjectId managerId = 0;
   int intSource = -1;

   InterruptManager * manager = engine->getFutureIntSource();
   if ( manager != NULL )
      {
      managerId = kernel->getId( manager );
      intSource = manager->getIntSource();
      }

   lua_pushnumber( L, managerId );
   lua_pushnumber( L, intSource );
   return 2;
   };
