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

#include "Coordinator.h"
#include <fstream>
#include <string>
using namespace std;

Define_Module(Coordinator);

void Coordinator::initialize()
{
    ifstream readFile ("../src/coordinator.txt");
    if(!readFile.is_open()) {
        EV<<"DMSOMDOKSMD"<<endl;
    }
    string data;
    getline(readFile,data);
    int node_id = data[0] - '0';
    string start = "";

    for(int i=2;i<data.size();i++) {
        start += data[i];
    }

    float startTime = stof(start);
    readFile.close();
    EV<<node_id<<" "<<startTime<<endl;

    string msgString = "wake up";
    msgString += (node_id + '0');
    EV<<msgString<<endl;
    cMessage* msg = new cMessage(msgString.c_str());

    sendDelayed(msg, startTime, "out", node_id);

}

void Coordinator::handleMessage(cMessage *msg)
{
    // TODO - Generated method body
}
