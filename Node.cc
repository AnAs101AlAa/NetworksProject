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
    currMessage = 0;
    buffered = 0;
    expected_frame = 0;
    next_frame = 0;
    ack_expected = 0;

    transmissionDelay = par("transmissionDelay").doubleValue();
    windowSize = par("windowSize").intValue();
    processTime = par("processTime").doubleValue();
    errorDelay = par("errorDelay").doubleValue();
    duplicateDelay = par("duplicateDelay").doubleValue();
    timeoutTime = par("timeoutTime").doubleValue();
    lossProb = par("lossProb").doubleValue();

    currWindow = new string[windowSize+1];
    timers = new cMessage*[windowSize+1];
    for(int i=0;i<windowSize;i++) {
        timers[i] = nullptr;
    }
}

//increment any required number in a circular order using windowSize
void Node::increment(int& num) {
    num = (num+1)%(windowSize+1);
}

//checks if b is between a and b circularly
bool Node::between(int a,int b,int c) {
    return (((a <= b) && (b < c)) || ((c < a) && (a <= b)) || ((b < c) && (c < a)));
}

//start timeout delay for a single frame
void Node::startTimer(int seq_num) {
    if(timers[seq_num] != nullptr) {
        return;
    }

    string msgName = "timeout";
    msgName += to_string(seq_num);
    timers[seq_num] = new cMessage(msgName.c_str());
    scheduleAt(simTime() + timeoutTime, timers[seq_num]);
}

//stop the timeout delay for the frame
void Node::stopTimer(int seq_num) {
    cancelEvent(timers[seq_num]);
    delete timers[seq_num];
    timers[seq_num] = nullptr;
}

//reads messages from file and inserts in class data member as whole line (modification bits included)
void Node::readDataFile(char node_id) {
    string fileName = "../src/node";
    fileName += node_id;
    fileName += ".txt";
    ifstream readFile (fileName);
    string currLine;
    while(getline(readFile,currLine)) {
        messageBuff.push_back(currLine);
    }
}

//fetch a new string frame from whole file buffer
string Node::from_buffer() {
    string ret = messageBuff[currMessage];
    currMessage++;
    return ret;
}

//byte stuffing inserts flag at start and end and stuffs all inside. flag is '$' and escape is '\'
void Node::frameData(string payload) {
    string framed = "$";
    for(char c: payload) {
        if(c == '$' || c == '\\') {
            framed += '\\';
        }
        framed += c;
    }
    framed += '$';
    messageOut->setPayload(framed.c_str());
}

void Node::deframeData(CustomMessage_Base* msg) {
    string payload = msg->getPayload();
    string cleanString = "";
    string deflagged = payload.substr(2, payload.size() - 3);

    for (size_t i = 0; i < deflagged.size(); ++i) {
        if (deflagged[i] == '\\' && i + 1 < deflagged.size()) {
            ++i;
        }
        cleanString += deflagged[i];
    }
    msg->setPayload(cleanString.c_str());
}

//takes current message and gets the even parity then adds it to member custom message in the trailer
void Node::parityApply(string payload) {
    vector<bitset<8>> msgBits;
     bitset<8> parity(0);
     for (int i = 0; i < payload.size(); i++) {
         msgBits.push_back(bitset<8>((unsigned char)payload[i]));
         parity = parity ^ msgBits[i];
     }

     messageOut->setParity((char)parity.to_ulong());
}

//takes incoming message and checks for error using parity byte at trailer
bool Node::parityCheck(CustomMessage_Base* msg) {
    bitset<8> parity(0);
    string payload = msg->getPayload();

    for (int i = 0; i < payload.size(); i++) {
        bitset<8> msgBits((unsigned char)payload[i]);
        parity = parity ^ msgBits;
    }

    return ((char)parity.to_ulong() == msg->getParity());
}

//handles advancing window and sending out the message finally
void Node::sendMessage(int target, float delay = 0, bool errorAdd = false) {
    messageOut = new CustomMessage_Base();
    frameData(currWindow[target]);

    string correctPayload = messageOut->getPayload();
    if(errorAdd) {
        int rand = intuniform(0, correctPayload.size()-1);
        correctPayload[rand] = correctPayload[rand] - 1;
    }

    delay += processTime;
    parityApply(correctPayload);
    messageOut->setFType(2);
    messageOut->setSeq_num(target);
    startTimer(target);
    EV<<"send message with seq_num:"<<messageOut->getSeq_num() << " payload:" << currWindow[target] << " parity:" << messageOut->getParity() <<" dealy: "<<delay<<endl;
    sendDelayed(messageOut, delay + transmissionDelay, gate("out"));
}

// *still in work* handles sending messages in windows
void Node::processMessage(int target) {
    if(currMessage == messageBuff.size()|| buffered == windowSize) {
        return;
    }
    string currString = from_buffer();
    string commands = currString.substr(0,4);
    string body = currString.substr(4);
    bool errorAdd = false;

    float totalDelay = 0;
    if(commands[1] == '1') {
        EV<<"dropping frame seq_num "<<target<<endl;
        increment(next_frame);
        scheduleAt(simTime() + processTime, new cMessage("readNextLine"));
        currWindow[target] = body;
        startTimer(target);
        return;
    }
    if(commands[3] == '1') {
        totalDelay += errorDelay;
    }
    if(commands[2] == '1') {
        string msgString = "duplicateLine" + to_string(totalDelay);
        scheduleAt(simTime() + duplicateDelay, new cMessage(msgString.c_str()));
    }
    if(commands[0] == '1') {
        errorAdd = true;
    }

    currWindow[target] = body;
    sendMessage(target, totalDelay, errorAdd);
    increment(next_frame);
    buffered++;
    scheduleAt(simTime() + processTime, new cMessage("readNextLine"));

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
