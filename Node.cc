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

//initialize all params and data members of the class
void Node::initialize()
{
    currMessage = 0;
    buffered = 0;
    expected_frame = 0;
    next_frame = 0;
    ack_expected = 0;
    scheduledChain = nullptr;

    transmissionDelay = par("transmissionDelay").doubleValue();
    windowSize = par("windowSize").intValue();
    processTime = par("processTime").doubleValue();
    errorDelay = par("errorDelay").doubleValue();
    duplicateDelay = par("duplicateDelay").doubleValue();
    timeoutTime = par("timeoutTime").doubleValue();
    lossProb = par("lossProb").doubleValue();

    node_id = getFullName()[4];
    string trash = "../src/output";
    trash += node_id;
    trash += ".txt";
    writeFile.open(trash, ios::out | ios::trunc);

    currWindow = new string[windowSize+1];
    timers = new cMessage*[windowSize+1];
    commandWindow = new string[windowSize+1];

    for(int i=0;i<=windowSize;i++) {
        timers[i] = nullptr;
    }
}

//uses regex as a custom sorting key for the log file
double extractTime(const std::string &line) {
    std::regex timeRegex(R"(At time ([0-9]*\.?[0-9]+))"); // Match "At time X"
    std::smatch match;
    if (std::regex_search(line, match, timeRegex)) {
        return std::stod(match[1]); // Convert matched time to double
    }
    return 0.0; // Default to 0.0 if no match found
}

// Comparator function for sorting
bool naturalSort(const std::string &a, const std::string &b) {
    double timeA = extractTime(a);
    double timeB = extractTime(b);
    return timeA < timeB;
}

//take output files of both nodes and merge them into output.txt
void Node::mergeFiles() {
    ifstream file1("../src/output0.txt"), file2("../src/output1.txt");

    vector<string> lines;
    string line;
    while(getline(file1, line)) {
        lines.push_back(line);
    }

    while(getline(file2, line)) {
        lines.push_back(line);
    }

    sort(lines.begin(), lines.end(), naturalSort);

    ofstream res("../src/output.txt", ios::out | ios::trunc);
    for(auto sortedLine: lines) {
        res << sortedLine<<endl;
    }

    file1.close();
    file2.close();
    res.close();
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
    scheduleAt(simTime() + timeoutTime + processTime, timers[seq_num]);
}

//stop the timeout delay for the frame
void Node::stopTimer(int seq_num) {
    // Check if the timer exists
    if (!timers[seq_num]) {
        EV << "Timer does not exist for seq_num: " << seq_num << endl;
        return;
    }

    // Try to cancel the event
    if (timers[seq_num]->isScheduled()) {
        cancelEvent(timers[seq_num]);
    }

    // Safely delete the timer and set pointer to nullptr
    delete timers[seq_num];
    timers[seq_num] = nullptr;

    EV << "Timers now:"<< endl;
    for(int i = 0;i< 5;i++) {
        if(!timers[i]){
            EV<<"null ";
            continue;
        }
        EV<<timers[i]->getName()<<" ";
    }
    EV<<endl;
}

//reads messages from file and inserts in class data member as whole line (modification bits included)
void Node::readDataFile() {
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
        if(c == '$' || c == '/') {
            framed += '/';
        }
        framed += c;
    }
    framed += '$';
    messageOut->setPayload(framed.c_str());
}

