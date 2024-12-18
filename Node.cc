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

#include "Node.h"
using namespace std;

Define_Module(Node);

void Node::initialize()
{
    transmissionDelay = par("transmissionDelay").doubleValue();
    windowSize = par("windowSize").intValue();
    processTime = par("processTime").doubleValue();
    errorDelay = par("errorDelay").doubleValue();
    duplicateDelay = par("duplicateDelay").doubleValue();
    timeoutTime = par("timeoutTime").doubleValue();
    lossProb = par("lossProb").doubleValue();
}


void Node::handleMessage(cMessage *msg)
{
    if (strcmp(msg->getClassName(), "omnetpp::cMessage") == 0) {
        string msgName = msg->getName();
        //first coordinator message (reads the file then processes the messages for buffer size)
        if(strncmp(msg->getName(),"wake up", 6) == 0) {
            readDataFile(msgName[msgName.size()-1]);
            processMessage(next_frame);
        }
        else if(strcmp(msg->getName(), "readNextLine") == 0) {
            processMessage(next_frame);
        }
        else if(strncmp(msg->getName(), "duplicateLine",12) == 0) {
            int dupDelay = stoi(msgName.substr(13));
            int num = (next_frame + windowSize) % (windowSize + 1);
            sendMessage(num, dupDelay);
        }
        //timeout event (resends the frame with sequence number from the message
        else if(msgName.size() > 7 && strncmp(msg->getName(), "timeout", 6) == 0) {
            int target = msgName[7] - '0';
            sendMessage(target);
        }
    }
    else {
        CustomMessage_Base *mmsg = check_and_cast<CustomMessage_Base *>(msg);
        //received data frame (run checks and confirm or ignore)
        if(mmsg->getFType() == 2){
            if(mmsg->getSeq_num() == expected_frame && parityCheck(mmsg)) {
                deframeData(mmsg);
                EV<<"got message with seq_num:"<<mmsg->getSeq_num() << " payload:" << mmsg->getPayload() << " parity:" << mmsg->getParity() <<endl;
                mmsg->setFType(1);
                mmsg->setAck_num(mmsg->getSeq_num());
                sendDelayed(mmsg,processTime+transmissionDelay,gate("out"));
                increment(expected_frame);
                nackOn = false;
            }
            else {
                if(!nackOn) {
                    EV<<"sending nack on frame "<<expected_frame<<endl;
                    nackOn = true;
                    mmsg->setFType(0);
                    mmsg->setAck_num(expected_frame);
                    sendDelayed(mmsg,processTime+transmissionDelay,gate("out"));
                }
            }
        }

        else if(mmsg->getFType() == 1) {
            //receive frame ack (send next frame or ignore if numbers don't match)
            int received_ack = mmsg->getAck_num();
            int lower_limit = ack_expected;
            while(between(lower_limit, ack_expected, received_ack) || ack_expected == received_ack) {
                EV<<"got ack"<<endl;
                stopTimer(ack_expected);
                increment(ack_expected);
                buffered--;
                processMessage(next_frame);
            }
        }

        else if(mmsg->getFType() == 0) {
            EV<<"got nack"<<endl;
            int current_ack = mmsg->getAck_num();
            int lower_limit = current_ack;
            int step = 1;
            while(between(lower_limit, current_ack, next_frame)) {
                stopTimer(current_ack);
                string msgName = "timeout";
                msgName += to_string(current_ack);
                cMessage* slfmsg = new cMessage(msgName.c_str());
                scheduleAt(simTime() + processTime*step, slfmsg);
                step++;
                increment(current_ack);
            }
        }
    }
}
