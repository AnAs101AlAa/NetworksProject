//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#ifndef __PROJECTHAMADA_NODE_H_
#define __PROJECTHAMADA_NODE_H_

#include <omnetpp.h>
#include <vector>
#include <fstream>
#include <string>
#include <bitset>
#include "customMessage_m.h"
using namespace std;

using namespace omnetpp;

/**
 * TODO - Generated class
 */
class Node : public cSimpleModule
{
  protected:

    int windowSize; //static window size = 5
    float transmissionDelay; //delay of sending frame
    float processTime; //delay for processing any frame to be sent
    float errorDelay; //delay added by error bit
    float duplicateDelay; //delay between original and duplicated messages
    float timeoutTime; //interval for a frame to re-transmit
    float lossProb; //probability of dropping a frame send

    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

#endif
