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

package projecthamada;

import ned.IdealChannel;


network Network
{
    @display("bgb=849,468");
    submodules:
        coordinator: Coordinator {
            @display("p=421,333");
        }
        node: Node {
            @display("p=171,165");
        }
        node1: Node {
            @display("p=656,165");
        }
    connections:
        node1.out --> IdealChannel --> node.in[0];
        node.out --> IdealChannel --> node1.in[0];
        coordinator.out[0] --> IdealChannel --> node.in[1];
        coordinator.out[1] --> IdealChannel --> node1.in[1];
}
