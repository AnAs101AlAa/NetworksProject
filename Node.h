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
#include<regex>
#include<algorithm>
#include "customMessage_m.h"
using namespace std;

using namespace omnetpp;

/**
 * TODO - Generated class
 */
class Node : public cSimpleModule
{
  protected:
    //---------------------------------<data members for containers and pointers>----------------------------------//
    int currMessage; //index of frame to be sent in messageBuff
    string* currWindow; //array for current messages in the window
    string* commandWindow; //window to carry command bits
    cMessage* scheduledChain; //pointer to the self message of sending more lines (needed for duplication filtering)
    cMessage** timers; //array of pointers to timers of each frame in window
    vector<string> messageBuff; //buffer that has all messages read from file
    CustomMessage_Base* messageOut; //the custom message that contains all to be sent out for current message

    //---------------------------------<data members for go back n implementation>---------------------------------//
    int buffered; //amount of busy buffers of current window
    int ack_expected; //lower edge of sender window
    int next_frame; //upper edge of sender window
    int expected_frame; //frame to be received (receiver window)
    bool nackOn; // flag for nack on message

    //---------------------------------<data members for simulation constants>-------------------------------------//
    int windowSize; //static window size = 5
    float transmissionDelay; //delay of sending frame
    float processTime; //delay for processing any frame to be sent
    float errorDelay; //delay added by error bit
    float duplicateDelay; //delay between original and duplicated messages
    float timeoutTime; //interval for a frame to re-transmit
    float lossProb; //probability of dropping a frame send

    ofstream writeFile;
    char node_id;

    virtual void mergeFiles();
    virtual void increment(int& num);
    virtual bool between(int a,int b,int c);
    virtual void startTimer(int seq_num);
    virtual void stopTimer(int seq_num);
    virtual string from_buffer();
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
};

#endif
