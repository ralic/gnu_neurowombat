/***************************************************************************
 *   Copyright (C) 2009, 2010 Andrew Timashov                              *
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


#include "neurons/abstract/AbstractNeuron.h"


#include <string.h>


/***************************************************************************
 *   AbstractNeuronExcp class implementation                               *
 ***************************************************************************/


AbstractNeuronExcp::AbstractNeuronExcp( NS_ABSTRACTNEURON::EC error )
   : Exception < NS_ABSTRACTNEURON::EC > ( error )
   {
   // Do nothing;
   };


/***************************************************************************
 *   AbstractNeuron class implementation                                   *
 ***************************************************************************/


AbstractNeuron::AbstractNeuron(
   unsigned int inputsCount,
   unsigned int * inputConnectors,
   AbstractConnectors * connectors,
   unsigned int connectorsBaseIndex,
   AbstractWeights * weights,
   unsigned int weightsBaseIndex,
   ProcessingUnit * processingUnit,
   ActivationFunction * activationFunction
   )
   : KernelObject()
   {
   this->inputsCount = inputsCount;

   // Allocate memory for inputConnectors;
   if ( inputsCount > 0 && inputConnectors != NULL )
      {
      this->inputConnectors = new unsigned int[ inputsCount ];

      // Fill inputConnectors;
      if ( this->inputConnectors != NULL )
         {
         memcpy( this->inputConnectors, inputConnectors, inputsCount * sizeof( unsigned int ) );
         }
      }
   else
      {
      this->inputConnectors = NULL;
      }

   // Setup connectors;
   this->connectors = connectors;
   this->connectorsBaseIndex = connectorsBaseIndex;
   if ( connectors != NULL ) connectors->capture();

   // Try to create built-in weights;
   if ( weights == NULL && inputsCount > 0 )
      {
      builtInWeights = new double[ inputsCount ];
      }
   else
      {
      builtInWeights = NULL;
      }

   // Setup weights;
   this->weights = weights;
   this->weightsBaseIndex = weightsBaseIndex;
   if ( weights != NULL ) weights->capture();

   // Setup built-in buffers;
   this->builtInBuffers = NULL;

   // Setup processingUnit;
   this->processingUnit = processingUnit;
   if ( processingUnit != NULL ) processingUnit->capture();

   // Setup activationFunction;
   this->activationFunction = activationFunction;
   if ( activationFunction != NULL ) activationFunction->capture();

   // Setup processingUnitOut;
   this->processingUnitOut = 0.0;

   // Setup delta;
   this->delta = 0.0;
   };


AbstractNeuron::~AbstractNeuron()
   {
   if ( this->inputConnectors != NULL ) delete[] this->inputConnectors;
   if ( this->builtInWeights != NULL ) delete[] this->builtInWeights;
   if ( this->builtInBuffers != NULL ) delete[] this->builtInBuffers;

   // Release captured objects;
   if ( connectors != NULL ) connectors->release();
   if ( weights != NULL ) weights->release();
   if ( activationFunction != NULL ) activationFunction->release();
   if ( processingUnit != NULL ) processingUnit->release();
   };


unsigned int AbstractNeuron::getInputsCount() const
   {
   return this->inputsCount;
   };


void AbstractNeuron::setWeight( unsigned int index, double weight )
   {
   if ( this->weights == NULL )
      {
      // Set built-in weight;
      this->builtInWeights[ index ] = weight;
      }
   else
      {
      // Set external weight;
      this->weights->at( weightsBaseIndex + index ) = weight;
      }
   };


double AbstractNeuron::getWeight( unsigned int index )
   {
   if ( this->weights == NULL )
      {
      // Return built-in weights;
      return this->builtInWeights[ index ];
      }
   else
      {
      // Return external weights;
      return this->weights->at( weightsBaseIndex + index );
      }
   };


double AbstractNeuron::getOutput()
   {
   return connectors->at( connectorsBaseIndex );
   };


double AbstractNeuron::leftCompute()
   {
   // Calculate processor out;
   processingUnitOut = processingUnit->process(
      inputsCount, inputConnectors, connectors,
      builtInWeights, weights, weightsBaseIndex
      );

   return processingUnitOut;
   };


void AbstractNeuron::rightCompute( double processingUnitOut )
   {
   this->processingUnitOut = processingUnitOut;
   connectors->at( connectorsBaseIndex ) = activationFunction->evaluateFunction( processingUnitOut );
   };


void AbstractNeuron::compute()
   {
   // Calculate processor out;
   processingUnitOut = processingUnit->process(
      inputsCount, inputConnectors, connectors,
      builtInWeights, weights, weightsBaseIndex
      );

   connectors->at( connectorsBaseIndex ) = activationFunction->evaluateFunction( processingUnitOut );
   };


void AbstractNeuron::createDampingBuffers()
   {
   if ( builtInBuffers == NULL )
      {
      builtInBuffers = new double[ inputsCount ];
      for ( unsigned int i = 0; i < inputsCount; i ++ )
         {
         builtInBuffers[ i ] = 0.0;
         };
      };
   };


void AbstractNeuron::snapDelta( double err )
   {
   delta = err;
   delta *= activationFunction->evaluateDerivative( processingUnitOut );
   };


double AbstractNeuron::getDelta()
   {
   return delta;
   };


double AbstractNeuron::getWeightedDelta( unsigned int index )
   {
   if ( weights == NULL )
      {
      // Use build-in weights;
      return delta * builtInWeights[ index ];
      }
   else
      {
      // Use external weights;
      return delta * weights->at( weightsBaseIndex + index );
      }
   };


void AbstractNeuron::modifyWeights( double damping, double speed )
   {
   double d = delta * speed;
   double dw = 0;

   if ( weights == NULL )
      {
      // Use build-in weights;
      for ( unsigned int i = 0; i < inputsCount; i ++ )
         {
         dw = damping * builtInBuffers[ i ] +
            d * connectors->at( inputConnectors[ i ] );
         builtInBuffers[ i ] = dw;
         builtInWeights[ i ] += dw;
         }
      }
   else
      {
      // Use external weights;
      for ( unsigned int i = 0; i < inputsCount; i ++ )
         {
         dw = damping * builtInBuffers[ i ] +
            d * connectors->at( inputConnectors[ i ] );
         builtInBuffers[ i ] = dw;
         weights->at( weightsBaseIndex + i ) += dw;
         }
      }
   };


AbstractNeuron::AbstractNeuron()
   : KernelObject()
   {
   // Do nothing;
   };


AbstractNeuron::AbstractNeuron( const AbstractNeuron & other )
   {
   // Do nothing;
   };
