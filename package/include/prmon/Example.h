#ifndef EXAMPLE_H
#define EXAMPLE_H

// // Copyright 2018 Graeme A Stewart <graeme.andrew.stewart@cern.ch>

// 

// Licensed under the Apache License, Version 2.0 (the "License");

// you may not use this file except in compliance with the License.

// You may obtain a copy of the License at

// 

//    http://www.apache.org/licenses/LICENSE-2.0

// 

// Unless required by applicable law or agreed to in writing, software

// distributed under the License is distributed on an "AS IS" BASIS,

// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.

// See the License for the specific language governing permissions and

// limitations under the License.



/**
 * @file
 * @author Graeme A Stewart <graeme.andrew.stewart@cern.ch>
 * @version 1.0
 *
 * @section DESCRIPTION
 *
 * An example class for the HSF
 *
 *
 */

namespace prmon {

  class Example {
  public:
    /// Default constructor
    Example();

    /// Default destructor
    virtual ~Example();

    /**
     * Constructor from a given number
     *
     * @param number initial value
     */
    Example(int number);

    /**
     * Get current value
     */
    int get() const;

    /**
     * Set new value
     *
     * @param number new value
     *
     */
     void set(int number);

  private:
    /// Internally stored number
    int m_number;
  };
}

#endif