//takes the byte stuffing and flags out of message at receiver side
void Node::deframeData(CustomMessage_Base* msg) {
    string payload = msg->getPayload();
    string cleanString = "";
    string deflagged = payload.substr(2, payload.size() - 3);

    for (size_t i = 0; i < deflagged.size(); i++) {
        if (deflagged[i] == '/' && i + 1 < deflagged.size()) {
            i++;
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

    if((char)parity.to_ulong() == msg->getParity()){
        return true;
    }
    else {
        return false;
    }
}

//handles advancing window and sending out the message finally
void Node::sendMessage(int target, float delay = 0, string commands = "0000", bool dupped = false) {
    bool errorAdd = commands[0] - '0';
    bool errorLose = commands[1] - '0';
    bool errorDup = commands[2] - '0';
    bool errorDel = commands[3] - '0';

    int dupVersion = 0;
    if(dupped) {
        dupVersion = 2;
    }
    else if(errorDup) {
        dupVersion = 1;
    }

    if(commands[3] == '1') {
        delay += errorDelay;
    }
    if(commands[2] == '1') {
        string msgString = "duplicateLine";
        msgString += to_string(target);
        scheduleAt(simTime() + duplicateDelay, new cMessage(msgString.c_str()));
    }

    messageOut = new CustomMessage_Base();
    frameData(currWindow[target]);

    string correctPayload = messageOut->getPayload();
    int rand = intuniform(0, correctPayload.size()-1);
    if(errorAdd) {
        correctPayload[rand] = correctPayload[rand] - 1;
    }

    delay += processTime;
    parityApply(correctPayload);
    messageOut->setFType(2);
    messageOut->setSeq_num(target);
    startTimer(target);

    char parity = messageOut->getParity();
    bitset<8> paritybits(parity);

    writeFile << "At time "<<simTime() + processTime<<", Node ["<< node_id <<"] sent frame with seq_num["<<target<<"] and payload["<<correctPayload<<"] and trailer["<< paritybits<<"], modified["<<(errorAdd ? rand : -1)<<"], lost["<<(errorLose ? "yes" : "no")<<"], Duplicate["<<dupVersion<<"], Delay["<<(errorDel ? errorDelay : 0)<<"]."<<endl;
    writeFile.flush();

    EV<<"send message with seq_num:"<<messageOut->getSeq_num() << " payload:" << messageOut->getPayload() << " parity:" << messageOut->getParity() <<" dealy: "<<delay<<endl;
    if(errorLose) {
        return;
    }
    sendDelayed(messageOut, delay + transmissionDelay, gate("out"));
}

//processes the message from the file buffer and sends it out
void Node::processMessage(int target) {
    scheduledChain = nullptr;
    if(currMessage >= messageBuff.size()|| buffered == windowSize) {
        cout<<"done "<<simTime()<<endl;
        return;
    }

    //fetch new message from the file buffer
    string currString = from_buffer();
    string commands = currString.substr(0,4);
    string body = currString.substr(5);

    writeFile << "At time "<<simTime() <<", Node "<< node_id <<", introducing channel error with code = "<<commands<<endl;
    writeFile.flush();

    currWindow[target] = body;
    commandWindow[target] = commands;

    sendMessage(target, 0, commands);
    increment(next_frame);
    buffered++;
    scheduledChain = new cMessage("readNextLine");
    scheduleAt(simTime() + processTime, scheduledChain);
}

//message handler for all incoming messages receiver side and sender side
void Node::handleMessage(cMessage *msg)
{
    if (strcmp(msg->getClassName(), "omnetpp::cMessage") == 0) {
        string msgName = msg->getName();
        //first coordinator message (reads the file then processes the messages for buffer size)
        if(strncmp(msg->getName(),"wake up", 6) == 0) {
            readDataFile();
            processMessage(next_frame);
        }
        //handle processing a new frame and sending it
        else if(strcmp(msg->getName(), "readNextLine") == 0) {
            processMessage(next_frame);
        }
        //handles sending a duplicate frame (target number in the message)
        else if(strncmp(msg->getName(), "duplicateLine",12) == 0) {
            int target = stoi(msgName.substr(13));
            string errsNoDup = commandWindow[target];
            errsNoDup[2] = '0';
            sendMessage(target, 0, errsNoDup, true);
        }
        //timeout event (resends the frame with sequence number from the message)
        else if(msgName.size() > 7 && strncmp(msg->getName(), "timeout", 6) == 0) {
            int target = stoi(msgName.substr(7));
            int lower_limit = target;
            int step = 1;
            if(scheduledChain != nullptr) {
                cancelAndDelete(scheduledChain);
                EV<<"already scheduled"<<endl;
            }

            writeFile <<"At time "<<simTime()<<",Time out event at Node["<<node_id<<"] for frame with seq_num["<<target<<"]"<<endl;
            writeFile.flush();

            sendMessage(target, 0);
            increment(target);
            while(between(lower_limit, target, next_frame)) {
                string msgBody = "sendMessage";
                msgBody += to_string(target);
                cMessage* msgOut = new cMessage(msgBody.c_str());
                stopTimer(target);
                increment(target);
                scheduleAt(simTime() + step*processTime, msgOut);
                step++;
            }
        }

        else if(strncmp(msg->getName(), "sendMessage", 10) == 0) {
            int target = stoi(msgName.substr(11));
            sendMessage(target, 0, commandWindow[target]);
        }
    }
    else {
        CustomMessage_Base *mmsg = check_and_cast<CustomMessage_Base *>(msg);
        //received data frame (run checks and confirm or ignore)
        if(mmsg->getFType() == 2){
            float rand = uniform(0,1);
            if(mmsg->getSeq_num() == expected_frame) {
                if(parityCheck(mmsg)) {
                    deframeData(mmsg);
                    mmsg->setFType(1);
                    mmsg->setAck_num(mmsg->getSeq_num());

                    writeFile<<"At time "<<simTime()+processTime<<", Node["<<node_id<<"] Sending ack with number ["<<expected_frame<<"] , loss ["<<(rand > lossProb ? "no]" : "yes]")<<endl;
                    writeFile.flush();

                    if(rand > lossProb) {
                        sendDelayed(mmsg,processTime+transmissionDelay,gate("out"));
                    }
                    increment(expected_frame);
                    EV<<"got message with seq_num:"<<mmsg->getSeq_num() << " payload:" << mmsg->getPayload() << " parity:" << mmsg->getParity() <<"waiting for "<<expected_frame<<endl;
                    nackOn = false;
                }
                //handle sending nack for out of roder or errores frames
                else {
                    if(!nackOn) {
                        nackOn = true;
                        mmsg->setFType(0);
                        mmsg->setAck_num(expected_frame);

                        writeFile<<"At time "<<simTime()+processTime<<", Node["<<node_id<<"] Sending nack with number ["<<expected_frame<<"] , loss ["<<(rand > lossProb ? "no]" : "yes]")<<endl;
                        writeFile.flush();

                        if(rand > lossProb) {
                            sendDelayed(mmsg,processTime+transmissionDelay,gate("out"));
                            EV<<"sending nack on frame "<<expected_frame<<endl;
                        }
                        else{
                            EV<<"dropping nack"<<endl;
                        }
                    }
                }
            }
            else{
                return;
            }
        }

        else if(mmsg->getFType() == 1) {
            //receive frame ack (send next frame or ignore if numbers don't match)
            int received_ack = mmsg->getAck_num();
            int lower_limit = ack_expected;
            while(between(lower_limit, ack_expected, received_ack) || ack_expected == received_ack) {
                EV<<"got ack "<<ack_expected<<endl;
                stopTimer(ack_expected);
                increment(ack_expected);
                buffered--;
            }
            if(scheduledChain != nullptr) {
                EV<<"already scheduled"<<endl;
                return;
            }
            if(currMessage >= messageBuff.size()) {
                mergeFiles();
                return;
            }
            scheduledChain = new cMessage("readNextLine");
            scheduleAt(simTime(), scheduledChain);
        }
        //receive frame nack (resend all outstanding frames until end of window)
        else if(mmsg->getFType() == 0) {
            return;
        }
    }
}
